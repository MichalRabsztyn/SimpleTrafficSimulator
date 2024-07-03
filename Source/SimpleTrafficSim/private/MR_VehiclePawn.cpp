#include "MR_VehiclePawn.h"
#include "DrawDebugHelpers.h"
#include "Components/SplineComponent.h"
#include "MR_RoadActor.h"
#include "Engine/World.h"
#include "TimerManager.h"

AMR_VehiclePawn::AMR_VehiclePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
	
	VehicleMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	VehicleMesh->SetSimulatePhysics(false);
	VehicleMesh->SetEnableGravity(false);
	VehicleMesh->SetGenerateOverlapEvents(false);
	VehicleMesh->SetCastShadow(false);

	MinPreferredSpeed = 20.0f;
	MaxPreferredSpeed = 140.0f;

	SafeDistance = 100.0f;
	BufferDistance = 50.0f;

	AccelerationRate = 2.0f;
	InitialAccelerationRate = 0.5f;
	DecelerationRate = 1.0f;

	DistanceCheckSamplers = 1;
	DistanceCheckSamplersMaxAngle = 0.0f;
	DistanceCheckFrequency = 0.1f;
	DistanceCheckSamplersStartOffset = FVector::ZeroVector;

	bDrawDebugLines = false;
	bDrawDebugHitPoints = false;

	SetIsActive(false);
}

void AMR_VehiclePawn::SetIsActive(const bool bActive)
{
	bIsActive = bActive;
	SetActorHiddenInGame(!bActive);
	SetActorTickEnabled(bActive);
	SetActorEnableCollision(bActive);

	bActive ? BeginRide() : EndRide();
}

void AMR_VehiclePawn::BeginRide()
{
	StartSpeed = FMath::RandRange(MinPreferredSpeed, MaxPreferredSpeed);
	PreferredSpeed = StartSpeed;
	CurrentSpeed = 0.0f;

	CurrentSplinePosition = Spline.IsValid() ? Spline->GetDistanceAlongSplineAtLocation(GetActorLocation(), ESplineCoordinateSpace::World) : 0.0f;
	
	CheckDistance();
	GetWorldTimerManager().SetTimer(DistanceCheckTimerHandle, this, &AMR_VehiclePawn::CheckDistance, DistanceCheckFrequency, true);
}

void AMR_VehiclePawn::EndRide()
{
	if(DistanceCheckTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(DistanceCheckTimerHandle);
	}
}

void AMR_VehiclePawn::BeginPlay()
{
	Super::BeginPlay();

	AMR_RoadActor* const Road = Cast<AMR_RoadActor>(GetOwner());
	if (IsValid(Road))
	{
		Spline = TWeakObjectPtr<USplineComponent>(Road->GetComponentByClass<USplineComponent>());
	}
}

void AMR_VehiclePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSpeed(DeltaTime);
	UpdatePosition(DeltaTime);
}

void AMR_VehiclePawn::UpdateSpeed(float DeltaTime)
{
	const float EffectiveAccelerationRate = CurrentSpeed == 0.0f ? InitialAccelerationRate : AccelerationRate;
	const float UpdateRate = CurrentSpeed < PreferredSpeed ? EffectiveAccelerationRate : DecelerationRate;

	CurrentSpeed = FMath::FInterpTo(CurrentSpeed, PreferredSpeed, DeltaTime, UpdateRate);

	if (CurrentSpeed < 0.0f)
	{
		CurrentSpeed = 0.0f;
	}

	if (FMath::Abs(CurrentSpeed - PreferredSpeed) < KINDA_SMALL_NUMBER)
	{
		CurrentSpeed = PreferredSpeed;
	}
}

void AMR_VehiclePawn::UpdatePosition(float DeltaTime)
{
	if (!Spline.IsValid())
	{
		return;
	}

	CurrentSplinePosition += CurrentSpeed * DeltaTime;

	const float SplineLength = Spline->GetSplineLength();
	if (CurrentSplinePosition >= SplineLength)
	{
		SetIsActive(false);
		return;
	}

	const FVector NewLocation = Spline->GetLocationAtDistanceAlongSpline(CurrentSplinePosition, ESplineCoordinateSpace::World);
	const FRotator NewRotation = Spline->GetRotationAtDistanceAlongSpline(CurrentSplinePosition, ESplineCoordinateSpace::World);

	SetActorLocationAndRotation(NewLocation, NewRotation);
}

void AMR_VehiclePawn::CheckDistance()
{
	if (DistanceCheckSamplers <= 0 || DistanceCheckSamplersMaxAngle < 0.0f)
	{
		return;
	}

	TArray<TFuture<TArray<FHitResult>>> FutureResults;

	const FVector StartLocation = GetActorLocation() + DistanceCheckSamplersStartOffset;
	const FVector ForwardDirection = GetActorForwardVector();
	const float BrakingDistance = CalculateBrakingDistance(CurrentSpeed, DecelerationRate);
	const float VehicleLength = IsValid(VehicleMesh) ? VehicleMesh->Bounds.BoxExtent.X : 0.0f;
	const float DistanceCheckSamplersMaxDistance = BrakingDistance + SafeDistance + BufferDistance + VehicleLength;
	const float HalfMaxAngle = DistanceCheckSamplers == 1 ? 0.0f : DistanceCheckSamplersMaxAngle * 0.5f;
	const float AngleIncrement = DistanceCheckSamplers == 1 ? 0.0f : DistanceCheckSamplersMaxAngle / (DistanceCheckSamplers - 1);

	for (uint8 i = 0; i < DistanceCheckSamplers; ++i)
	{
		const float Angle = DistanceCheckSamplers == 1 ? 0.0f : -HalfMaxAngle + i * AngleIncrement;
		const FRotator Rotation = FRotator(0.0f, Angle, 0.0f);
		const FVector Direction = Rotation.RotateVector(ForwardDirection);
		const FVector EndLocation = StartLocation + Direction * DistanceCheckSamplersMaxDistance;

		FutureResults.Add(Async(EAsyncExecution::Thread, [this, StartLocation, EndLocation]()
			{
				TArray<FHitResult> HitResults;
				GetWorld()->LineTraceMultiByChannel(HitResults, StartLocation, EndLocation, ECC_Visibility);
				return HitResults;
			}));

		if (bDrawDebugLines)
		{
			DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, DistanceCheckFrequency, 0, 1.0f);
		}
	}

	for (TFuture<TArray<FHitResult>>& Future : FutureResults)
	{
		Future.Wait();
		TArray<FHitResult> HitResults = Future.Get();
		HandleDistanceCheckResults(HitResults);
	}
}

void AMR_VehiclePawn::HandleDistanceCheckResults(const TArray<FHitResult>& HitResults)
{
	if (HitResults.Num() <= 0)
	{
		PreferredSpeed = StartSpeed;
		return;
	}

	const UWorld* const World = GetWorld();
	if (!IsValid(World))
	{
		PreferredSpeed = 0.0f;
		return;
	}

	bool bSafeToAccelerate = true;
	float ClosestSpeed = MaxPreferredSpeed;

	for (const FHitResult& HitResult : HitResults)
	{
		const AMR_VehiclePawn* const OtherVehicle = Cast<AMR_VehiclePawn>(HitResult.GetActor());
		if (!IsValid(OtherVehicle) || !IsValid(OtherVehicle->VehicleMesh))
		{
			PreferredSpeed = 0.0f;
			return;
		}

		if (bDrawDebugHitPoints)
		{
			DrawDebugSphere(World, HitResult.ImpactPoint, 10.0f, 12, FColor::Green, false, DistanceCheckFrequency, 0, 1.0f);
		}

		const float Distance = HitResult.Distance;
		const float FullSafeDistance = SafeDistance + BufferDistance + VehicleMesh->Bounds.BoxExtent.X + OtherVehicle->VehicleMesh->Bounds.BoxExtent.X;

		if (Distance <= FullSafeDistance)
		{
			bSafeToAccelerate = false;
			ClosestSpeed = OtherVehicle->CurrentSpeed;
		}
		else if (Distance <= FullSafeDistance * 2.0f)
		{
			bSafeToAccelerate = false;
			ClosestSpeed = OtherVehicle->CurrentSpeed;
		}
		else
		{
			ClosestSpeed = FMath::Min(ClosestSpeed, OtherVehicle->CurrentSpeed);
		}
	}

	if (bSafeToAccelerate)
	{
		PreferredSpeed = StartSpeed;
	}
	else
	{
		PreferredSpeed = ClosestSpeed;
	}
}

float AMR_VehiclePawn::CalculateBrakingDistance(float Speed, float Deceleration)
{
	if (Deceleration <= 0.0f)
	{
		return 0.0f;
	}
	return (Speed * Speed) / (2 * Deceleration);
}
