//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "NoxelRendererStructs.h"
#include "NoxelRMCProvider.generated.h"

UCLASS(HideCategories = Object, BlueprintType)
class NOXELRENDERER_API UNoxelRMCProvider : public URuntimeMeshProvider
{
	GENERATED_BODY()
private:
	mutable FCriticalSection PropertySyncRoot;
	TArray<FVector> Nodes;

	TArray<FNoxelRendererPanelData> Panels;

	UMaterialInterface* NoxelMaterial;

	mutable FRWLock CacheSyncRoot;
	//Should cache be rebuilt ?
	bool bIsCacheDirty;
	//Cached intersection data, needed for LODs 0 and 1
	TArray<FNoxelRendererBakedIntersectionData> CachedIntersectionData;

	//Links the face index to the PanelIndex
	TArray<int32> CollisionMap;
	//CollisionMap waiting for collision cooking to finish
	TArray<int32> NewCollisionMap;

public:
	UFUNCTION(BlueprintPure)
	TArray<FVector> GetNodes() const;
	UFUNCTION(BlueprintCallable)
	void SetNodes(UPARAM(ref) const TArray<FVector>& InNodes);

	UFUNCTION(BlueprintPure)
	TArray<FNoxelRendererPanelData> GetPanels() const;
	UFUNCTION(BlueprintCallable)
	void SetPanels(UPARAM(ref) const TArray<FNoxelRendererPanelData>& InPanels);

	UFUNCTION(BlueprintPure)
	UMaterialInterface* GetNoxelMaterial() const;
	UFUNCTION(BlueprintCallable)
	void SetNoxelMaterial(UMaterialInterface* InNoxelMaterial);

	UFUNCTION(BlueprintPure)
	bool GetPanelIndexHit(int32 HitTriangleIndex, int32& OutPanelIndex) const;

	UFUNCTION(BlueprintCallable)
	static bool Intersection3Planes(FVector Plane1Normal, FVector Plane1Point, FVector Plane2Normal, FVector Plane2Point, FVector Plane3Normal, FVector Plane3Point, FVector& OutIntersection);

	static bool Intersection3Planes(FVector Plane1Normal, FVector Plane1Point, FNoxelRendererAdjacencyData Plane2, FNoxelRendererAdjacencyData Plane3, FVector& OutIntersection);

	UFUNCTION(BlueprintCallable)
	//PlaneLocation is the centroid
	static bool PlaneFit(UPARAM(ref) const TArray<FVector>& Points, FVector& OutPlaneLocation, FVector& OutPlaneNormal);

	UFUNCTION(BlueprintCallable)
	//Fit a box around the panel, with all points inside the box, of thickness greater or equal to that of the sides
	FRuntimeMeshCollisionBox BoxFit(UPARAM(ref) const FNoxelRendererPanelData& Panel, UPARAM(ref) const TArray<FVector>& TempNodes);

	//OutNewIndex contains at position [i] the index of the node that should be at position i
	UFUNCTION(BlueprintCallable)
	static bool ReorderNodes(UPARAM(ref) const TArray<FVector>& Points, FVector PlaneCentroid, FVector PlaneNormal, TArray<int32>& OutNewIndex);

	UFUNCTION(BlueprintCallable)
	static float ComputeTriangleArea(FVector A, FVector B, FVector C);

	UFUNCTION(BlueprintCallable)
	static float ComputeTriangleFanArea(FVector Center, TArray<FVector> Nodes);

private:
	void GetShapeParams(TArray<FVector>& OutNodes, TArray<FNoxelRendererPanelData>& OutPanels);
	//Mark the cache as needing a rebuild
	void MarkCacheDirty();
	//Returns !bIsCacheDirty
	bool GetCachedData(TArray<FNoxelRendererBakedIntersectionData>& OutIntersectionData);
	//Build the intersection cache, should be called before any mesh generation
	void MakeCacheIfDirty();

	//Used to reserve memory
	//NumTriangle is the amount of triangles, not triangle indices
	void GetNumIndicesPerSide(int32 LODIndex, int32& NumVertices, int32& NumTriangles);

	int32 GetNumSides(TArray<FNoxelRendererPanelData>& InPanels);

	//Makes the mesh for a single panel
	//PanelNodes is an array where Panel.Nodes[i]'s location is stored in PanelNodes[i]
	void MakeMeshForLOD(int32 LODIndex, int32 SectionId, TArray<FVector>& PanelNodes, FNoxelRendererPanelData& Panel, FNoxelRendererBakedIntersectionData& ThisPanelIntersectionData,
		TFunction<void(const FVector& InPosition, const FVector2D& InTexCoord)> AddVertex,
		TFunction<void(const int32 A, const int32 B, const int32 C)> AddTriangle);

protected:
	void Initialize() override;
	FBoxSphereBounds GetBounds() override;
	bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;
	void CollisionUpdateCompleted() override;
	bool IsThreadSafe() override;

};
