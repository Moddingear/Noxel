//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Connectors/ForceConnector.h"
#include "ControlScheme.generated.h"

/**
 * 
 */
UCLASS()
class NOBJECTS_API UControlScheme : public UObject
{
	GENERATED_BODY()
public:
	UControlScheme();
	~UControlScheme();

protected:
	FVector Translation;
	FVector Rotation;
	float Mass;
	FVector COM;
	TArray<FTorsor> inputForces;

	AActor* getOwnerActor();

public:
	void SetInputValues(FVector InputTranslation, FVector InputRotation);
	void SetForces(TArray<FTorsor> Forces);
	void SetCOMAndMass(FVector COM, float Mass);

	virtual TArray<float> Solve();

	static TArray<float> VoronoiAngleFill(TArray<FVector>& ForcesLocation);

	static TArray<float> GetSumOnDirection(FVector Direction, FVector Normal, TArray<FVector>& ForcesLocation);

	static TArray<float>* NormalizeArray(TArray<float>& Input);

	static bool CheckSaturation(TArray<FTorsor>& Torsors, TArray<float>& Floats, TArray<bool>& Saturated);
};
