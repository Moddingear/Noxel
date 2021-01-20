//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "Kismet/KismetMathLibrary.h"

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NoxelLibrary.generated.h"


#define NUMLODs 1

#define NODEMESHSCALE 100

#define OBJECTLIBRARY_PATH TEXT("DataTable'/Game/NoxelEditor/NObjects/NObjectsDataTable.NObjectsDataTable'")

/**
 * 
 */
UCLASS()
class NOXEL_API UNoxelLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UNoxelLibrary();

	//static const float ChunkSize;
	static const float AngleTolerance;
	static const float ConnectionTolerance;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Vectors ----------------------------------------------------------------------------------------------------------------------
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * Rounds the components of a vector, forming an int vector
	 */
	UFUNCTION(BlueprintPure, Category = "Noxel|Vectors")
		static FIntVector RoundVector(FVector Location);
	/**
	 * Divides the given location by a constant chunk size then round, giving the int location of the chunk
	 */
	//UFUNCTION(BlueprintPure, Category = "Noxel|Vectors")
	//	static FIntVector LocationToChunk(FVector Location);

	/**
	 * Find the normal of a triangle
	 */
	UFUNCTION(BlueprintPure, Category = "Noxel|Vectors")
		static FVector GetNormal(FVector A, FVector B, FVector C);
	/**
	 * Find the center of mass of a triangle
	 */
	UFUNCTION(BlueprintPure, Category = "Noxel|Vectors")
		static FVector GetCentroid(FVector A, FVector B, FVector C);
	/**
	 * Compute the area of a triangle
	 */
	UFUNCTION(BlueprintPure, Category = "Noxel|Vectors")
		static float getArea(FVector A, FVector B, FVector C);
	/**
	 * Compute the angle formed by the axes projected onto a plane
	 */
	UFUNCTION(BlueprintPure, Category = "Noxel|Vectors")
		static float getAngleOnPlane(FVector Axis_X, FVector Axis_Y, FVector Plane_Base, FVector Plane_Normal, FVector Point);
	/**
	 * Compute the intersection of two planes
	 */
	UFUNCTION(BlueprintPure, Category = "Noxel|Vectors")
		static bool getPlanePlaneIntersection(FVector Plane1Base, FVector Plane1Normal, FVector Plane2Base, FVector Plane2Normal, FVector& IntersectionPoint, FVector& IntersectionVector);
	/**
	 * Compute the intersection of a line vector with a plane
	 */
	UFUNCTION(BlueprintPure, Category = "Noxel|Vectors")
		static bool getLinevecPlaneIntersection(FVector LinePoint, FVector LineVector, FVector PlaneBase, FVector PlaneNormal, FVector& Intersection);
	/**
	 * Compute the closest points on both lines
	 */
	UFUNCTION(BlueprintPure, Category = "Noxel|Vectors")
		static bool getClosestPointOnTwoLines(FVector Line1Point, FVector Line1Direction, FVector Line2Point, FVector Line2Direction, FVector& ClosestPointOnLine1, FVector& ClosestPointOnLine2);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Raycast ----------------------------------------------------------------------------------------------------------------------
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static FCollisionQueryParams getCollisionParameters();
};
