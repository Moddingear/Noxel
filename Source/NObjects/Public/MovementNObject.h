// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NObjects/NObjectSimpleBase.h"
#include "MovementNObject.generated.h"

class UForceConnector;
/**
 * 
 */
UCLASS()
class NOBJECTS_API AMovementNObject : public ANObjectSimpleBase
{
	GENERATED_BODY()
	
public:
	AMovementNObject();
	
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly)
	UForceConnector* ForceIn;

	UPROPERTY(BlueprintReadOnly)
	UPrimitiveComponent* AttachParent;

	virtual FJsonObjectWrapper OnReadMetadata_Implementation(const TArray<AActor*>& Components) override;
	virtual bool OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components) override;

	virtual bool OnNObjectAttach_Implementation(ANoxelPart* Part) override;
	
};
