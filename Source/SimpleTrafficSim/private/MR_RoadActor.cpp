#include "MR_RoadActor.h"

#include "Components/SplineComponent.h"

#include "MR_VehicleSpawningComponent.h"

AMR_RoadActor::AMR_RoadActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RoadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoadMesh"));
	RootComponent = RoadMesh;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	VehicleSpawner = CreateDefaultSubobject<UMR_VehicleSpawningComponent>(TEXT("VehicleSpawner"));
	VehicleSpawner->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
}