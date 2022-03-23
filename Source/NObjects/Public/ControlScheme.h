//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Connectors/ForceConnector.h"
#include "ControlScheme.generated.h"

struct FTRVector;
/**
 * This class convert keyboard inputs and current craft location into wanted forces and torques
 */
UCLASS()
class NOBJECTS_API UControlScheme : public UObject
{
	GENERATED_BODY()
public:
	UControlScheme();
	~UControlScheme();

protected:

	AActor* getOwnerActor();

public:

	/*
	 * Get forces and torques required to run this
	 */
	virtual FTRVector GetUsedForces();
	/*
	 * Get which inputs are used
	 */
	virtual FTRVector GetUsedInputs();
	
	/*
	 * Convert keyboard inputs to forces and torques
	 * Local space (relative to possessed actor)
	 * Input in local space, Location in world space (of the possessed actor)
	 * CameraTransform, speed and acceleration in world space
	 */
	virtual FTRVector ApplyInputMatrix(FTRVector Input, FTransform Location, FTransform CameraTransform, FTRVector Speed, FTRVector Acceleration, FTRVector Mass);

};
