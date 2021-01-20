//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "NoxelRMCProvider.h"
#include "Kismet/KismetMathLibrary.h"
#include "Modifiers/RuntimeMeshModifierNormals.h"
#include "NoxelRenderer.h"

struct AngleIndexPair
{
	float Angle;
	int32 Index;
	AngleIndexPair(const float InAngle, const int32 InIndex)
		: Angle(InAngle),
		Index(InIndex)
	{}

	FORCEINLINE friend bool operator<(const AngleIndexPair& lhs, const AngleIndexPair& rhs)
	{
		return lhs.Angle < rhs.Angle;
	}

	/*operator int32&()
	{
		return Index;
	}
	operator int32() const
	{
		return Index;
	}*/

};

TArray<FVector> UNoxelRMCProvider::GetNodes() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return Nodes;
}

void UNoxelRMCProvider::SetNodes(UPARAM(ref) const TArray<FVector>& InNodes)
{
	MarkCacheDirty();
	FScopeLock Lock(&PropertySyncRoot);
	Nodes = InNodes;
}

TArray<FNoxelRendererPanelData> UNoxelRMCProvider::GetPanels() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return Panels;
}

void UNoxelRMCProvider::SetPanels(UPARAM(ref) const TArray<FNoxelRendererPanelData>& InPanels)
{
	MarkCacheDirty();
	FScopeLock Lock(&PropertySyncRoot);
	Panels = InPanels;
}

UMaterialInterface* UNoxelRMCProvider::GetNoxelMaterial() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return NoxelMaterial;
}

void UNoxelRMCProvider::SetNoxelMaterial(UMaterialInterface* InNoxelMaterial)
{
	FScopeLock Lock(&PropertySyncRoot);
	NoxelMaterial = InNoxelMaterial;
	SetupMaterialSlot(0, FName("Noxel"), NoxelMaterial);
}

bool UNoxelRMCProvider::GetPanelIndexHit(int32 HitTriangleIndex, int32& OutPanelIndex) const
{
	if (CollisionMap.IsValidIndex(HitTriangleIndex))
	{
		OutPanelIndex = CollisionMap[HitTriangleIndex];
		return true;
	}
	else
	{
		OutPanelIndex = INDEX_NONE;
		return false;
	}
}

bool UNoxelRMCProvider::Intersection3Planes(FVector Plane1Normal, FVector Plane1Point, FVector Plane2Normal, FVector Plane2Point, FVector Plane3Normal, FVector Plane3Point, FVector& OutIntersection)
{
	if (abs(Plane1Normal | (Plane2Normal ^ Plane3Normal)) < KINDA_SMALL_NUMBER)
	{
		return false;
	}
	float d1 = Plane1Normal | Plane1Point, d2 = Plane2Normal | Plane2Point, d3 = Plane3Normal | Plane3Point;
	OutIntersection = (d1 * (Plane2Normal ^ Plane3Normal) + d2 * (Plane3Normal ^ Plane1Normal) + d3 * (Plane1Normal ^ Plane2Normal))
		/ (Plane1Normal | (Plane2Normal ^ Plane3Normal));
	return true;
}

bool UNoxelRMCProvider::Intersection3Planes(FVector Plane1Normal, FVector Plane1Point, FNoxelRendererAdjacencyData Plane2, FNoxelRendererAdjacencyData Plane3, FVector& OutIntersection)
{
	return Intersection3Planes(Plane1Normal, Plane1Point, Plane2.PlaneNormal, Plane2.PlanePosition, Plane3.PlaneNormal, Plane3.PlanePosition, OutIntersection);
}

bool UNoxelRMCProvider::PlaneFit(UPARAM(ref) TArray<FVector>& Points, FVector & OutPlaneLocation, FVector & OutPlaneNormal)
{
	//https://www.ilikebigbits.com/2017_09_25_plane_from_points_2.html
	int32 n = Points.Num();
	if (n < 3)
	{
		return false;
	}
	float nd = n;
	FVector sum = FVector::ZeroVector;
	for (FVector Point : Points)
	{
		sum = sum + Point;
	}
	FVector centroid = sum / n;
	float xx = 0, xy = 0, xz = 0, yy = 0, yz = 0, zz = 0;

	float prescaler = 0.0;
	for (FVector Point : Points)
	{
		float distance = (Point - centroid).Size();
		prescaler += distance;
	}
	prescaler /= nd;

	for (FVector Point : Points)
	{
		FVector r = (Point - centroid)/prescaler;
		xx += r.X * r.X;
		xy += r.X * r.Y;
		xz += r.X * r.Z;
		yy += r.Y * r.Y;
		yz += r.Y * r.Z;
		zz += r.Z * r.Z;
	}
	xx /= nd;
	xy /= nd;
	xz /= nd;
	yy /= nd;
	yz /= nd;
	zz /= nd;

	FVector weighted_dir = FVector::ZeroVector;
	//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::PlaneFit] xx = %f, xy = %f, xz = %f, yy = %f, yz = %f, zz = %f"), xx, xy, xz, yy, yz, zz);
	float det_x = yy * zz - yz * yz, det_y = xx * zz - xz * xz, det_z = xx * yy - xy * xy;
	//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::PlaneFit] det_x = %f, det_y = %f, det_z = %f"), det_x, det_y, det_z);
	FVector axis_dir_x(det_x, xz*yz - xy * zz, xy*yz - xz * yy),
		axis_dir_y(xz*yz - xy * zz, det_y, xy*xz - yz * xx),
		axis_dir_z(xy*yz - xz * yy, xy*xz - yz * xx, det_z);
	//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::PlaneFit] axis_dir_x = %s, axis_dir_y = %s, axis_dir_z = %s"), *axis_dir_x.ToString(), *axis_dir_y.ToString(), *axis_dir_z.ToString());
	float weight_x = det_x * det_x, weight_y = det_y * det_y, weight_z = det_z * det_z;
	//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::PlaneFit] weight_x = %f, weight_y = %f, weight_z = %f"), weight_x, weight_y, weight_z);
	if ((weighted_dir | axis_dir_x) < 0.0f)
	{
		weight_x = -weight_x;
	}
	weighted_dir += axis_dir_x * weight_x;
	if ((weighted_dir | axis_dir_y) < 0.0f)
	{
		weight_y = -weight_y;
	}
	weighted_dir += axis_dir_y * weight_y;
	if ((weighted_dir | axis_dir_z) < 0.0f)
	{
		weight_z = -weight_z;
	}
	weighted_dir += axis_dir_z * weight_z;

	OutPlaneNormal = weighted_dir.GetUnsafeNormal();
	OutPlaneLocation = centroid;
	return true;
}

bool UNoxelRMCProvider::ReorderNodes(UPARAM(ref) TArray<FVector>& Points, FVector PlaneCentroid, FVector PlaneNormal, TArray<int32>& OutNewIndex)
{
	if (Points.Num() < 1)
	{
		return false;
	}
	FString GivenPoints;
	FVector Xdir = (UKismetMathLibrary::ProjectPointOnToPlane(Points[0], PlaneCentroid, PlaneNormal) - PlaneCentroid).GetSafeNormal();
	FVector Ydir = (PlaneNormal ^ Xdir).GetSafeNormal();
	TArray<AngleIndexPair> PointsAngles;
	for (int32 PointIdx = 0; PointIdx < Points.Num(); PointIdx++)
	{
		FVector Point = Points[PointIdx];
		FVector Relative = Point - PlaneCentroid;
		float X = Relative | Xdir;
		float Y = Relative | Ydir;
		float angle = FMath::Atan2(Y, X);
		PointsAngles.Emplace(angle, PointIdx);
		UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::ReorderNodes] Point %i at relative location %s has angle %f"), PointIdx, *Relative.ToString(), angle);
	}
	PointsAngles.Sort();
	OutNewIndex.Empty(Points.Num());
	for (int32 PointIdx = 0; PointIdx < Points.Num(); PointIdx++)
	{
		OutNewIndex.Add(PointsAngles[PointIdx].Index);
		UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::ReorderNodes] OutNewIndex[%i] = %i"), PointIdx, OutNewIndex[PointIdx]);
	}
	return true;
}

float UNoxelRMCProvider::ComputeTriangleArea(FVector A, FVector B, FVector C)
{
	return ((B-A)^(C-A)).Size() /2.f;
}

float UNoxelRMCProvider::ComputeTriangleFanArea(FVector Center, TArray<FVector> Nodes)
{
	float area = 0.f;
	int32 NumNodes = Nodes.Num();
	if (NumNodes < 2)
	{
		return 0.0f;
	}
	for (int32 i = 0; i < NumNodes; i++)
	{
		int32 j = (i + 1) % NumNodes;
		area = area + ComputeTriangleArea(Center, Nodes[i], Nodes[j]);
	}
	return area;
}

void UNoxelRMCProvider::GetShapeParams(TArray<FVector>& OutNodes, TArray<FNoxelRendererPanelData>& OutPanels)
{
	FScopeLock Lock(&PropertySyncRoot);
	OutNodes = Nodes;
	OutPanels = Panels;
}

void UNoxelRMCProvider::MarkCacheDirty()
{
	FScopeLock Lock(&CacheSyncRoot);
	bIsCacheDirty = true;
}

bool UNoxelRMCProvider::GetCachedData(TArray<FNoxelRendererBakedIntersectionData>& OutIntersectionData)
{
	FScopeLock Lock(&CacheSyncRoot);
	OutIntersectionData = CachedIntersectionData;
	return !bIsCacheDirty;
}

void UNoxelRMCProvider::MakeCacheIfDirty()
{
	TArray<FNoxelRendererBakedIntersectionData> IntersectionData;
	if (GetCachedData(IntersectionData))
	{
		//Cache is good, nothing to do
		return;
	}

	UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Rebuilding cache"));
	//Past this point, cache is dirty

	TArray<FVector> TempNodes;
	TArray<FNoxelRendererPanelData> TempPanels;
	GetShapeParams(TempNodes, TempPanels);
	int32 NumPanels = TempPanels.Num();

	TArray<FNoxelRendererBakedIntersectionData> AllPanelsIntersections;

	//At this point, nodes should be in the correct order (TODO)
	//AdjacentPanels should be filled
	//Center should be filled
	for (int32 PanelIdx = 0; PanelIdx < NumPanels; PanelIdx++)
	{
	FNoxelRendererPanelData Panel = TempPanels[PanelIdx];
	int32 NumNodes = Panel.Nodes.Num();
		//Compute adjacency
		TArray<FNoxelRendererAdjacencyData> ThisPanelAdjacency; //node0 top then bottom then node1 top then bottom...
		ThisPanelAdjacency.Reserve(NumNodes * 2);
		for (int32 NodeIdx = 0; NodeIdx < NumNodes; NodeIdx++)
		{
			int32 Node = Panel.Nodes[NodeIdx];
			FVector NodePos = TempNodes[Node];
			int32 NextNode = Panel.Nodes[(NodeIdx + 1) % NumNodes];
			FVector NextNodePos = TempNodes[NextNode];
			FVector SideDirection = NextNodePos - NodePos;
			FVector Normal = ((NodePos - Panel.Center) ^ (NextNodePos - Panel.Center)).GetSafeNormal();
			FVector Zdir = SideDirection.GetSafeNormal();
			FVector Xdir = (UKismetMathLibrary::ProjectPointOnToPlane(Panel.Center, NodePos, Zdir) - NodePos).GetSafeNormal();
			FVector Ydir = Zdir ^ Xdir;
			if ((Ydir | Panel.Normal) < 0.f)
			{
				UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Flipping Y coord for side %i of panel number %i"), NodeIdx, PanelIdx);
				Ydir = -Ydir;
			}
			TArray<AngleIndexPair> PanelCandidates; //Index is panel index the TempPanels
			for (int32 OtherPanelIdx : Panel.AdjacentPanels)
			{
				FNoxelRendererPanelData OtherPanel = TempPanels[OtherPanelIdx];
				int32 OtherNumNodes = OtherPanel.Nodes.Num();
				int32 OtherPanelNodeIdx = OtherPanel.Nodes.Find(Node);
				if (OtherPanelNodeIdx != INDEX_NONE)
				{
					bool usable = false;
					//If the panels are on the same side, normals will be in the same direction
					//Normals will go clockwise when the side direction is going away from view
					FVector OtherNormal = ((NodePos - OtherPanel.Center) ^ (NextNodePos - OtherPanel.Center)).GetSafeNormal();
					if (OtherPanel.Nodes[(OtherPanelNodeIdx + 1) % OtherNumNodes] == NextNode)
					{
						//Same order
						usable = true;
					}
					if (OtherPanel.Nodes[(OtherPanelNodeIdx - 1 + OtherNumNodes) % OtherNumNodes] == NextNode)
					{
						//Opposite order
						usable = true;
					}
					if (usable)
					{
						FVector relativecenter = OtherPanel.Center - NodePos;
						float X = relativecenter | Xdir;
						float Y = relativecenter | Ydir;
						float angle = FMath::Atan2(Y, -X);
						PanelCandidates.Emplace(angle, OtherPanelIdx);
						UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Other panel at index %i is usable for side %i of panel number %i; angle = %f"), OtherPanelIdx, NodeIdx, PanelIdx, angle);
					}
				}
			}
			if (PanelCandidates.Num() == 0)
			{
				UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Found no adjacent panels for side %i of panel number %i"), NodeIdx, PanelIdx);
				ThisPanelAdjacency.Emplace(Xdir, NodePos);
				ThisPanelAdjacency.Emplace(Xdir, NodePos);
			}
			else
			{
				PanelCandidates.Sort(); //Sorted ascending
				//Keep least and most angle closest to 0 for top and bottom respectively
				//Top should connect with the smallest positive angle, and bottom should connect with the biggest negative angle
				//TArray<int32> PanelIndices = { PanelCandidates[0].Index, PanelCandidates.Last().Index };
				TArray<int32> PanelIndices = { PanelCandidates.Last().Index, PanelCandidates[0].Index };

				UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Found %i adjacent panels for side %i of panel number %i, selecting panels %i for top and %i for bottom"), 
					PanelCandidates.Num(), NodeIdx, PanelIdx, PanelIndices[0], PanelIndices[1]);
				for (int32 PanelIndex : PanelIndices)
				{
					FNoxelRendererPanelData OtherPanel = TempPanels[PanelIndex];
					FVector Bisector = UKismetMathLibrary::ProjectVectorOnToPlane(Panel.Center - NodePos, Zdir).GetSafeNormal() + UKismetMathLibrary::ProjectVectorOnToPlane(OtherPanel.Center - NodePos, Zdir).GetSafeNormal();
					if (Bisector.IsNearlyZero())
					{
						Bisector = Normal;
					}
					FVector IntersectionNormal = (Bisector ^ SideDirection).GetSafeNormal();
					UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Intersection between panels %i and other panel %i; Bisector = %s; SideDirection = %s; IntersectionNormal = %s"), 
						PanelIdx, PanelIndex, *Bisector.GetSafeNormal().ToString(), *SideDirection.GetSafeNormal().ToString(), *IntersectionNormal.ToString());
					ThisPanelAdjacency.Emplace(IntersectionNormal, NodePos);
				}
			}
		}

		TArray<FVector> ThisPanelIntersections;
		//Compute Intersections
		for (int32 NodeIdx = 0; NodeIdx < NumNodes; NodeIdx++)
		{
			int32 Node = Panel.Nodes[NodeIdx];
			FVector NodePos = TempNodes[Node];
			int32 NextNodeIdx = (NodeIdx + 1) % NumNodes;
			int32 NextNode = Panel.Nodes[NextNodeIdx];
			FVector NextNodePos = TempNodes[NextNode];
			FVector SideDirection = NextNodePos - NodePos;
			FVector Normal = Panel.Normal; //((NodePos - Panel.Center) ^ (NextNodePos - Panel.Center)).GetSafeNormal();//wrong normal used
			FNoxelRendererAdjacencyData PlaneTop = ThisPanelAdjacency[2 * NodeIdx],
				PlaneTopNext = ThisPanelAdjacency[2 * NextNodeIdx],
				PlaneBottom = ThisPanelAdjacency[2 * NodeIdx + 1],
				PlaneBottomNext = ThisPanelAdjacency[2 * NextNodeIdx + 1];
			FVector IntersectionTop, IntersectionBottom;
			if (Intersection3Planes(Normal, NextNodePos + Normal * Panel.ThicknessNormal, PlaneTop, PlaneTopNext, IntersectionTop))
			{
				//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Intersection for node %i at normals side happens at location %s"), NextNodeIdx, *IntersectionTop.ToString());
				ThisPanelIntersections.Add(IntersectionTop);
			}
			else
			{
				//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Intersection failed for node %i at normals side"), NextNodeIdx);
				ThisPanelIntersections.Add(NextNodePos + Normal * Panel.ThicknessNormal);
			}
			if (Intersection3Planes(Normal, NextNodePos - Normal * Panel.ThicknessAntiNormal, PlaneBottom, PlaneBottomNext, IntersectionBottom))
			{
				//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Intersection for node %i at antinormals side happens at location %s"), NextNodeIdx, *IntersectionBottom.ToString());
				ThisPanelIntersections.Add(IntersectionBottom);
			}
			else
			{
				//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeCacheIfDirty] Intersection failed for node %i at antinormals side"), NextNodeIdx);
				ThisPanelIntersections.Add(NextNodePos - Normal * Panel.ThicknessAntiNormal);
			}
		}
		AllPanelsIntersections.Emplace(ThisPanelIntersections);
	}

	FScopeLock Lock(&CacheSyncRoot);
	CachedIntersectionData = AllPanelsIntersections;
	bIsCacheDirty = false;
}

void UNoxelRMCProvider::GetNumIndicesPerSide(int32 LODIndex, int32 & NumVertices, int32 & NumTriangles)
{
	switch (LODIndex)
	{
	case 0:
		NumVertices = 12;
		NumTriangles = 6;
		break;
	case 1:
		NumVertices = 10;
		NumTriangles = 4;
		break;
	case 2:
		NumVertices = 4;
		NumTriangles = 2;
		break;
	default:
		NumVertices = 0;
		NumTriangles = 0;
		break;
	}
}

int32 UNoxelRMCProvider::GetNumSides(TArray<FNoxelRendererPanelData>& InPanels)
{
	int32 sum = 0;
	for (FNoxelRendererPanelData Panel : Panels)
	{
		sum = sum + Panel.Nodes.Num();
	}
	return sum;
}

void UNoxelRMCProvider::MakeMeshForLOD(int32 LODIndex, int32 SectionId, TArray<FVector>& PanelNodes, FNoxelRendererPanelData& Panel, FNoxelRendererBakedIntersectionData& ThisPanelIntersectionData,
	TFunction<void(const FVector& InPosition, const FVector2D& InTexCoord)> AddVertex,
	TFunction<void(const int32 A, const int32 B, const int32 C)> AddTriangle)
{
	int32 NumNodes = Panel.Nodes.Num();
	FVector CenterTop = Panel.Center + Panel.Normal * Panel.ThicknessNormal;
	FVector CenterBottom = Panel.Center - Panel.Normal * Panel.ThicknessAntiNormal;
	//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::MakeMeshForLOD] Panel center = %s; Panel normal = %s;"), *Panel.Center.ToString(), *Panel.Normal.ToString());
	int32 NumVertsPerSide, NumTrianglesPerSide;
	GetNumIndicesPerSide(LODIndex, NumVertsPerSide, NumTrianglesPerSide);

	for (int32 i = 0; i < NumNodes; i++)
	{
		int32 NodeIdx = (i + 1) % NumNodes;
		int32 Node = Panel.Nodes[NodeIdx];
		FVector NodePos = PanelNodes[NodeIdx];
		int32 NextNodeIdx = (NodeIdx + 1) % NumNodes;
		int32 NextNode = Panel.Nodes[NextNodeIdx];
		FVector NextNodePos = PanelNodes[NextNodeIdx];

		int32 VertIndex0 = NumVertsPerSide * i; //Offset per edge

		auto AddTriangleInternal = [&](const int32 A, const int32 B, const int32 C)
		{
			AddTriangle(A + VertIndex0, B + VertIndex0, C + VertIndex0);
		};

		AddVertex(CenterTop, FVector2D(0, 0));//0
		AddVertex(CenterBottom, FVector2D(1, 1));//1
		float epsilonup = 0.1f, epsilondown = 0.1f;
		//TODO : conserve angles from center
		if (LODIndex == 0 || LODIndex == 1)
		{
			TArray<FVector> ThisPanelIntersections = ThisPanelIntersectionData.Intersections;
			//Only for the faces
			AddVertex(ThisPanelIntersections[2 * i], FVector2D(1 - epsilonup, 0));//2 Top first
			AddVertex(ThisPanelIntersections[2 * (NodeIdx)], FVector2D(0, 1 - epsilonup));//3 //Top next
			AddVertex(ThisPanelIntersections[2 * i + 1], FVector2D(1, epsilondown));//4 //Bottom first
			AddVertex(ThisPanelIntersections[2 * (NodeIdx)+1], FVector2D(epsilondown, 1));//5 //Bottom next

			//Only for the edge
			AddVertex(ThisPanelIntersections[2 * i], FVector2D(1 - epsilonup, 0));//6 Top first
			AddVertex(ThisPanelIntersections[2 * (NodeIdx)], FVector2D(0, 1 - epsilonup));//7 //Top next
			AddVertex(ThisPanelIntersections[2 * i + 1], FVector2D(1, epsilondown));//8 //Bottom first
			AddVertex(ThisPanelIntersections[2 * (NodeIdx)+1], FVector2D(epsilondown, 1));//9 //Bottom next
		}
		if (LODIndex == 0 || LODIndex == 2)
		{
			AddVertex(NodePos, FVector2D(1 - epsilonup / 2.f, epsilondown / 2.f));//2 or 10
			AddVertex(NextNodePos, FVector2D(epsilonup / 2.f, 1 - epsilondown / 2.f));// 3 or 11
		}
		if (LODIndex == 0)
		{
			AddTriangleInternal(0, 3, 2); //top triangle
			AddTriangleInternal(1, 4, 5); //bottom triangle
			AddTriangleInternal(6, 7, 11); //Side Top Top
			AddTriangleInternal(6, 11, 10); //Side Top bottom
			AddTriangleInternal(10, 11, 9); //Side Bottom top
			AddTriangleInternal(10, 9, 8); //Side Bottom Bottom
		}
		if (LODIndex == 1)
		{
			AddTriangleInternal(0, 3, 2); //top triangle
			AddTriangleInternal(1, 4, 5); //bottom triangle
			AddTriangleInternal(6, 7, 9); //Side Top
			AddTriangleInternal(6, 9, 8); //Side Bottom
		}
		if (LODIndex == 2)
		{
			AddTriangleInternal(0, 3, 2); //top triangle
			AddTriangleInternal(1, 2, 3); //bottom triangle
		}
	}
}

void UNoxelRMCProvider::Initialize()
{
	FRuntimeMeshLODProperties LOD0Properties, LOD1Properties, LOD2Properties;
	LOD0Properties.ScreenSize = 0.1f;
	LOD1Properties.ScreenSize = 0.01f;
	LOD2Properties.ScreenSize = 0.0f;

	ConfigureLODs({ LOD0Properties, LOD1Properties, LOD2Properties });

	SetupMaterialSlot(0, FName("Noxel"), GetNoxelMaterial());

	for (int32 LODIndex = 0; LODIndex < 3; LODIndex++)
	{
		FRuntimeMeshSectionProperties Properties;
		Properties.bCastsShadow = true;
		Properties.bIsVisible = true;
		Properties.MaterialSlot = 0;
		Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
		CreateSection(LODIndex, 0, Properties);
	}
	MarkCacheDirty();
	MarkAllLODsDirty();
	MarkCollisionDirty();
}

FBoxSphereBounds UNoxelRMCProvider::GetBounds()
{
	float MaxThickness = 0.0f;
	for (FNoxelRendererPanelData panel : Panels)
	{
		if (panel.ThicknessNormal > MaxThickness)
		{
			MaxThickness = panel.ThicknessNormal;
		}
		if (panel.ThicknessAntiNormal > MaxThickness)
		{
			MaxThickness = panel.ThicknessAntiNormal;
		}
	}
	FBoxSphereBounds NodesBox(Nodes);
	NodesBox.BoxExtent = NodesBox.BoxExtent + FVector::OneVector * MaxThickness;
	NodesBox.SphereRadius = NodesBox.SphereRadius + MaxThickness;
	return NodesBox;
}

bool UNoxelRMCProvider::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{	// We should only ever be queried for section 0 and lod 0
	check(SectionId == 0 && LODIndex <= 2);
	TArray<FVector> TempNodes;
	TArray<FNoxelRendererPanelData> TempPanels;
	GetShapeParams(TempNodes, TempPanels);
	int32 NumPanels = TempPanels.Num();
	//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::GetSectionMeshForLOD] called with parameters LODIndex = %i and SectionId = %i"), LODIndex, SectionId);
	TArray<FNoxelRendererBakedIntersectionData> AllPanelsIntersections;
	if (LODIndex <= 1)
	{
		//Cached intersections
		MakeCacheIfDirty();
		if (!GetCachedData(AllPanelsIntersections))
		{
			UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::GetSectionMeshForLOD] Getting cached data failed"));
			return false; //something went wrong, cache is still dirty
		}
	}
	int32 NumSides = GetNumSides(TempPanels);
	int32 NumVertsPerSide, NumTrianglesPerSide;
	GetNumIndicesPerSide(LODIndex, NumVertsPerSide, NumTrianglesPerSide);
	int32 NumVerts = NumSides * NumVertsPerSide;
	MeshData.Triangles = FRuntimeMeshTriangleStream(NumVerts > (1 << 16)); //Switch to 32 bit if needed
	MeshData.ReserveVertices(NumVerts);
	MeshData.Triangles.Reserve(NumSides * NumTrianglesPerSide * 3);

	//At this point, nodes should be in the correct order (TODO)
	//AdjacentPanels should be filled
	//Center should be filled
	for (int32 PanelIdx = 0; PanelIdx < NumPanels; PanelIdx++)
	{
		FNoxelRendererPanelData Panel = TempPanels[PanelIdx];
		int32 NumNodes = Panel.Nodes.Num();

		//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::GetSectionMeshForLOD] Now rendering panel number %i, with PanelIndex = %i"), PanelIdx, Panel.PanelIndex);
		//meshing

		int32 VertIndex0 = MeshData.Positions.Num(); //Offset per panel
		auto AddVertex = [&](const FVector& InPosition, const FVector2D& InTexCoord)
		{
			int32 VertIdx = MeshData.Positions.Add(InPosition);
			MeshData.Tangents.Add(FVector::ZeroVector, FVector::ZeroVector);
			MeshData.Colors.Add(FColor::White);
			MeshData.TexCoords.Add(InTexCoord);
			//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::GetSectionMeshForLOD] Adding a vertex at index %i with position %s and UV %s"), VertIdx, *InPosition.ToString(), *InTexCoord.ToString());
		};
		auto AddTriangle = [&](const int32 A, const int32 B, const int32 C)
		{
			//UE_LOG(NoxelRendererLog, Log, TEXT("[UNoxelRMCProvider::GetSectionMeshForLOD] Adding a triangle with indices %i, %i and %i"), A + VertIndex0, B + VertIndex0, C + VertIndex0);
			MeshData.Triangles.AddTriangle(A + VertIndex0, B + VertIndex0, C + VertIndex0);
		};
		TArray<FVector> PanelNodes;
		PanelNodes.Reserve(NumNodes);
		for (int32 NodeIdx : Panel.Nodes)
		{
			PanelNodes.Add(TempNodes[NodeIdx]);
		}
		if (LODIndex <= 1)
		{
			MakeMeshForLOD(LODIndex, SectionId, PanelNodes, Panel, AllPanelsIntersections[PanelIdx], AddVertex, AddTriangle);
		}
		else
		{
			FNoxelRendererBakedIntersectionData Isct;
			MakeMeshForLOD(LODIndex, SectionId, PanelNodes, Panel, Isct, AddVertex, AddTriangle);
		}
	}

	URuntimeMeshModifierNormals::CalculateNormalsTangents(MeshData, false);

	return true;
}

FRuntimeMeshCollisionSettings UNoxelRMCProvider::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = true;
	Settings.bUseComplexAsSimple = false;
	//TODO : Simple collision for the panels (box fit)
	return Settings;
}

bool UNoxelRMCProvider::HasCollisionMesh()
{
	return true;
}

#define COLLISIONMESH_LOD 2

bool UNoxelRMCProvider::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	

	TArray<FVector> TempNodes;
	TArray<FNoxelRendererPanelData> TempPanels;
	GetShapeParams(TempNodes, TempPanels);
	int32 NumPanels = TempPanels.Num();

	int32 NumSides = GetNumSides(TempPanels);
	int32 NumVertsPerSide, NumTrianglesPerSide;
	GetNumIndicesPerSide(COLLISIONMESH_LOD, NumVertsPerSide, NumTrianglesPerSide);
	CollisionData.ReserveVertices(NumSides * NumVertsPerSide);
	int32 NumTriangles = NumSides * NumTrianglesPerSide;
	CollisionData.Triangles.Reserve(NumTriangles);

	TArray<int32> TempCollisionMap;
	TempCollisionMap.Reserve(NumTriangles);

	for (int32 PanelIdx = 0; PanelIdx < NumPanels; PanelIdx++)
	{
		FNoxelRendererPanelData Panel = TempPanels[PanelIdx];
		int32 NumNodes = Panel.Nodes.Num();

		for (int32 i = 0; i < NumNodes; i++)
		{
			TempCollisionMap.Add(Panel.PanelIndex);
		}

		//meshing

		int32 VertIndex0 = CollisionData.Vertices.Num();
		auto AddVertex = [&](const FVector& InPosition, const FVector2D& InTexCoord)
		{
			CollisionData.Vertices.Add(InPosition);
			CollisionData.TexCoords.Add(0, InTexCoord);
		};
		auto AddTriangle = [&](const int32 A, const int32 B, const int32 C)
		{
			CollisionData.Triangles.Add(A + VertIndex0, B + VertIndex0, C + VertIndex0);
		};
		TArray<FVector> PanelNodes;
		PanelNodes.Reserve(NumNodes);
		for (int32 NodeIdx : Panel.Nodes)
		{
			PanelNodes.Add(TempNodes[NodeIdx]);
		}
		FNoxelRendererBakedIntersectionData Isct;
		MakeMeshForLOD(COLLISIONMESH_LOD, 0, PanelNodes, Panel, Isct, AddVertex, AddTriangle);
	}

	// Add the single collision section
	CollisionData.CollisionSources.Emplace(0, CollisionData.Triangles.Num() -1, this, 0, ERuntimeMeshCollisionFaceSourceType::Collision);

	NewCollisionMap = TempCollisionMap;

	return true;
}

void UNoxelRMCProvider::CollisionUpdateCompleted()
{
	CollisionMap = NewCollisionMap;
}

bool UNoxelRMCProvider::IsThreadSafe()
{
	return false;
}


