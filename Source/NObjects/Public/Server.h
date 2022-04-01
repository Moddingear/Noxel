//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ControlScheme.h"
#include "NObjects/NObjectPossessableBase.h"
#include "Server.generated.h"

class UForceConnector;
class UGunConnector;
class UControlScheme;
class UBruteForceSolver;
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

private:
	UPROPERTY()
	UBruteForceSolver* Solver;

	TArray<FForceSource> LastTorsors;

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
	TSubclassOf<UControlScheme> ControlSchemeClass;
	UPROPERTY(BlueprintReadOnly)
	UControlScheme* ControlScheme;

	virtual void OnNObjectEnable_Implementation(UCraftDataHandler* Craft) override;
	virtual void OnNObjectDisable_Implementation() override;
	virtual FJsonObjectWrapper OnReadMetadata_Implementation(const TArray<AActor*>& Components) override;
	virtual bool OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components) override;

	UFUNCTION()
	void OnRep_CraftDataHandler();
};
