//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "ControlScheme.h"
#include "NObjects.h"

NOBJECTS_API const FTRVector FTRVector::ZeroVector(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
NOBJECTS_API const FTRVector FTRVector::OneVector(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

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

FTRVector UControlScheme::ApplyInputMatrix(FTRVector Input, FTransform Location, FTRVector Speed, FTRVector Acceleration, FTRVector Mass)
{
	return Input;
}