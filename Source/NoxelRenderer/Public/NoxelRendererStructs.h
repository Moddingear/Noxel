//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "NoxelRendererStructs.generated.h"

USTRUCT(BlueprintType)
struct FNoxelRendererNodeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector RelativeLocation;
	UPROPERTY(BlueprintReadWrite)
	FColor Color;

	FNoxelRendererNodeData()
		: RelativeLocation(),
		Color()
	{}

	FNoxelRendererNodeData(const FVector InRelativeLocation, const FColor InColor)
		: RelativeLocation(InRelativeLocation),
		Color(InColor)
	{}

	operator FVector&()
	{
		return RelativeLocation;
	}
	operator FVector() const
	{
		return RelativeLocation;
	}
};

struct FNoxelRendererAdjacencyData
{
	FVector PlaneNormal;
	FVector PlanePosition;

	FNoxelRendererAdjacencyData()
		: PlaneNormal(FVector::UpVector),
		PlanePosition(FVector::ZeroVector)
	{}

	FNoxelRendererAdjacencyData(const FVector InPlaneNormal, const FVector InPlanePosition)
		: PlaneNormal(InPlaneNormal),
		PlanePosition(InPlanePosition)
	{}
};

struct FNoxelRendererBakedIntersectionData
{
	TArray<FVector> Intersections;

	FNoxelRendererBakedIntersectionData()
		: Intersections()
	{}

	FNoxelRendererBakedIntersectionData(const TArray<FVector>& InIntersections)
		: Intersections(InIntersections)
	{}
};

USTRUCT(BlueprintType)
struct FNoxelRendererPanelData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 PanelIndex; //Used only for collision
	UPROPERTY(BlueprintReadWrite)
	TArray<int32> Nodes;
	UPROPERTY(BlueprintReadWrite)
	float ThicknessNormal;
	UPROPERTY(BlueprintReadWrite)
	float ThicknessAntiNormal;
	UPROPERTY(BlueprintReadWrite)
	float Area;
	UPROPERTY(BlueprintReadWrite)
	FVector Normal;
	UPROPERTY(BlueprintReadWrite)
	FVector Center;
	UPROPERTY(BlueprintReadWrite)
	TArray<int32> AdjacentPanels; //Contains the indices of the panels it's connected to in the Panels array

	FNoxelRendererPanelData()
		: PanelIndex(),
		Nodes(),
		ThicknessNormal(1.0f),
		ThicknessAntiNormal(1.0f),
		Area(1.0f),
		Normal(FVector::UpVector),
		Center(FVector::ZeroVector),
		AdjacentPanels()
	{}

	FNoxelRendererPanelData(const int32 InPanelIndex, const TArray<int32>& InNodes, const float InThicknessNormal, const float InThicknessAntiNormal, const float InArea, const FVector InNormal, const FVector InCenter, const TArray<int32>& InAdjacentPanels)
		: PanelIndex(InPanelIndex),
		Nodes(InNodes),
		ThicknessNormal(InThicknessNormal),
		ThicknessAntiNormal(InThicknessAntiNormal),
		Area(InArea),
		Normal(InNormal),
		Center(InCenter),
		AdjacentPanels(InAdjacentPanels)
	{}
};