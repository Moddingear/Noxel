// Fill out your copyright notice in the Description page of Project Settings.


#include "SplineProvider.h"

#include "RuntimeMeshStaticMeshConverter.h"
#include "Modifiers/RuntimeMeshModifierNormals.h"


TArray<FSplineSegment> USplineProvider::GetSegments() const
{
	FReadScopeLock Lock(PropertySyncRoot);
	return Segments;
}

void USplineProvider::SetSegments(TArray<FSplineSegment> InSegments)
{
	FWriteScopeLock Lock(PropertySyncRoot);
	Segments = InSegments;
	MarkAllLODsDirty();
	MarkCollisionDirty();
}

TArray<FSplineCurves> USplineProvider::GetSplines() const
{
	FReadScopeLock Lock(PropertySyncRoot);
	return Curves;
}

void USplineProvider::SetSplines(TArray<FSplineCurves> InCurves)
{
	FWriteScopeLock Lock(PropertySyncRoot);
	Curves = InCurves;
	MarkAllLODsDirty();
	MarkCollisionDirty();
}

void USplineProvider::SetSplines(TArray<USplineComponent*> InCopyFrom)
{
	TArray<FSplineCurves> CurvesTemp;
	CurvesTemp.SetNum(InCopyFrom.Num());
	for (int i = 0; i < InCopyFrom.Num(); ++i)
	{
		if (IsValid(InCopyFrom[i]))
		{
			CurvesTemp[i] = InCopyFrom[i]->SplineCurves;
		}
	}
	
	FWriteScopeLock Lock(PropertySyncRoot);
	Curves = CurvesTemp;
	MarkAllLODsDirty();
	MarkCollisionDirty();
}

TArray<UStaticMesh*> USplineProvider::GetMeshes() const
{
	FReadScopeLock Lock(PropertySyncRoot);
	return Meshes;
}

void USplineProvider::SetMeshes(TArray<UStaticMesh*> InMeshes)
{
	{
		FWriteScopeLock Lock(PropertySyncRoot);
		if(InMeshes == Meshes)
		{
			return;
		}
		bIsCacheValid = false;
		Meshes = InMeshes;
	}
	RecreateMeshCache();
	MarkAllLODsDirty();
	MarkCollisionDirty();
}

TArray<FVector> FSplineSegment::GetInterpolationPoints(const FSplineCurves* Curve)
{
	return {TransformLocation(FVector::ZeroVector, 0, Curve),
		TransformLocation(FVector::ZeroVector, 0.5, Curve),
		TransformLocation(FVector::ZeroVector, 1, Curve)};
}

TArray<FBoxSphereBounds> FSplineSegment::ScaleBounds(FBoxSphereBounds InBounds, const FSplineCurves* Curve)
{
	TArray<FVector> InterpPoints = GetInterpolationPoints(Curve);
	TArray<FBoxSphereBounds> NewBounds;
	for (int i = 0; i < InterpPoints.Num(); ++i)
	{
		float alpha = (float)i / (InterpPoints.Num()-1);
		FBoxSphereBounds bound = InBounds;
		const FVector Scale0 = Curve->Scale.Eval(StartKey, FVector(1.0f));
		float ExpansionStart = Scale0.GetAbsMax();
		const FVector Scale1 = Curve->Scale.Eval(EndKey, FVector(1.0f));
		float ExpansionEnd = Scale1.GetAbsMax();
		
		bound.ExpandBy(FMath::Lerp(ExpansionStart, ExpansionEnd, alpha));
		bound.Origin += FVector(InterpPoints[i]);
		NewBounds.Add(bound);
	}
	return NewBounds;
}

FQuat FSplineSegment::TransformRotation(float alpha, const FSplineCurves* Curve) const
{
	float Key = FMath::Lerp(StartKey, EndKey, alpha);
	FQuat Quat = Curve->Rotation.Eval(Key, FQuat::Identity);
	Quat.Normalize();

	const FVector Direction = Curve->Position.EvalDerivative(Key, FVector::ZeroVector).GetSafeNormal();
	const FVector UpVector = Quat.GetUpVector();

	FQuat Rot = (FRotationMatrix::MakeFromXZ(Direction, UpVector)).ToQuat();
	
	return Rot;
}

FVector FSplineSegment::TransformLocation(FVector InPosition, float alpha, const FSplineCurves* Curve) const
{
	float Key = FMath::Lerp(StartKey, EndKey, alpha);
	FVector Location = Curve->Position.Eval(Key, FVector::ZeroVector);
	FQuat Quat = TransformRotation(alpha, Curve);
	const FVector Scale = Curve->Scale.Eval(Key, FVector(1.0f));
	
	/*const FVector3f Start = FTransform3f(SegmentStart).GetLocation();
	const FVector3f StartTarget = Start + FTransform3f(SegmentStart).GetUnitAxis(EAxis::X) * FTransform3f(SegmentStart).GetScale3D().X;
	const FVector3f End = FTransform3f(SegmentEnd).GetLocation();
	const FVector3f EndTarget = End - FTransform3f(SegmentEnd).GetUnitAxis(EAxis::X) * FTransform3f(SegmentEnd).GetScale3D().X;
	const FVector3f LocationStart = FMath::Lerp(Start, StartTarget, alpha);
	const FVector3f LocationEnd = FMath::Lerp(EndTarget, End, alpha);
	const FVector3f LocationBase = FMath::Lerp(LocationStart, LocationEnd, alpha);
	const FQuat4f rotation = TransformRotation(alpha);*/
	
	InPosition.Y *= Scale.Y;
	InPosition.Z *= Scale.Z;
	InPosition.X = 0;

	return Location + Quat.RotateVector(InPosition);
}

// Sets default values for this component's properties
USplineProvider::USplineProvider()
{
	bIsCacheValid = false;
}

void USplineProvider::Initialize()
{
	bIsCacheValid = false;
	bIsInitialised = true;
	RecreateMeshCache();

	MarkAllLODsDirty();
	MarkCollisionDirty();
}

bool USplineProvider::GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas)
{
	//per mesh, per section, mesh data
	TArray<TArray<FRuntimeMeshRenderableMeshData>> TempMeshDataCache;
	//per mesh, per section, index of section to fill
	TArray<TArray<int32>> TempSectionMapCache;
	TArray<FSplineSegment> TempSegments;
	TArray<FBoxSphereBounds> TempBounds;
	TArray<FSplineCurves> TempCurves;
	{
		FReadScopeLock Lock(PropertySyncRoot);
		int32 NumMeshes = MeshDataCache.Num();
		if (NumMeshes == 0 || Segments.Num() == 0 || Curves.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[USplineProvider::GetAllSectionsMeshForLOD]Skipped rendering, data missing"));
			return false;
		}
		TempMeshDataCache.SetNum(NumMeshes);
		TempSectionMapCache.SetNum(NumMeshes);
		for (int i = 0; i < NumMeshes; ++i)
		{
			TempMeshDataCache[i] = MeshDataCache[i][LODIndex];
            TempSectionMapCache[i] = SectionMapCache[i][LODIndex];
		}
		TempBounds = BoundsCache;
		TempSegments = Segments;
		TempCurves = Curves;
	}
	for (int SegmentIdx = 0; SegmentIdx < TempSegments.Num(); ++SegmentIdx)
	{
		FSplineSegment& segment = TempSegments[SegmentIdx];
		check(TempCurves.IsValidIndex(segment.SplineToUse));
		FSplineCurves* Curve = &TempCurves[segment.SplineToUse];
		check(TempMeshDataCache.IsValidIndex(segment.MeshIndex));
		TArray<FRuntimeMeshRenderableMeshData>& MeshSections = TempMeshDataCache[segment.MeshIndex];
		TArray<int32>& MeshSectionIndicies = TempSectionMapCache[segment.MeshIndex];
		const int NumSections = MeshSections.Num();
		for (int SectionIdx = 0; SectionIdx < NumSections; ++SectionIdx)
		{
			FRuntimeMeshSectionData& data = MeshDatas[MeshSectionIndicies[SectionIdx]];
			FRuntimeMeshRenderableMeshData& datasource = TempMeshDataCache[segment.MeshIndex][SectionIdx];
			FRuntimeMeshRenderableMeshData& datadest = data.MeshData;
			const int32 vert0 = datadest.Positions.Num();
			//copy mesh data to section
			for (int i = 0; i < datasource.Positions.Num(); ++i)
			{
				FVector basepos = datasource.Positions.GetPosition(i);
				FBoxSphereBounds& Bounds = TempBounds[segment.MeshIndex];
				float alpha = ((basepos.X - Bounds.Origin.X)/Bounds.BoxExtent.X + 1.f)/2.f;
				FQuat rotation = segment.TransformRotation(alpha, Curve);
				FVector LocationTransformed = segment.TransformLocation(basepos, alpha, Curve);
				datadest.Positions.Add(LocationTransformed);
				FVector TangentX, TangentY, TangentZ;
				datasource.Tangents.GetTangents(i, TangentX, TangentY, TangentZ);
				datadest.Tangents.Add(rotation.RotateVector(TangentX), rotation.RotateVector(TangentY),
					rotation.RotateVector(TangentZ));
			}
			for (int i = 0; i < datasource.Colors.Num(); ++i)
			{
				datadest.Colors.Add(datasource.Colors.GetColor(i));
			}
			for (int i = 0; i < datasource.TexCoords.Num(); ++i)
			{
				datadest.TexCoords.Add(datasource.TexCoords.GetTexCoord(i));
			}
			for (int i = 0; i < datasource.Triangles.Num(); ++i)
			{
				datadest.Triangles.Add(datasource.Triangles.GetVertexIndex(i) + vert0);
			}
		}
	}
	
	return true;
}

FRuntimeMeshCollisionSettings USplineProvider::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = false;
	Settings.bUseComplexAsSimple = true;

	return Settings;
}

FBoxSphereBounds USplineProvider::GetBounds()
{
	TArray<FSplineSegment> TempSegments;
	TArray<FBoxSphereBounds> TempBounds;
	TArray<FSplineCurves> TempCurves;
	{
		FReadScopeLock Lock(PropertySyncRoot);
		TempBounds = BoundsCache;
		TempSegments = Segments;
		TempCurves = Curves;
	}
	TArray<FVector> Extremas;
	for (int SegmentIdx = 0; SegmentIdx < TempSegments.Num(); ++SegmentIdx)
	{
		FSplineSegment& LocalSegment = TempSegments[SegmentIdx];
		FSplineCurves* Curve = &TempCurves[LocalSegment.SplineToUse];
		if (!TempBounds.IsValidIndex(LocalSegment.MeshIndex))
		{
			continue;
		}
		const FBoxSphereBounds& LocalBounds = TempBounds[LocalSegment.MeshIndex];
		TArray<FBoxSphereBounds> InterpPoints = LocalSegment.ScaleBounds(LocalBounds, Curve);
		for (int i = 0; i < InterpPoints.Num(); ++i)
		{
			Extremas.Add(InterpPoints[i].GetBoxExtrema(0));
			Extremas.Add(InterpPoints[i].GetBoxExtrema(1));
		}
	}
	if (Extremas.Num() == 0)
	{
		return FBoxSphereBounds();
	}
	return FBoxSphereBounds(Extremas);
}

bool USplineProvider::IsThreadSafe()
{
	return true;
}

bool USplineProvider::HasCollisionMesh()
{
	FReadScopeLock Lock(PropertySyncRoot);
	for (int i = 0; i < Segments.Num(); ++i)
	{
		if (Meshes.IsValidIndex(Segments[i].MeshIndex))
		{
			if (IsValid(Meshes[Segments[i].MeshIndex]))
			{
				return true;
			}
		}
	}
	return false;
}

bool USplineProvider::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	//per mesh, per section, mesh data
	TArray<FRuntimeMeshCollisionData> TempCollisionDataCache;
	//per mesh, per section, index of section to fill
	TArray<FSplineSegment> TempSegments;
	TArray<FBoxSphereBounds> TempBounds;
	TArray<FSplineCurves> TempCurves;
	{
		FReadScopeLock Lock(PropertySyncRoot);
		TempCollisionDataCache = CollisionDataCache;
		TempSegments = Segments;
		TempBounds = BoundsCache;
		TempCurves = Curves;
	}
	for (int SegmentIdx = 0; SegmentIdx < TempSegments.Num(); ++SegmentIdx)
	{
		FSplineSegment& segment = TempSegments[SegmentIdx];
		FSplineCurves* Curve = &TempCurves[segment.SplineToUse];
		if (!TempCollisionDataCache.IsValidIndex(segment.MeshIndex))
		{
			continue;
		}
		FRuntimeMeshCollisionData& data = TempCollisionDataCache[segment.MeshIndex];
		FRuntimeMeshCollisionData& datasource = data;
		FRuntimeMeshCollisionData& datadest = CollisionData;
		int32 vert0 = datadest.Vertices.Num();
		//copy mesh data to section
		for (int i = 0; i < datasource.Vertices.Num(); ++i)
		{
			FVector basepos = datasource.Vertices.GetPosition(i);
			FBoxSphereBounds& Bounds = TempBounds[segment.MeshIndex];
			float alpha = ((basepos.X - Bounds.Origin.X)/Bounds.BoxExtent.X + 1.f)/2.f;
			FVector LocationTransformed = segment.TransformLocation(basepos, alpha, Curve);
			datadest.Vertices.Add(LocationTransformed);
		}
		datadest.TexCoords.SetNumChannels(FMath::Max(datadest.TexCoords.NumChannels(), datasource.TexCoords.NumChannels()), false);
		for (int i = 0; i < datasource.TexCoords.NumChannels(); ++i)
		{
			for (int j = 0; j < datasource.TexCoords.NumTexCoords(i); ++j)
			{
				datadest.TexCoords.Add(i, datasource.TexCoords.GetTexCoord(i,j));
			}
			
		}
		for (int i = 0; i < datasource.Triangles.Num(); ++i)
		{
			int A,B,C;
			datasource.Triangles.GetTriangleIndices(i, A, B, C);
			datadest.Triangles.Add(A + vert0, B + vert0, C + vert0);
		}
	}
	return true;
}

void USplineProvider::RecreateMeshCache()
{
	{
		FReadScopeLock Lock2(PropertySyncRoot);
		//skip if cache is valid
		if (bIsCacheValid || !bIsInitialised)
		{
			return;
		}
	}
	FWriteScopeLock Lock(PropertySyncRoot);
	int NumAfter = Meshes.Num();
	
	//Allocate space for the meshes
	TArray<TArray<TArray<FRuntimeMeshSectionData>>> NewSectionData;
	TArray<FRuntimeMeshCollisionData> NewCollisionData;
	TArray<FRuntimeMeshCollisionSettings> NewCollisionSettings;
	TArray<TArray<FRuntimeMeshLODProperties>> NewLODProperties;
	TArray<TArray<FStaticMaterial>> NewMaterials;
	TArray<FBoxSphereBounds> NewBounds;
	NewSectionData.SetNum(NumAfter); NewCollisionData.SetNum(NumAfter);
	NewCollisionSettings.SetNum(NumAfter); NewLODProperties.SetNum(NumAfter);
	NewMaterials.SetNum(NumAfter); NewBounds.SetNum(NumAfter);
	
	//Convert all meshes
	for (int i = 0; i < Meshes.Num(); ++i)
	{
		if (!IsValid(Meshes[i]))
		{
			NumAfter--;
			NewSectionData.SetNum(NumAfter); NewCollisionData.SetNum(NumAfter);
			NewCollisionSettings.SetNum(NumAfter); NewLODProperties.SetNum(NumAfter);
			NewMaterials.SetNum(NumAfter); NewBounds.SetNum(NumAfter);
			continue;
		}
		NewBounds[i] = Meshes[i]->GetBounds();
		URuntimeMeshStaticMeshConverter::CopyStaticMeshData(Meshes[i], NewSectionData[i], NewLODProperties[i], NewMaterials[i], 
			NewCollisionData[i], NewCollisionSettings[i]);
	}
	
	//Register materials, make them unique
	TArray<UMaterialInterface*> UniqueMaterials;
	for (int i = 0; i < NewMaterials.Num(); ++i)
	{
		for (int j = 0; j < NewMaterials[i].Num(); ++j)
		{
			if (UniqueMaterials.Find(NewMaterials[i][j].MaterialInterface) == INDEX_NONE)
			{
				int index = UniqueMaterials.Add(NewMaterials[i][j].MaterialInterface);
				SetupMaterialSlot(index, NewMaterials[i][j].MaterialSlotName, NewMaterials[i][j].MaterialInterface);
			}
		}
	}

	TArray<TArray<int32>> NewMaterialMap; //per lod, per section, index of material assigned to section
	TArray<TArray<TArray<int32>>> NewSectionMap; //per mesh, per lod, per section, section index to use
	TArray<TArray<TArray<FRuntimeMeshRenderableMeshData>>> NewMeshData; //per mesh, per lod, per section, section index to use
	TArray<FRuntimeMeshLODProperties> LODSettings;
	NewSectionMap.SetNum(NumAfter);
	NewMeshData.SetNum(NumAfter);
	//Establish LODs and section to material links
	for (int meshidx = 0; meshidx < NumAfter; ++meshidx)
	{
		int32 NumLODSThisMesh = NewSectionData[meshidx].Num();
		NewSectionMap[meshidx].SetNum(NumLODSThisMesh);
		NewMeshData[meshidx].SetNum(NumLODSThisMesh);
		if (NumLODSThisMesh > NewMaterialMap.Num())
		{
			NewMaterialMap.SetNum(NumLODSThisMesh);
			LODSettings.SetNum(NumLODSThisMesh);
		}
		for (int LODidx = 0; LODidx < NumLODSThisMesh; ++LODidx)
		{
			int32 NumSectionsThisLOD = NewSectionData[meshidx][LODidx].Num();
			NewSectionMap[meshidx][LODidx].SetNum(NumSectionsThisLOD);
			NewMeshData[meshidx][LODidx].SetNum(NumSectionsThisLOD);
			for (int Sectionidx = 0; Sectionidx < NumSectionsThisLOD; ++Sectionidx)
			{
				int32 MaterialSlotOrigin = NewSectionData[meshidx][LODidx][Sectionidx].Properties.MaterialSlot;
				UMaterialInterface* Material = NewMaterials[meshidx][MaterialSlotOrigin].MaterialInterface;
				int32 MaterialSlotHere = UniqueMaterials.Find(Material);
				int32 SectionIndexHere = NewMaterialMap[LODidx].Find(MaterialSlotHere);
				if (SectionIndexHere == INDEX_NONE)
				{
					SectionIndexHere = NewMaterialMap[LODidx].Add(MaterialSlotHere);
				}
				NewSectionMap[meshidx][LODidx][Sectionidx] = SectionIndexHere;
				NewMeshData[meshidx][LODidx][Sectionidx] = NewSectionData[meshidx][LODidx][Sectionidx].MeshData;
			}
			FRuntimeMeshLODProperties& SettingsGlobal = LODSettings[LODidx];
			FRuntimeMeshLODProperties& SettingsLocal = NewLODProperties[meshidx][LODidx];
			SettingsGlobal.ScreenSize = FMath::Min(SettingsGlobal.ScreenSize, SettingsLocal.ScreenSize);
		}
	}
	//Setup basic settings for LODs
	for (int LODIndex = 0; LODIndex < LODSettings.Num(); ++LODIndex)
	{
		FRuntimeMeshLODProperties& SettingsGlobal = LODSettings[LODIndex];
		SettingsGlobal.bCanGetAllSectionsAtOnce = true;
		SettingsGlobal.bCanGetSectionsIndependently = false;
	}
	ConfigureLODs(LODSettings);
	//Fill sections
	for (int LODIndex = 0; LODIndex < NewMaterialMap.Num(); ++LODIndex)
	{
		if (SectionMapCache.Num() > LODIndex)
		{
			for (int SectionIndex = 0; SectionIndex < SectionMapCache[LODIndex].Num(); ++SectionIndex)
            {
            	RemoveSection(LODIndex, SectionIndex);
            }
		}
		for (int SectionIndex = 0; SectionIndex < NewMaterialMap[LODIndex].Num(); ++SectionIndex)
		{
			FRuntimeMeshSectionProperties SectionProperties = FRuntimeMeshSectionProperties();
			SectionProperties.bCastsShadow = true;
			SectionProperties.bIsVisible = true;
			SectionProperties.MaterialSlot = NewMaterialMap[LODIndex][SectionIndex];
			SectionProperties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
			SectionProperties.bWants32BitIndices = true;
			CreateSection(LODIndex, SectionIndex, SectionProperties);
		}
	}
	MeshDataCache = NewMeshData;
	CollisionDataCache = NewCollisionData;
	CollisionSettingsCache = NewCollisionSettings;
	SectionMapCache = NewSectionMap;
	BoundsCache = NewBounds;
	bIsCacheValid = true;
}
