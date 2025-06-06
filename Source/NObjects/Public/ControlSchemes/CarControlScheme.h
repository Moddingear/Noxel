//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ControlScheme.h"
#include "CarControlScheme.generated.h"

/**
 * 
 */
UCLASS()
class NOBJECTS_API UCarControlScheme : public UControlScheme
{
	GENERATED_BODY()
public:

	virtual FTRVector GetUsedForces() override;
	virtual FTRVector GetUsedInputs() override;

	virtual FTRVector ApplyInputMatrix(FTRVector Input, FTransform Location, FTransform CameraTransform, FTRVector Speed, FTRVector Acceleration, FTRVector Mass) override;
};
