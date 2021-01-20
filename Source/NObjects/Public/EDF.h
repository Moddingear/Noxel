//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NObjects/NObjectSimpleBase.h"
#include "Connectors/ForceConnector.h"
#include "EDF.generated.h"

/**
 * 
 */
UCLASS()
class NOBJECTS_API AEDF : public ANObjectSimpleBase
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
	UPROPERTY(BlueprintReadOnly)
	UForceConnector* ForceIn;

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

	virtual FString OnReadMetadata_Implementation() override;
	virtual bool OnWriteMetadata_Implementation(const FString& Metadata) override;

	//Force Connector Interface
	UFUNCTION()
	void OnGetReceived();
	UFUNCTION()
	void OnSetReceived();
};
