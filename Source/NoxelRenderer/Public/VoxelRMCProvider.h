//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "VoxelRMCProvider.generated.h"

UCLASS(HideCategories = Object, BlueprintType)
class NOXELRENDERER_API UVoxelRMCProvider : public URuntimeMeshProvider
{
	GENERATED_BODY()
private:
	mutable FCriticalSection PropertySyncRoot;

	TArray<FIntVector> Cubes;
	float CubeRadius;

	UMaterialInterface* Material;

public:

	UVoxelRMCProvider();

	TArray<FIntVector> GetCubes() const;
	void SetCubes(const TArray<FIntVector> InCubes);

	float GetCubeRadius() const;
	void SetCubeRadius(const float& InRadius);

	UMaterialInterface* GetVoxelMaterial() const;
	void SetVoxelMaterial(UMaterialInterface* InMaterial);

private:

	void GetMeshData(TArray<FIntVector>& OutCubes, float& OutRadius);


protected:
	void Initialize() override;
	FBoxSphereBounds GetBounds() override;
	bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;
	bool IsThreadSafe() override;

};
