//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshComponent.h"

#include "VoxelRMCProvider.h"

#include "VoxelComponent.generated.h"

/**
 * 
 */

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class NOXEL_API UVoxelComponent : public URuntimeMeshComponent
{
	GENERATED_BODY()

public:

	UVoxelComponent(const FObjectInitializer & ObjectInitializer = FObjectInitializer::Get());

protected:

	void OnRegister() override;

	// Called when the game starts
	virtual void BeginPlay() override;

private:

	UVoxelRMCProvider* VoxelProvider;

	TArray<FIntVector> Cubes;

	FIntVector Round(FVector InVector);

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		UMaterialInterface* VoxelMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float CubeRadius = 10.0f;

	UFUNCTION(BlueprintCallable)
		void addCube(FIntVector location);

	UFUNCTION(BlueprintCallable)
		void removeCube(FIntVector location);

	UFUNCTION(BlueprintCallable)
		bool trace(FVector start, FVector end, FIntVector& cube_hit, FIntVector& hit_normal);

	UFUNCTION(BlueprintCallable)
		FVector voxelToWorld(FIntVector cube);

	UFUNCTION(BlueprintCallable)
		FIntVector worldToVoxel(FVector location);
	
};
