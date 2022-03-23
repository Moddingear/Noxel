//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "ControlScheme.h"
#include "NObjects.h"
#include "NObjects/BruteForceSolver.h"

UControlScheme::UControlScheme()
{
}

UControlScheme::~UControlScheme()
{
}
AActor * UControlScheme::getOwnerActor()
{
	return Cast<AActor>(GetOuter());
}

FTRVector UControlScheme::GetUsedForces()
{
	return FTRVector::ZeroVector;
}

FTRVector UControlScheme::GetUsedInputs()
{
	return FTRVector::ZeroVector;
}

FTRVector UControlScheme::ApplyInputMatrix(FTRVector Input, FTransform Location, FTransform CameraTransform, FTRVector Speed, FTRVector Acceleration, FTRVector Mass)
{
	return Input;
}