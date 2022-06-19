//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "ControlSchemes/CarControlScheme.h"
#include "NObjects.h"
#include "NObjects/BruteForceSolver.h"

FTRVector UCarControlScheme::GetUsedForces()
{
	return FTRVector(1,0,0,1,1,1);
}

FTRVector UCarControlScheme::GetUsedInputs()
{
	return FTRVector(1,1,0,0,0,0);
}

FTRVector UCarControlScheme::ApplyInputMatrix(FTRVector Input, FTransform Location, FTransform CameraTransform,
                                                FTRVector Speed, FTRVector Acceleration, FTRVector Mass)
{
	FTRVector OutputForce = FTRVector::ZeroVector;
	OutputForce.Translation.X = -Input.Translation.X * Mass.Translation.X * Acceleration.Translation.Z *10;
	OutputForce.Rotation.Z = Input.Translation.Y * Mass.Rotation.Z * 10;
	return OutputForce;
}
