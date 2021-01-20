//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/NoxelLibrary.h"
#include "Noxel.h"

#include "FunctionLibrary.h"

#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"

//const float UNoxelLibrary::ChunkSize = 1000.0f;
const float UNoxelLibrary::AngleTolerance = 45.0f;
const float UNoxelLibrary::ConnectionTolerance = -0.9f;

#define NOXEL_DEBUG_RENDERING

UNoxelLibrary::UNoxelLibrary() {
}

///// FVectors

/**
* Rounds the components of a vector, forming an int vector
*/
FIntVector UNoxelLibrary::RoundVector(FVector Location)
{
	return FIntVector(FMath::RoundToInt(Location.X), FMath::RoundToInt(Location.Y), FMath::RoundToInt(Location.Z));
}

/**
* Divides the given location by a constant chunk size then round, giving the int location of the chunk
*/
/*FIntVector UNoxelLibrary::LocationToChunk(FVector Location)
{
	return FIntVector(RoundVector(Location/ChunkSize));
}*/


/**
* Find the normal of a triangle
*/
FVector UNoxelLibrary::GetNormal(FVector A, FVector B, FVector C)
{
	FVector norm = FVector::CrossProduct(B - A, C - A);
	norm.Normalize();
	return norm;
}

/**
* Find the center of mass of a triangle
*/
FVector UNoxelLibrary::GetCentroid(FVector A, FVector B, FVector C)
{
	FVector D = (B - A) / 2.0f + A;
	FVector E = (C - B) / 2.0f + B;
	FVector int1, int2;
	UKismetMathLibrary::FindNearestPointsOnLineSegments(C, D, A, E, int1, int2);
	return (int1 + int2) / 2.0f;
}

/**
* Compute the area of a triangle
*/
float UNoxelLibrary::getArea(FVector A, FVector B, FVector C)
{
	FVector dir = A - B;
	float dist = UKismetMathLibrary::GetPointDistanceToLine(C, A, dir);
	return dist * dir.Size() / 2.0f;
}

/**
* Compute the angle formed by the axes projected onto a plane
*/
float UNoxelLibrary::getAngleOnPlane(FVector Axis_X, FVector Axis_Y, FVector Plane_Base, FVector Plane_Normal, FVector Point)
{
	FVector PN = Plane_Normal, AxisX, AxisY, point;
	PN.Normalize();
	AxisX = UKismetMathLibrary::ProjectVectorOnToPlane(Axis_X, PN);
	AxisY = UKismetMathLibrary::ProjectVectorOnToPlane(Axis_Y, PN);
	point = UKismetMathLibrary::ProjectPointOnToPlane(Point, Plane_Base, PN);
	FVector pointdir = point - Plane_Base;
	pointdir.Normalize();
	AxisX.Normalize();
	AxisY.Normalize();
	float x = FVector::DotProduct(AxisX, pointdir);
	float y = FVector::DotProduct(AxisY, pointdir);
	float rawangle = FMath::Atan2(y, x) * 180.0f / PI;
	float angle = rawangle + (rawangle < 0 ? 360.f : 0.f);
	return angle;
}

/**
* Compute the intersection of two planes
*/
bool UNoxelLibrary::getPlanePlaneIntersection(FVector Plane1Base, FVector Plane1Normal, FVector Plane2Base, FVector Plane2Normal, FVector & IntersectionPoint, FVector & IntersectionVector)
{
	Plane1Normal.Normalize();
	Plane2Normal.Normalize();
	FVector lineVec = FVector::CrossProduct(Plane1Normal, Plane2Normal);
	IntersectionVector = lineVec;
	FVector ldir = FVector::CrossProduct(Plane2Normal, lineVec);
	float denominator = FVector::DotProduct(Plane1Normal, ldir);
	if (FMath::Abs(denominator) > 0.006f) 
	{
		FVector plane1toplane2 = Plane1Base - Plane2Base;
		float t = FVector::DotProduct(Plane1Normal, plane1toplane2) / denominator;
		FVector linepoint = t * ldir + Plane2Base;
		IntersectionPoint = linepoint;
		return true;
	}

	return false;
}

/**
* Compute the intersection of a line vector with a plane
*/
bool UNoxelLibrary::getLinevecPlaneIntersection(FVector LinePoint, FVector LineVector, FVector PlaneBase, FVector PlaneNormal, FVector & Intersection)
{
	LineVector.Normalize();
	PlaneNormal.Normalize();
	float dotNumerator = FVector::DotProduct(PlaneBase - LinePoint, PlaneNormal);
	float dotDenominator = FVector::DotProduct(LineVector, PlaneNormal);
	if (dotDenominator != 0.0f)
	{
		float length = dotNumerator / dotDenominator;
		Intersection = LineVector * length + LinePoint;
		return true;
	}
	return false;
}

bool UNoxelLibrary::getClosestPointOnTwoLines(FVector Line1Point, FVector Line1Direction, FVector Line2Point, FVector Line2Direction, FVector & ClosestPointOnLine1, FVector & ClosestPointOnLine2)
{
	Line1Direction.Normalize(); Line2Direction.Normalize();

	float a = Line1Direction | Line1Direction; //dot product;
	float b = Line1Direction | Line2Direction;

	float e = Line2Direction | Line2Direction;

	float d = a * e - b * b;

	if (d != 0.0f) {
		FVector r = Line1Point - Line2Point;
		float c = Line1Direction | r;
		float f = Line2Direction | r;

		float s = (b*f - c * e) / d;
		float t = (a*f - c * b) / d;

		ClosestPointOnLine1 = Line1Point + Line1Direction * s;
		ClosestPointOnLine2 = Line2Point + Line2Direction * t;

		return true;
	}
	return false;
}

/////Raycast

FCollisionQueryParams UNoxelLibrary::getCollisionParameters()
{
	FCollisionQueryParams NoxelCollisionParameters = FCollisionQueryParams();
	NoxelCollisionParameters.bTraceComplex = true;
	NoxelCollisionParameters.bReturnFaceIndex = true;
	//NoxelCollisionParameters.bTraceAsyncScene = true;
	return NoxelCollisionParameters;
}
