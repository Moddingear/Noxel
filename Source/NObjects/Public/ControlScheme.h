//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Connectors/ForceConnector.h"
#include "ControlScheme.generated.h"

USTRUCT(BlueprintType)
struct FTRVector
{
	GENERATED_BODY()

	static NOBJECTS_API const FTRVector ZeroVector;
	static NOBJECTS_API const FTRVector OneVector;
	
	UPROPERTY(BlueprintReadWrite)
	FVector Translation;
	UPROPERTY(BlueprintReadWrite)
	FVector Rotation;

	FTRVector()
	{}

	FTRVector(FVector InTranslation, FVector InRotation)
		:Translation(InTranslation), Rotation(InRotation)
	{}

	FTRVector(float tx, float ty, float tz, float rx, float ry, float rz)
		:Translation(tx, ty, tz), Rotation( rx, ry, rz)
	{}

	FTRVector operator+(const FTRVector& rhs) const
	{
		return FTRVector(Translation+rhs.Translation, Rotation+rhs.Rotation);
	}
	
	FTRVector operator*(const float& rhs) const
	{
		return FTRVector(Translation*rhs, Rotation*rhs);
	}
	
	FTRVector operator*(const FTRVector& rhs) const
	{
		return FTRVector(Translation*rhs.Translation, Rotation*rhs.Rotation);
	}

	float Sum()
	{
		return Translation.X + Translation.Y + Translation.Z + Rotation.X + Rotation.Y + Rotation.Z;
	}
};
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
	 */
	virtual FTRVector ApplyInputMatrix(FTRVector Input, FTransform Location, FTRVector Speed, FTRVector Acceleration, FTRVector Mass);

};
