//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "NodesRMCProvider.h"
#include "Engine/StaticMesh.h"
#include "RuntimeMeshStaticMeshConverter.h"
#include "NoxelRenderer.h"

UNodesRMCProvider::UNodesRMCProvider()
{
	WantedMeshBounds = FBoxSphereBounds(FVector::ZeroVector, FVector::OneVector * 10, 10);
}

FBoxSphereBounds UNodesRMCProvider::GetWantedMeshBounds() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return WantedMeshBounds;
}

void UNodesRMCProvider::SetWantedMeshBounds(const FBoxSphereBounds InWantedMeshBounds)
{
	FScopeLock Lock(&PropertySyncRoot);
	WantedMeshBounds = InWantedMeshBounds;
	PrepareStaticMesh();
}

UStaticMesh * UNodesRMCProvider::GetStaticMesh() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return StaticMesh;
}

void UNodesRMCProvider::SetStaticMesh(UStaticMesh * InStaticMesh)
{
	FScopeLock Lock(&PropertySyncRoot);
	StaticMesh = InStaticMesh;
	PrepareStaticMesh();
}

TArray<FNoxelRendererNodeData> UNodesRMCProvider::GetNodes() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return Nodes;
}

void UNodesRMCProvider::SetNodes(const TArray<FNoxelRendererNodeData>& InNodes)
{
	FScopeLock Lock(&PropertySyncRoot);
	BakedNodeBounds = FBoxSphereBounds(TArray<FVector>(InNodes), InNodes.Num());
	Nodes = InNodes;
	MarkCollisionDirty();
	MarkSectionDirty(0, 0);
}

UMaterialInterface* UNodesRMCProvider::GetNodesMaterial() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return NodesMaterial;
}

void UNodesRMCProvider::SetNodesMaterial(UMaterialInterface* InNodesMaterial)
{
	FScopeLock Lock(&PropertySyncRoot);
	NodesMaterial = InNodesMaterial;
	SetupMaterialSlot(0, FName("Nodes"), NodesMaterial);
}

bool UNodesRMCProvider::GetHitNodeIndex(int32 faceIndex, int32 & HitNode)
{
	FScopeLock Lock(&PropertySyncRoot);
	HitNode = faceIndex / (StaticMeshCollidable.Triangles.Num());
	UE_LOG(NoxelRendererLog, Log, TEXT("[UNodesRMCProvider::GetHitNodeIndex(%d, %d)] NumTriangles = %d"), faceIndex, HitNode, StaticMeshCollidable.Triangles.Num());
	return Nodes.IsValidIndex(HitNode);
}



void UNodesRMCProvider::Initialize()
{
	FRuntimeMeshLODProperties LODProperties;
	LODProperties.ScreenSize = 0.0f;

	ConfigureLODs({ LODProperties });

	SetupMaterialSlot(0, FName("Nodes"), GetNodesMaterial());

	FRuntimeMeshSectionProperties Properties;
	Properties.bCastsShadow = true;
	Properties.bIsVisible = true;
	Properties.MaterialSlot = 0;
	Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Frequent;
	CreateSection(0, 0, Properties);

	MarkSectionDirty(0, 0);
	MarkCollisionDirty();
}

FBoxSphereBounds UNodesRMCProvider::GetBounds()
{
	FVector minposn = BakedNodeBounds.GetBoxExtrema(0);
	FVector maxposn = BakedNodeBounds.GetBoxExtrema(1);
	FVector minposm = BakedMeshBounds.GetBoxExtrema(0);
	FVector maxposm = BakedMeshBounds.GetBoxExtrema(1);
	return FBoxSphereBounds(FBox(minposn + minposm, maxposn + maxposm));
}

bool UNodesRMCProvider::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{	// We should only ever be queried for section 0 and lod 0
	check(SectionId == 0 && LODIndex == 0);

	FRuntimeMeshRenderableMeshData TempStaticMeshRenderable; TArray<FNoxelRendererNodeData> TempNodes;
	GetShapeMeshParams(TempStaticMeshRenderable, TempNodes);
	const int32 NumNodes = TempNodes.Num();
	const int32 NumVerts = TempStaticMeshRenderable.Positions.Num();
	const int32 NumTris = TempStaticMeshRenderable.Triangles.Num();
	MeshData.Positions.Reserve(NumNodes * NumVerts);
	MeshData.Tangents.Reserve(NumNodes * NumVerts);
	MeshData.Colors.Reserve(NumNodes * NumVerts);
	MeshData.TexCoords.Reserve(NumNodes * NumVerts);
	MeshData.Triangles.Reserve(NumNodes * NumTris);

	for (int32 NodeIdx = 0; NodeIdx < TempNodes.Num(); NodeIdx++)
	{
		FNoxelRendererNodeData Node = TempNodes[NodeIdx];
		FVector Nodepos = Node.RelativeLocation;

		//Copy position and offset by node pos
		for (int32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
		{
			MeshData.Positions.Add(TempStaticMeshRenderable.Positions.GetPosition(VertIdx) + Nodepos);
			MeshData.Colors.Add(Node.Color);
		}
		//Append other buffers
		MeshData.Tangents.Append(TempStaticMeshRenderable.Tangents);
		MeshData.TexCoords.Append(TempStaticMeshRenderable.TexCoords);

		//Copy triangles
		for (int32 TrisIdx = 0; TrisIdx < NumTris; TrisIdx++)
		{
			MeshData.Triangles.Add(TempStaticMeshRenderable.Triangles.GetVertexIndex(TrisIdx) + NodeIdx * NumVerts);
		}
	}
	/*UE_LOG(NoxelRendererLog, Log, TEXT("[UNodesRMCProvider::GetSectionMeshForLOD] Printing"))
	FString Positions;
	for (int i = 0; i < MeshData.Positions.Num(); ++i)
	{
		Positions += FString::Printf(TEXT("[%d]=(%s) | "), i, *MeshData.Positions.GetPosition(i).ToCompactString());
	}
	UE_LOG(NoxelRendererLog, Log, TEXT("Positions (%d) = %s"), MeshData.Positions.Num(), *Positions)
	FString Tangents;
	for (int i = 0; i < MeshData.Tangents.Num(); ++i)
	{
		Tangents += FString::Printf(TEXT("[%d]=(%s / %s) | "), i, *MeshData.Tangents.GetNormal(i).ToCompactString(), *MeshData.Tangents.GetTangent(i).ToCompactString());
	}
	UE_LOG(NoxelRendererLog, Log, TEXT("Tangents (%d) = %s"), MeshData.Tangents.Num(), *Tangents)
	FString Colors;
	for (int i = 0; i < MeshData.Colors.Num(); ++i)
	{
		Colors += FString::Printf(TEXT("[%d]=%s | "), i, *MeshData.Colors.GetColor(i).ToString());
	}
	UE_LOG(NoxelRendererLog, Log, TEXT("Colors (%d) = %s"), MeshData.Colors.Num(), *Colors)
	FString Triangles;
	for (int i = 0; i < MeshData.Triangles.Num(); ++i)
	{
		Triangles += FString::Printf(TEXT("[%d]=%d | "), i, MeshData.Triangles.GetVertexIndex(i));
	}
	UE_LOG(NoxelRendererLog, Log, TEXT("Triangles (%d) = %s"), MeshData.Triangles.Num(), *Triangles)*/
	return true;
}

FRuntimeMeshCollisionSettings UNodesRMCProvider::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = true;
	Settings.bUseComplexAsSimple = false;
	Settings.CookingMode = ERuntimeMeshCollisionCookingMode::CollisionPerformance;

	for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
	{
		for (int32 SphereIdx = 0; SphereIdx < StaticMeshSettings.Spheres.Num(); SphereIdx++)
		{
			FRuntimeMeshCollisionSphere sphere = StaticMeshSettings.Spheres[SphereIdx];
			sphere.Center += Nodes[NodeIdx].RelativeLocation;
			Settings.Spheres.Add(sphere);
		}
	}

	return Settings;
}

bool UNodesRMCProvider::HasCollisionMesh()
{
	return true;
}

bool UNodesRMCProvider::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	FRuntimeMeshCollisionData TempStaticMeshCollidable; TArray<FNoxelRendererNodeData> TempNodes;
	GetShapeCollisionParams(TempStaticMeshCollidable, TempNodes);
	int32 NumNodes = TempNodes.Num();
	int32 NumVerts = TempStaticMeshCollidable.Vertices.Num();
	int32 NumTris = TempStaticMeshCollidable.Triangles.Num();

	CollisionData.Vertices.SetNum(NumNodes * NumVerts);
	CollisionData.Triangles.SetNum(NumNodes * NumTris);

	for (int32 NodeIdx = 0; NodeIdx < TempNodes.Num(); NodeIdx++)
	{
		FNoxelRendererNodeData Node = TempNodes[NodeIdx];
		FVector Nodepos = Node.RelativeLocation;

		//Copy position and offset by node pos
		for (int32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
		{
			CollisionData.Vertices.SetPosition(VertIdx + NumVerts * NodeIdx, TempStaticMeshCollidable.Vertices.GetPosition(VertIdx) + Nodepos);
		}

		//Copy triangles
		for (int32 TrisIdx = 0; TrisIdx < NumTris; TrisIdx++)
		{
			int32 A, B, C;
			TempStaticMeshCollidable.Triangles.GetTriangleIndices(TrisIdx, A, B, C);
			CollisionData.Triangles.SetTriangleIndices(TrisIdx + NodeIdx * NumTris, A + NodeIdx * NumTris * 3, B + NodeIdx * NumTris * 3, C + NodeIdx * NumTris * 3);
		}
	}
	UE_LOG(NoxelRendererLog, Log, TEXT("[UNodesRMCProvider::GetCollisionMesh] Recreated, %d nodes"), NumNodes);

	return true;
}

bool UNodesRMCProvider::IsThreadSafe()
{
	return true;
}

void UNodesRMCProvider::PrepareStaticMesh()
{
	if (!StaticMesh)
	{
		return;
	}
	URuntimeMeshStaticMeshConverter::CopyStaticMeshSectionToRenderableMeshData(StaticMesh, 0, 0, StaticMeshRenderable);
	URuntimeMeshStaticMeshConverter::CopyStaticMeshLODToCollisionData(StaticMesh, 0, StaticMeshCollidable);
	URuntimeMeshStaticMeshConverter::CopyStaticMeshCollisionToCollisionSettings(StaticMesh, StaticMeshSettings);
	FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
	for (int32 VertxIdx = 0; VertxIdx < StaticMeshRenderable.Positions.Num(); VertxIdx++)
	{
		FVector Position = StaticMeshRenderable.Positions.GetPosition(VertxIdx);
		Position -= MeshBounds.Origin;
		Position *= WantedMeshBounds.BoxExtent / MeshBounds.BoxExtent;
		Position += WantedMeshBounds.Origin;
		StaticMeshRenderable.Positions.SetPosition(VertxIdx, Position);
	}
	for (int32 VertxIdx = 0; VertxIdx < StaticMeshRenderable.Tangents.Num(); VertxIdx++)
	{
		FVector Normal = StaticMeshRenderable.Tangents.GetNormal(VertxIdx);
		Normal *= WantedMeshBounds.BoxExtent / MeshBounds.BoxExtent;
		Normal.Normalize();
		FVector Tangent = StaticMeshRenderable.Tangents.GetTangent(VertxIdx);
		Tangent *= WantedMeshBounds.BoxExtent / MeshBounds.BoxExtent;
		Tangent = Tangent - (Tangent | Normal) * Normal;
		Tangent.Normalize();
		StaticMeshRenderable.Tangents.SetNormal(VertxIdx, Normal);
		StaticMeshRenderable.Tangents.SetTangent(VertxIdx, Tangent);
	}
	for (int32 VertxIdx = 0; VertxIdx < StaticMeshCollidable.Vertices.Num(); VertxIdx++)
	{
		FVector Position = StaticMeshCollidable.Vertices.GetPosition(VertxIdx);
		Position -= MeshBounds.Origin;
		Position *= WantedMeshBounds.BoxExtent / MeshBounds.BoxExtent;
		Position += WantedMeshBounds.Origin;
		StaticMeshCollidable.Vertices.SetPosition(VertxIdx, Position);
	}
	for (int32 SphColIdx = 0; SphColIdx < StaticMeshSettings.Spheres.Num(); SphColIdx++)
	{
		FVector Position = StaticMeshSettings.Spheres[SphColIdx].Center;
		Position -= MeshBounds.Origin;
		Position *= WantedMeshBounds.BoxExtent / MeshBounds.BoxExtent;
		Position += WantedMeshBounds.Origin;
		StaticMeshSettings.Spheres[SphColIdx].Center = Position;
		StaticMeshSettings.Spheres[SphColIdx].Radius *= WantedMeshBounds.BoxExtent.Size() / MeshBounds.BoxExtent.Size();
	}
	BakedMeshBounds = WantedMeshBounds;
	MarkCollisionDirty();
	MarkAllLODsDirty();
}

void UNodesRMCProvider::GetShapeMeshParams(FRuntimeMeshRenderableMeshData & OutStaticMeshRenderable, TArray<FNoxelRendererNodeData>& OutNodes)
{
	FScopeLock Lock(&PropertySyncRoot);
	OutStaticMeshRenderable = StaticMeshRenderable;
	OutNodes = Nodes;
}

void UNodesRMCProvider::GetShapeCollisionParams(FRuntimeMeshCollisionData & OutStaticMeshCollidable, TArray<FNoxelRendererNodeData>& OutNodes)
{
	FScopeLock Lock(&PropertySyncRoot);
	OutStaticMeshCollidable = StaticMeshCollidable;
	OutNodes = Nodes;
}


