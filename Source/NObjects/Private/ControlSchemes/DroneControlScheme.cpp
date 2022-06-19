//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "ControlSchemes/DroneControlScheme.h"
#include "NObjects.h"
#include "NObjects/BruteForceSolver.h"

FTRVector UDroneControlScheme::GetUsedForces()
{
	return FTRVector(1,1,1,1,1,1);
}

FTRVector UDroneControlScheme::GetUsedInputs()
{
	return FTRVector(1,1,1,0,0,1);
}

FTRVector UDroneControlScheme::ApplyInputMatrix(FTRVector Input, FTransform Location, FTransform CameraTransform,
                                                FTRVector Speed, FTRVector Acceleration, FTRVector Mass)
{
	FTRVector OutputForce = FTRVector::ZeroVector;
	float scalarUp = Location.GetRotation().GetUpVector() | FVector::UpVector;
	float UpForce = 0;
	if (scalarUp > 0.1)
	{
		UpForce = Mass.Translation.Z / scalarUp * (Input.Translation.Z * 0.5 + 1) * -Acceleration.Translation.Z;
	}
	OutputForce.Translation.Z = UpForce;
	FRotator camerarot = CameraTransform.Rotator(), craftrot = Location.Rotator();
	FRotator CameraToCraft = camerarot.GetInverse() + craftrot;
	float cam2craftplane = CameraToCraft.GetComponentForAxis(EAxis::Type::Z);
	FVector RotVec(-Input.Translation.Y, Input.Translation.X, Input.Rotation.Z - cam2craftplane*0.1);
	FVector RestoringMoment = Location.GetRotation().Euler();
	RestoringMoment.Z = 0;
	RestoringMoment*=0.0;
	OutputForce.Rotation = (RotVec*1 + RestoringMoment) * Mass.Rotation;
	return OutputForce;
}
