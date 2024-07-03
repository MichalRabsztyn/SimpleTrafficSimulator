#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "MR_VehicleSpawningComponent.generated.h"

class AMR_VehiclePawn;

UCLASS(ClassGroup=(TrafficSim), meta=(BlueprintSpawnableComponent))
class SIMPLETRAFFICSIM_API UMR_VehicleSpawningComponent : public USceneComponent
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AMR_VehiclePawn>> VehiclePool;

	UPROPERTY(EditAnywhere, meta = (Units = "Seconds", ClampMin = 0, UIMin = 0), Category = "Spawner Properties|Spawn Interval")
	float MinSpawnInterval;
	UPROPERTY(EditAnywhere, meta = (Units = "Seconds", ClampMin = 0, UIMin = 0), Category = "Spawner Properties|Spawn Interval")
	float MaxSpawnInterval;
	UPROPERTY(EditAnywhere, Category = "Spawner Properties|Spawn Interval")
	bool bRetrySpawn;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bRetrySpawn", EditConditionHides,
		Units = "Seconds", ClampMin = 0, UIMin = 0), Category = "Spawner Properties|Spawn Interval")
	float SpawnRetryFrequency;

	FTimerHandle SpawnTimerHandle;

public:
	UMR_VehicleSpawningComponent();

protected:
	virtual void BeginPlay() override;

private:
	void SpawnVehicle(const uint8 VehicleIndexInPool);
	void ScheduleSpawn(const bool bVehicleSpawned, const uint8 VehicleIndexInPool);

	void CollectVehiclePool();
	void GetAvailableVehiclePoolIndices(TArray<uint8>& OutAvailableVehiclePoolIndices) const;
};
