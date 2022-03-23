//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "ControlSchemes/DroneControlScheme.h"
#include "NObjects.h"
#include "NObjects/BruteForceSolver.h"

FTRVector UDroneControlScheme::GetUsedForces()
{
	return FTRVector(0,0,1,1,1,1);
}

FTRVector UDroneControlScheme::GetUsedInputs()
{
	return FTRVector(1,1,1,0,0,1);
}

FTRVector UDroneControlScheme::ApplyInputMatrix(FTRVector Input, FTransform Location, FTransform CameraTransform,
                                                FTRVector Speed, FTRVector Acceleration, FTRVector Mass)
{
	FTRVector OutputForce = FTRVector::ZeroVector;
	float UpForce = Mass.Translation.Z / (Location.GetRotation().GetUpVector() | FVector::UpVector) * (Input.Translation.Z * 0.5 + 1) * -Acceleration.Translation.Z;
	OutputForce.Translation.Z = UpForce;
	FVector RotVec(-Input.Translation.Y, Input.Translation.X, Input.Rotation.Z);
	FVector RestoringMoment = Location.GetRotation().Euler();
	RestoringMoment.Z = 0;
	RestoringMoment*=-10;
	OutputForce.Rotation = (RotVec*1 + RestoringMoment) * Mass.Rotation;
	return OutputForce;
}
