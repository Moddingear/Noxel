//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "ControlSchemes/DroneControlScheme.h"
#include "NObjects.h"

FTRVector UDroneControlScheme::GetUsedForces()
{
	return FTRVector(0,0,1,1,1,1);
}

FTRVector UDroneControlScheme::GetUsedInputs()
{
	return FTRVector(1,1,1,0,0,1);
}

FTRVector UDroneControlScheme::ApplyInputMatrix(FTRVector Input, FTransform Location, FTRVector Speed,
                                                FTRVector Acceleration, FTRVector Mass)
{
	FTRVector OutputForce = FTRVector::ZeroVector;
	float UpForce = Mass.Translation.Z / (Location.GetRotation().GetUpVector() | FVector::UpVector) * (Input.Translation.Z * 0.5 + 1) * -Acceleration.Translation.Z;
	OutputForce.Translation.Z = UpForce;
	FVector RotVec(Input.Translation.X, Input.Translation.Y, Input.Rotation.Z);
	OutputForce.Rotation = RotVec * Mass.Rotation;
	return OutputForce;
}
