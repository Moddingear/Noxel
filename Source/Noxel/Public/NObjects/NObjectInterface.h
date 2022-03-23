//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "NObjectInterface.generated.h"

class ANoxelPart;
class UCraftDataHandler;
/**
 * 
 */
UINTERFACE(BlueprintType)
class NOXEL_API UNObjectInterface : public UInterface
{
	GENERATED_BODY()
};

class NOXEL_API INObjectInterface
{
	GENERATED_BODY()

public:

	bool Enabled = false;
	UCraftDataHandler* ParentCraft;

	//Called before enabling physics and attaching
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void OnNObjectEnable(UCraftDataHandler* Craft);
	virtual void OnNObjectEnable_Implementation(UCraftDataHandler* Craft);

	//Called before destroying the actor
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void OnNObjectDisable();
	virtual void OnNObjectDisable_Implementation();

	//Called when attempting to attach an actor to a part
	//Must return true when attach was overriden
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		bool OnNObjectAttach(ANoxelPart* Part);
	virtual bool OnNObjectAttach_Implementation(ANoxelPart* Part);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FJsonObjectWrapper OnReadMetadata(const TArray<AActor*>& Components);
	virtual FJsonObjectWrapper OnReadMetadata_Implementation(const TArray<AActor*>& Components);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		bool OnWriteMetadata(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components);
	virtual bool OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components);

	void SetupNodeContainerBySocket(class UStaticMeshComponent* Mesh, FString SocketRegex, class UNodesContainer* Target);

	//Everything is in world space
	bool ComputeCOMFromComponents(TArray<AActor*> &Actors, FVector &COM, float &Mass, FVector& InertiaTensor);
};

/*
	//Called before enabling physics and attaching
	virtual void OnNObjectEnable_Implementation();

	//Called before destroying the actor
	virtual void OnNObjectDisable_Implementation();

	//Called when attempting to attach an actor to a part
	//Must return true when attach was overriden
	virtual bool OnNObjectAttach_Implementation(ANoxelPart* Part);

	virtual FString OnReadMetadata_Implementation();

	virtual bool OnWriteMetadata_Implementation(const FString& Metadata);
*/