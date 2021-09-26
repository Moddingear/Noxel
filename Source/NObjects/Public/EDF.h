//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovementNObject.h"
#include "Connectors/ForceConnector.h"
#include "EDF.generated.h"

/**
 * 
 */
UCLASS()
class NOBJECTS_API AEDF : public AMovementNObject
{
	GENERATED_BODY()

public:
	AEDF();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly)
	UStaticMeshComponent* BladesTop;
	UPROPERTY(BlueprintReadOnly)
	UStaticMeshComponent* BladesBottom;

	UPROPERTY(BlueprintReadWrite)
	float MaxLift = 50000000.f;
	UPROPERTY(BlueprintReadWrite)
	float MaxTorque = 5000.f;
	UPROPERTY(BlueprintReadWrite)
	float TopPropellerRotationSpeed = -60.f; // in deg/s

	bool ShouldInvertBlades();
	bool BladesInverted = false;
	void CheckBlades();

	TArray<FTorsor> GetMaxTorsor();

	float CurrentThrust = 0.f;

	virtual void OnNObjectEnable_Implementation(UCraftDataHandler* Craft) override;
	virtual void OnNObjectDisable_Implementation() override;

	virtual FJsonObjectWrapper OnReadMetadata_Implementation(const TArray<AActor*>& Components) override;
	virtual bool OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components) override;

	//Force Connector Interface
	UFUNCTION()
	void OnGetReceived();
	UFUNCTION()
	void OnSetReceived();
};
