//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Voxel/VoxelComponent.h"
#include "Noxel.h"


UVoxelComponent::UVoxelComponent(const FObjectInitializer& ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>VoxMat(TEXT("Material'/Game/Materials/Voxel.Voxel'"));
	VoxelMaterial = VoxMat.Object;
	SetIsReplicatedByDefault(true);

	VoxelProvider = CreateDefaultSubobject<UVoxelRMCProvider>("VoxelProvider");
	VoxelProvider->SetVoxelMaterial(VoxelMaterial);

}

void UVoxelComponent::OnRegister()
{
	Super::OnRegister();
	Initialize(VoxelProvider);
}

void UVoxelComponent::BeginPlay()
{
	Super::BeginPlay();
}

FIntVector UVoxelComponent::Round(FVector InVector) {
	return FIntVector(FMath::RoundToInt(InVector.X),
		FMath::RoundToInt(InVector.Y),
		FMath::RoundToInt(InVector.Z));
};

void UVoxelComponent::addCube(FIntVector location)
{
	Cubes.AddUnique(location);
	VoxelProvider->SetCubes(Cubes);
}

void UVoxelComponent::removeCube(FIntVector location)
{
	Cubes.Remove(location);
	VoxelProvider->SetCubes(Cubes);
}

bool UVoxelComponent::trace(FVector start, FVector end, FIntVector& cube_hit, FIntVector& hit_normal)
{
	/*FHitResult hit;
	FCollisionQueryParams Params = FCollisionQueryParams();
	//Params.bTraceComplex = true;
	if (LineTraceComponent(hit, start, end, Params)) {
		FTransform world = GetComponentTransform();
		FVector locrel = world.InverseTransformPosition(hit.Location), dirrel = world.InverseTransformVector(hit.Normal);
		hit_normal = Round(dirrel); //Equals rounding the dir
		cube_hit = Round((locrel / CubeRadius) //Position of the hit inside the voxel
			- dirrel / 2.0f); //Go into the cube
		return true;
	}*/
	return false;
}

FVector UVoxelComponent::voxelToWorld(FIntVector cube)
{
	return GetComponentTransform().TransformPosition((FVector)cube*CubeRadius);
}

FIntVector UVoxelComponent::worldToVoxel(FVector location)
{
	return Round(GetComponentTransform().InverseTransformPosition(location) / CubeRadius);
}
