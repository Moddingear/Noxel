//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovementNObject.h"
#include "Connectors/ForceConnector.h"
#include "Wheel.generated.h"

/**
 * 
 */
UCLASS()
class NOBJECTS_API AWheel : public AMovementNObject
{
	GENERATED_BODY()

public:
	AWheel();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadWrite)
	float MaxForce = 50000000.f;

	UPROPERTY(BlueprintReadWrite)
	float MaxSpeed = 100.0f;

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
