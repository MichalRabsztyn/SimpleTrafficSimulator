#include "MR_VehicleSpawningComponent.h"

#include "Components/SplineComponent.h"

#include "MR_VehiclePawn.h"


UMR_VehicleSpawningComponent::UMR_VehicleSpawningComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MinSpawnInterval = 1.0f;
	MaxSpawnInterval = 2.0f;

	bRetrySpawn = true;
	SpawnRetryFrequency = 1.0f;
}

void UMR_VehicleSpawningComponent::BeginPlay()
{
	Super::BeginPlay();

	const AActor* const Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	const USplineComponent* const SplineComponent = Owner->FindComponentByClass<USplineComponent>();
	if (IsValid(SplineComponent))
	{
		const float ClosestPointOnSpline = SplineComponent->FindInputKeyClosestToWorldLocation(GetComponentLocation());
		const FVector NewLocation = SplineComponent->GetLocationAtSplineInputKey(ClosestPointOnSpline, ESplineCoordinateSpace::World);
		const FRotator NewRotation = SplineComponent->GetRotationAtSplineInputKey(ClosestPointOnSpline, ESplineCoordinateSpace::World);

		SetWorldLocationAndRotation(NewLocation, NewRotation);
	}

	CollectVehiclePool();

	SpawnVehicle(FMath::RandRange(0, VehiclePool.Num()));
}

void UMR_VehicleSpawningComponent::SpawnVehicle(const uint8 VehicleIndexInPool)
{
	UWorld* const World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AActor* const Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	TArray<uint8> AvailableVehiclePoolIndices;
	GetAvailableVehiclePoolIndices(AvailableVehiclePoolIndices);

	if (AvailableVehiclePoolIndices.Num() == 0)
	{
		ScheduleSpawn(false, VehicleIndexInPool);
		return;
	}

	uint8 VehicleIndexToSpawn = VehicleIndexInPool;
	if (VehicleIndexInPool >= VehiclePool.Num())
	{
		const uint8 RandomIndex = FMath::RandRange(0, AvailableVehiclePoolIndices.Num() - 1);
		VehicleIndexToSpawn = AvailableVehiclePoolIndices[RandomIndex];
	}

	AMR_VehiclePawn* const VehicleToSpawn = VehiclePool[VehicleIndexToSpawn].Get();
	if (!IsValid(VehicleToSpawn))
	{
		ScheduleSpawn(false, VehiclePool.Num());
		return;
	}

	const FVector SpawnLocation = GetComponentLocation();
	const FRotator SpawnRotation = GetComponentRotation();

	const bool bMoved = VehicleToSpawn->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, true);
	if (!bMoved)
	{
		ScheduleSpawn(false, VehicleIndexInPool);
	}
	else
	{
		VehicleToSpawn->SetIsActive(true);
		ScheduleSpawn(true, VehiclePool.Num());
	}
}

void UMR_VehicleSpawningComponent::ScheduleSpawn(const bool bVehicleSpawned, const uint8 VehicleIndexInPool)
{
	UWorld* const World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const bool bShouldRetrySpawn = bRetrySpawn && !bVehicleSpawned;
	const float SpawnInterval = bShouldRetrySpawn ? SpawnRetryFrequency : FMath::RandRange(MinSpawnInterval, MaxSpawnInterval);
	
	const FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UMR_VehicleSpawningComponent::SpawnVehicle, VehicleIndexInPool);
	
	World->GetTimerManager().SetTimer(SpawnTimerHandle, TimerDelegate, SpawnInterval, false);
}

void UMR_VehicleSpawningComponent::CollectVehiclePool()
{
	AActor* const Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	TArray<AActor*> AttachedActors;
	Owner->GetAttachedActors(AttachedActors, true);

	AttachedActors.FilterByPredicate([this, Owner](AActor* Actor) -> bool {
		if (AMR_VehiclePawn* AttachedActor = Cast<AMR_VehiclePawn>(Actor))
		{
			VehiclePool.Add(AttachedActor);
			AttachedActor->SetIsActive(false);
			AttachedActor->SetOwner(Owner);
			return true;
		}
		return false;
		});

}

void UMR_VehicleSpawningComponent::GetAvailableVehiclePoolIndices(TArray<uint8>& OutAvailableVehiclePoolIndices) const
{
	OutAvailableVehiclePoolIndices.Empty();

	for (int32 i = 0; i < VehiclePool.Num(); ++i)
	{
		AMR_VehiclePawn* Vehicle = VehiclePool[i].Get();
		if (IsValid(Vehicle) && !Vehicle->GetIsActive())
		{
			OutAvailableVehiclePoolIndices.Add(i);
		}
	}
}