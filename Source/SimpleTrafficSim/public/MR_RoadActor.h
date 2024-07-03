#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "MR_RoadActor.generated.h"

class USplineComponent;
class UStaticMeshComponent;

class UMR_VehicleSpawningComponent;

UCLASS(ClassGroup = (TrafficSim), Blueprintable, BlueprintType, Abstract)
class SIMPLETRAFFICSIM_API AMR_RoadActor : public AActor
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RoadMesh;

	UPROPERTY(EditAnywhere)
	USplineComponent* SplineComponent;

	UPROPERTY(EditAnywhere)
	UMR_VehicleSpawningComponent* VehicleSpawner;

public:
	AMR_RoadActor();
};