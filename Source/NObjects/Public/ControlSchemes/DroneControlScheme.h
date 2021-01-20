//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ControlScheme.h"
#include "DroneControlScheme.generated.h"

/**
 * 
 */
UCLASS()
class NOBJECTS_API UDroneControlScheme : public UControlScheme
{
	GENERATED_BODY()
public:
	TArray<FTorsor> ForceTorsors;
	TArray<FTorsor> TorqueTorsors;

	void GetUsableTorsors();

	float AttackAngle = 30.0f; //in deg

	virtual TArray<float> Solve() override;
};
