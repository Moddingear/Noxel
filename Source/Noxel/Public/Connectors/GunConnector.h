//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Connectors/ConnectorBase.h"
#include "GunConnector.generated.h"

/**
 * 
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class NOXEL_API UGunConnector : public UConnectorBase
{
	GENERATED_BODY()
	
public:
	UGunConnector();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable)
	void Fire();

	UFUNCTION(BlueprintCallable)
	void GetGunInformation(UGunConnector* Target, float& OutReloadTime, bool& bOutReadyToFire, float& OutReloadDuration);

	UPROPERTY(BlueprintReadWrite)
	float ReloadTime; 
	
	UPROPERTY(BlueprintReadWrite)
	bool bReadyToFire; 

	UPROPERTY(BlueprintReadWrite)
	float ReloadDuration;
};
