//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture2D.h"
#include "ConnectorBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FConnectorDelegate);

#define CONNECTOR_NAMESPACE "Connectors"

UCLASS( ClassGroup= "Connector"/*, meta=(BlueprintSpawnableComponent)*/)
class NOXEL_API UConnectorBase : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UConnectorBase();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsMale;

	UPROPERTY(BlueprintReadOnly, Replicated)
	TArray<UConnectorBase*> Connected;

	UPROPERTY(BlueprintAssignable)
	FConnectorDelegate OnSetReceivedDelegate;

	UPROPERTY(BlueprintAssignable)
	FConnectorDelegate OnGetReceivedDelegate;

	FString ConnectorID;
	FText ConnectorName;

	UTexture2D* ConnectorTexture;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure)
	virtual bool CanConnect(UConnectorBase* other);

	UFUNCTION(BlueprintPure)
	virtual bool CanSend();

	UFUNCTION(BlueprintPure)
	virtual bool CanReceive();

	UFUNCTION(BlueprintCallable)
	virtual bool Connect(UConnectorBase* other);

	UFUNCTION(BlueprintCallable)
	virtual bool Disconnect(UConnectorBase* other);

	UFUNCTION(BlueprintPure)
	static bool CanBothConnect(UConnectorBase* A, UConnectorBase* B);

	UFUNCTION(BlueprintPure)
	static bool AreConnected(UConnectorBase* A, UConnectorBase* B);

protected:
	void TriggerSetReceived()
	{
		if (OnSetReceivedDelegate.IsBound() && CanReceive())
		{
			OnSetReceivedDelegate.Broadcast();
		}
	}

	void TriggerGetReceived()
	{
		if (OnGetReceivedDelegate.IsBound() && CanReceive())
		{
			OnGetReceivedDelegate.Broadcast();
		}
	}
};
