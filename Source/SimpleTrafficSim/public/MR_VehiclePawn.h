#pragma once

#include "CoreMinimal.h"

#include "MR_VehiclePawn.generated.h"

class USplineComponent;
class UStaticMeshComponent;

UCLASS(ClassGroup = (TrafficSim), Blueprintable, BlueprintType, Abstract, meta = (PrioritizeCategories = "VehicleProperties"))
class SIMPLETRAFFICSIM_API AMR_VehiclePawn : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* VehicleMesh;
	TWeakObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, UIMin = 0), Category = "VehicleProperties|Common")
	float MinPreferredSpeed;
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, UIMin = 0), Category = "VehicleProperties|Common")
	float MaxPreferredSpeed;
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, UIMin = 0), Category = "VehicleProperties|Common")
	float AccelerationRate;
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, UIMin = 0), Category = "VehicleProperties|Common")
	float DecelerationRate;
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, UIMin = 0), Category = "VehicleProperties|Common")
	float SafeDistance;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0, UIMin = 0), Category = "VehicleProperties|Detection")
	uint8 DistanceCheckSamplers;
	UPROPERTY(EditAnywhere, meta = (Units = "Degrees", ClampMin = 0, UIMin = 0, ClampMax = 360, UIMax = 360),
		Category = "VehicleProperties|Detection")
	float DistanceCheckSamplersMaxAngle;
	UPROPERTY(EditAnywhere, meta = (Units = "Seconds", ClampMin = 0, UIMin = 0), Category = "VehicleProperties|Detection")
	float DistanceCheckFrequency;
	UPROPERTY(EditAnywhere, Category = "VehicleProperties|Detection")
	FVector DistanceCheckSamplersStartOffset;
	
	UPROPERTY(EditAnywhere, Category = "VehicleProperties|Debug")
	bool bDrawDebugLines;
	UPROPERTY(EditAnywhere, Category = "VehicleProperties|Debug")
	bool bDrawDebugHitPoints;

	float InitialAccelerationRate;
	float StartSpeed;
	float PreferredSpeed;
	float CurrentSpeed;
	float CurrentSplinePosition;
	float BufferDistance;

	bool bIsActive;

	FTimerHandle DistanceCheckTimerHandle;
	FTraceDelegate DistanceCheckTraceHandle;

public:
	AMR_VehiclePawn();

	void SetIsActive(const bool bActive);
	FORCEINLINE bool GetIsActive() const { return bIsActive; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	void UpdateSpeed(float DeltaTime);
	void UpdatePosition(float DeltaTime);
	
	void CheckDistance();
	void HandleDistanceCheckResults(const TArray<FHitResult>& HitResults);
	float CalculateBrakingDistance(float Speed, float Deceleration);

	void BeginRide();
	void EndRide();
};
