//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "NoxelRendererStructs.h"
#include "NodesRMCProvider.generated.h"

UCLASS(HideCategories = Object, BlueprintType)
class NOXELRENDERER_API UNodesRMCProvider : public URuntimeMeshProvider
{
	GENERATED_BODY()
private:
	mutable FCriticalSection PropertySyncRoot;

	//Bounds used by the nodes
	FBoxSphereBounds BakedNodeBounds;
	//Bounds used by the node mesh
	FBoxSphereBounds BakedMeshBounds;

	//Bounds wanted for the static mesh (after baking)
	FBoxSphereBounds WantedMeshBounds;

	//Mesh used by the nodes
	UStaticMesh* StaticMesh;

	//Baked renderable data from the static mesh
	FRuntimeMeshRenderableMeshData StaticMeshRenderable;
	//Baked collidable data from the static mesh
	FRuntimeMeshCollisionData StaticMeshCollidable;
	//Baked collision settings data from the static mesh
	FRuntimeMeshCollisionSettings StaticMeshSettings;

	//Array of nodes to be rendered
	TArray<FNoxelRendererNodeData> Nodes;

	//Material used to render the nodes
	UMaterialInterface* NodesMaterial;

public:
	UNodesRMCProvider();

	FBoxSphereBounds GetWantedMeshBounds() const;
	void SetWantedMeshBounds(const FBoxSphereBounds InWantedMeshBounds);

	UStaticMesh* GetStaticMesh() const;
	void SetStaticMesh(UStaticMesh* InStaticMesh);

	TArray<FNoxelRendererNodeData> GetNodes() const;
	void SetNodes(const TArray<FNoxelRendererNodeData>& InNodes);

	UMaterialInterface* GetNodesMaterial() const;
	void SetNodesMaterial(UMaterialInterface* InNodesMaterial);

	bool GetHitNodeIndex(int32 faceIndex, int32& HitNode) const;
	
protected:
	void Initialize() override;
	FBoxSphereBounds GetBounds() override;
	bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;
	bool IsThreadSafe() override;

private:
	void PrepareStaticMesh();

	void GetShapeMeshParams(FRuntimeMeshRenderableMeshData& OutStaticMeshRenderable, TArray<FNoxelRendererNodeData>& OutNodes);

	void GetShapeCollisionParams(FRuntimeMeshCollisionData& OutStaticMeshCollidable, TArray<FNoxelRendererNodeData>& OutNodes);
};
