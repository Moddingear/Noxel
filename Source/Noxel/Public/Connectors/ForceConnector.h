//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Noxel/NoxelCombatLibrary.h"

#include "Connectors/ConnectorBase.h"
#include "NObjects/BruteForceSolver.h"
#include "ForceConnector.generated.h"

class UForceConnector;
/**
 * 
 */
USTRUCT(BlueprintType)
struct NOXEL_API FTorsor
{
	//ALL LOCATIONS RELATIVE TO EMITTING OBJECT
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector RelativeLocation;

	UPROPERTY(BlueprintReadWrite)
	FVector Force; //R

	UPROPERTY(BlueprintReadWrite)
	FVector Torque; //M

	UPROPERTY(BlueprintReadWrite)
	UForceConnector* Source;

	UPROPERTY(BlueprintReadWrite)
	int Index;

	UPROPERTY(BlueprintReadWrite)
	float RangeMin;

	UPROPERTY(BlueprintReadWrite)
	float RangeMax;

	FTorsor()
		:RelativeLocation(FVector::ZeroVector), Force(FVector::ZeroVector), Torque(FVector::ZeroVector),
		Source(nullptr), Index(INDEX_NONE), RangeMin(0), RangeMax(0)
	{
	}
	;

	FTorsor(FVector ForceIn, FVector TorqueIn, float RangeMinIn, float RangeMaxIn)
		: RelativeLocation(FVector::ZeroVector),
		  Force(ForceIn),
		  Torque(TorqueIn),
		  Source(nullptr), Index(INDEX_NONE),
		  RangeMin(RangeMinIn),
		  RangeMax(RangeMaxIn)
	{
	}

	FTorsor(FVector RelativeLocationIn, FVector ForceIn, FVector TorqueIn, float RangeMinIn, float RangeMaxIn)
		: RelativeLocation(RelativeLocationIn),
		  Force(ForceIn),
		  Torque(TorqueIn),
		  Source(nullptr), Index(INDEX_NONE),
		  RangeMin(RangeMinIn),
		  RangeMax(RangeMaxIn)
	{
	}

	bool IsNearlyZero() const;

	bool IsForceOnly() const;

	bool IsTorqueOnly() const;

	AActor * GetOwner() const;

	FTransform GetOwnerTransform();

	FVector GetOwnerLocation();

	FVector GetTorsorLocationInWorld();

	FVector GetForceInWorld();

	FVector GetTorqueInWorld();

	FVector GetTorsorLocationRelativeTo(FTransform WorldTransform);

	FVector GetForceRelativeTo(FTransform WorldTransform);

	FVector GetTorqueRelativeTo(FTransform WorldTransform);

	bool IsSaturatedWith(float Input) const;

	float ClampToSaturation(float Input) const;

	FORCEINLINE bool operator== (const FTorsor& other) const
	{
		return (RelativeLocation == other.RelativeLocation && Force == other.Force && Torque == other.Torque && Source == other.Source && RangeMin == other.RangeMin && RangeMax == other.RangeMax);
	}

	FTorsor GetTorsorAt(FTransform WorldTransform);

	FForceSource ToForceSource(FTransform WorldLocation);

	/*FTorsor Displace(FVector NewRelativeLocation)
	{
		//M(B) = M(A) + BA ^ R
		FVector NewTorque = Torque + (RelativeLocation.GetLocation() - NewRelativeLocation) ^ Force;
		return FTorsor(FTransform(RelativeLocation.Rotator(), NewRelativeLocation, RelativeLocation.GetScale3D()), Force, NewTorque, Source);
	}

	bool operator< (FTorsor other) //Check to see if the bounds of the force and moment are smaller than the force
	{
		TArray<float> componentsmax = { other.Force.X, other.Force.Y, other.Force.Z,
										other.Torque.X, other.Torque.Y, other.Torque.Z };
		TArray<float> components = { Force.X, Force.Y, Force.Z,
									Torque.X, Torque.Y, Torque.Z };
		for (int8 i = 0; i < componentsmax.Num(); i++)
		{
			if (FMath::Abs(components[i]) >= componentsmax[i])
			{
				return false;
			}
		}
		return true;
	}

	FTorsor operator+ (FTorsor other) // Operator is non-symmetrical, will take the relative location of the lhs
	{
		FTorsor newOther = other.Displace(RelativeLocation);
		return FTorsor(RelativeLocation, Force + other.Force, Torque + other.Torque, Source);
	}

	FTorsor operator* (float other)
	{
		return FTorsor(RelativeLocation, Force * other, Torque * other, Source);
	}*/
};


UCLASS(meta = (BlueprintSpawnableComponent))
class NOXEL_API UForceConnector : public UConnectorBase
{
	GENERATED_BODY()

public:

	UForceConnector();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	

	void SendOrder(TArray<float>& Order, UForceConnector* Target);

	TArray<FTorsor> GetMaxTorsor(UForceConnector* Target);

	virtual bool CanConnect(UConnectorBase* other) override;

	TArray<FTorsor> GetAllTorsors();

	void SendAllOrders(TArray<FTorsor>& Torsors, TArray<float>& Scalars);

	private:
	
	UPROPERTY()
	TArray<float> LastOrder;

	UPROPERTY()
	TArray<FTorsor> MaxTorsors;

	public:
	
	void SetTorsors(TArray<FTorsor>& InMaxTorsors);

	TArray<float> GetLastOrder();

};
