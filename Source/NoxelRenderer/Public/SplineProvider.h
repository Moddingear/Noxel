// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "Components/SplineComponent.h"
#include "SplineProvider.generated.h"

USTRUCT(BlueprintType)
struct FSplineSegment
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int SplineToUse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EndKey;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MeshIndex;

	TArray<FVector> GetInterpolationPoints(const FSplineCurves* Curve);

	TArray<FBoxSphereBounds> ScaleBounds(FBoxSphereBounds InBounds, const FSplineCurves* Curve);

	FQuat TransformRotation(float alpha, const FSplineCurves* Curve) const;

	FVector TransformLocation(FVector InPosition, float alpha, const FSplineCurves* Curve) const;
};

UCLASS(HideCategories = Object, BlueprintType)
class NOXELRENDERER_API USplineProvider : public URuntimeMeshProvider
{
	GENERATED_BODY()

private:
	mutable FRWLock PropertySyncRoot;

	UPROPERTY(VisibleAnywhere)
	TArray<FSplineSegment> Segments;

	UPROPERTY(VisibleAnywhere)
	TArray<FSplineCurves> Curves;

	UPROPERTY(VisibleAnywhere)
	TArray<UStaticMesh*> Meshes;
	
	UPROPERTY()
	bool bIsCacheValid;
	UPROPERTY()
	bool bIsInitialised;
	//Per mesh, per lod, per section
	TArray<TArray<TArray<FRuntimeMeshRenderableMeshData>>> MeshDataCache; 
	TArray<FRuntimeMeshCollisionData> CollisionDataCache;
	TArray<FRuntimeMeshCollisionSettings> CollisionSettingsCache;
	//per mesh, per lod, per section, section index to use
	TArray<TArray<TArray<int32>>> SectionMapCache;
	TArray<FBoxSphereBounds> BoundsCache;

public:
	USplineProvider();

	UFUNCTION(BlueprintPure)
	TArray<FSplineSegment> GetSegments() const;
	UFUNCTION(BlueprintCallable)
	void SetSegments(TArray<FSplineSegment> InSegments);

	UFUNCTION()
	TArray<FSplineCurves> GetSplines() const;
	//UFUNCTION()
	void SetSplines(TArray<FSplineCurves> InCurves);
	UFUNCTION(BlueprintCallable)
	void SetSplines(TArray<USplineComponent*> InCopyFrom);
	
	UFUNCTION(BlueprintPure)
	TArray<UStaticMesh*> GetMeshes() const;
	UFUNCTION(BlueprintCallable)
	void SetMeshes(TArray<UStaticMesh*> InMeshes);

protected:
	virtual void Initialize() override;
	virtual bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	virtual FBoxSphereBounds GetBounds() override;
	virtual bool IsThreadSafe() override;

	virtual bool HasCollisionMesh() override;

	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;

private:
	void RecreateMeshCache();
};
