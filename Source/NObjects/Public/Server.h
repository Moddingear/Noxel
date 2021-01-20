//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NObjects/NObjectPossessableBase.h"
#include "Connectors/ForceConnector.h"
#include "Connectors/GunConnector.h"
#include "Server.generated.h"

/**
 * 
 */
UCLASS()
class NOBJECTS_API AServer : public ANObjectPossessableBase
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AServer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly)
	UForceConnector* ForcesOut;
	UPROPERTY(BlueprintReadOnly)
	UGunConnector* GunsOut;
	UPROPERTY(ReplicatedUsing = OnRep_CraftDataHandler)
	UCraftDataHandler* ReplicatedCraft;
	
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<class UControlScheme> ControlSchemeClass;
	UPROPERTY(BlueprintReadOnly)
	class UControlScheme* ControlScheme;

	virtual void OnNObjectEnable_Implementation(UCraftDataHandler* Craft) override;
	virtual void OnNObjectDisable_Implementation() override;
	virtual FString OnReadMetadata_Implementation() override;
	virtual bool OnWriteMetadata_Implementation(const FString& Metadata) override;

	UFUNCTION()
	void OnRep_CraftDataHandler();
};
