//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "VoxelRMCProvider.h"
#include "NoxelRenderer.h"
UVoxelRMCProvider::UVoxelRMCProvider()
{
	CubeRadius = 10.0f;
}

TArray<FIntVector> UVoxelRMCProvider::GetCubes() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return Cubes;
}

void UVoxelRMCProvider::SetCubes(const TArray<FIntVector> InCubes)
{
	FScopeLock Lock(&PropertySyncRoot);
	Cubes = InCubes;
	MarkAllLODsDirty();
	MarkCollisionDirty();
}

float UVoxelRMCProvider::GetCubeRadius() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return CubeRadius;
}

void UVoxelRMCProvider::SetCubeRadius(const float& InRadius)
{
	FScopeLock Lock(&PropertySyncRoot);
	CubeRadius = InRadius;

	MarkAllLODsDirty();
}

UMaterialInterface* UVoxelRMCProvider::GetVoxelMaterial() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return Material;
}

void UVoxelRMCProvider::SetVoxelMaterial(UMaterialInterface* InMaterial)
{
	FScopeLock Lock(&PropertySyncRoot);
	Material = InMaterial;
	SetupMaterialSlot(0, FName("Voxel"), Material);
}

void UVoxelRMCProvider::GetMeshData(TArray<FIntVector>& OutCubes, float & OutRadius)
{
	FScopeLock Lock(&PropertySyncRoot);
	OutCubes = Cubes;
	OutRadius = CubeRadius;
}



void UVoxelRMCProvider::Initialize()
{
	FRuntimeMeshLODProperties LODProperties;
	LODProperties.ScreenSize = 0.0f;

	ConfigureLODs({ LODProperties });

	SetupMaterialSlot(0, FName("Voxel"), GetVoxelMaterial());

	FRuntimeMeshSectionProperties Properties;
	Properties.bCastsShadow = true;
	Properties.bIsVisible = true;
	Properties.MaterialSlot = 0;
	Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
	CreateSection(0, 0, Properties);

	MarkAllLODsDirty();

	//UE_LOG(NoxelRendererLog, Log, TEXT("[UVoxelRMCProvider::Initialize] Called"));
}

FBoxSphereBounds UVoxelRMCProvider::GetBounds()
{
	FBox BoundsVoxel = FBox(TArray<FVector>(Cubes));
	return FBoxSphereBounds(FBox(BoundsVoxel.Min * 2 * CubeRadius - FVector::OneVector * CubeRadius, BoundsVoxel.Max * 2 * CubeRadius + FVector::OneVector * CubeRadius));
}

bool UVoxelRMCProvider::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{	// We should only ever be queried for section 0 and lod 0
	check(SectionId == 0 && LODIndex == 0);



	//UE_LOG(NoxelRendererLog, Log, TEXT("[UVoxelRMCProvider::GetSectionMeshForLOD] Called"));

	TArray<FIntVector> TempCubes;
	float TempRadius;
	GetMeshData(TempCubes, TempRadius);
	
	//UE_LOG(NoxelRendererLog, Log, TEXT("[UVoxelRMCProvider::GetSectionMeshForLOD] %i cubes, %f radius"), TempCubes.Num(), TempRadius);

	// Generate verts
	FVector BoxVerts[8];
	BoxVerts[0] = FVector(-TempRadius, TempRadius, TempRadius);
	BoxVerts[1] = FVector(TempRadius, TempRadius, TempRadius);
	BoxVerts[2] = FVector(TempRadius, -TempRadius, TempRadius);
	BoxVerts[3] = FVector(-TempRadius, -TempRadius, TempRadius);

	BoxVerts[4] = FVector(-TempRadius, TempRadius, -TempRadius);
	BoxVerts[5] = FVector(TempRadius, TempRadius, -TempRadius);
	BoxVerts[6] = FVector(TempRadius, -TempRadius, -TempRadius);
	BoxVerts[7] = FVector(-TempRadius, -TempRadius, -TempRadius);

	for (int32 CubeIdx = 0; CubeIdx < TempCubes.Num(); CubeIdx++)
	{
		//UE_LOG(NoxelRendererLog, Log, TEXT("[UVoxelRMCProvider::GetSectionMeshForLOD] Cube Index %i"), CubeIdx);
		FIntVector Cube = TempCubes[CubeIdx];

		auto AddVertex = [&](const FVector& InPosition, const FVector& InTangentX, const FVector& InTangentZ, const FVector2D& InTexCoord)
		{
			MeshData.Positions.Add(InPosition + FVector(Cube) * TempRadius * 2);
			MeshData.Tangents.Add(InTangentZ, InTangentX);
			MeshData.Colors.Add(FColor::White);
			MeshData.TexCoords.Add(InTexCoord);
		};

		auto AddTriangle = [&](const int32 A, const int32 B, const int32 C)
		{
			int32 NumVerts = MeshData.Positions.Num();
			MeshData.Triangles.AddTriangle(A + NumVerts, B + NumVerts, C + NumVerts);
		};

		FVector TangentX, TangentY, TangentZ;

		if (!TempCubes.Contains(Cube + FIntVector(0, 0, 1)))
		{
			// Pos Z
			TangentZ = FVector(0.0f, 0.0f, 1.0f);
			TangentX = FVector(0.0f, -1.0f, 0.0f);
			AddVertex(BoxVerts[0], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
			AddVertex(BoxVerts[1], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
			AddVertex(BoxVerts[2], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
			AddVertex(BoxVerts[3], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
			AddTriangle(-4, -3, -1);
			AddTriangle(-3, -2, -1);
		}

		if (!TempCubes.Contains(Cube + FIntVector(-1, 0, 0)))
		{
			// Neg X
			TangentZ = FVector(-1.0f, 0.0f, 0.0f);
			TangentX = FVector(0.0f, -1.0f, 0.0f);
			AddVertex(BoxVerts[4], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
			AddVertex(BoxVerts[0], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
			AddVertex(BoxVerts[3], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
			AddVertex(BoxVerts[7], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
			AddTriangle(-4, -3, -1);
			AddTriangle(-3, -2, -1);
		}

		if (!TempCubes.Contains(Cube + FIntVector(0, 1, 0)))
		{
			// Pos Y
			TangentZ = FVector(0.0f, 1.0f, 0.0f);
			TangentX = FVector(-1.0f, 0.0f, 0.0f);
			AddVertex(BoxVerts[5], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
			AddVertex(BoxVerts[1], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
			AddVertex(BoxVerts[0], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
			AddVertex(BoxVerts[4], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
			AddTriangle(-4, -3, -1);
			AddTriangle(-3, -2, -1);
		}

		if (!TempCubes.Contains(Cube + FIntVector(1, 0, 0)))
		{
			// Pos X
			TangentZ = FVector(1.0f, 0.0f, 0.0f);
			TangentX = FVector(0.0f, 1.0f, 0.0f);
			AddVertex(BoxVerts[6], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
			AddVertex(BoxVerts[2], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
			AddVertex(BoxVerts[1], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
			AddVertex(BoxVerts[5], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
			AddTriangle(-4, -3, -1);
			AddTriangle(-3, -2, -1);
		}

		if (!TempCubes.Contains(Cube + FIntVector(0, -1, 0)))
		{
			// Neg Y
			TangentZ = FVector(0.0f, -1.0f, 0.0f);
			TangentX = FVector(1.0f, 0.0f, 0.0f);
			AddVertex(BoxVerts[7], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
			AddVertex(BoxVerts[3], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
			AddVertex(BoxVerts[2], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
			AddVertex(BoxVerts[6], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
			AddTriangle(-4, -3, -1);
			AddTriangle(-3, -2, -1);
		}

		if (!TempCubes.Contains(Cube + FIntVector(0, 0, -1)))
		{
			// Neg Z
			TangentZ = FVector(0.0f, 0.0f, -1.0f);
			TangentX = FVector(0.0f, 1.0f, 0.0f);
			AddVertex(BoxVerts[7], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
			AddVertex(BoxVerts[6], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
			AddVertex(BoxVerts[5], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
			AddVertex(BoxVerts[4], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
			AddTriangle(-4, -3, -1);
			AddTriangle(-3, -2, -1);
		}
	}
	

	return true;
}

FRuntimeMeshCollisionSettings UVoxelRMCProvider::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = false;
	Settings.bUseComplexAsSimple = false;
	Settings.Boxes.Reserve(Cubes.Num());

	for (int32 CubeIdx = 0; CubeIdx < Cubes.Num(); CubeIdx++)
	{
		FIntVector Cube = Cubes[CubeIdx];
		FVector Cubelocation = FVector(Cube) * 2 * CubeRadius;
		FRuntimeMeshCollisionBox Box;
		Box.Extents = FVector(CubeRadius*2);
		Box.Center = Cubelocation;
		Box.Rotation = FRotator::ZeroRotator;
		Settings.Boxes.Add(Box);
	}

	return Settings;
}

bool UVoxelRMCProvider::HasCollisionMesh()
{
	return false;
}

bool UVoxelRMCProvider::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	if (Cubes.Num() == 0)
	{
		return false;
	}
	return false;
}

bool UVoxelRMCProvider::IsThreadSafe()
{
	return true;
}


