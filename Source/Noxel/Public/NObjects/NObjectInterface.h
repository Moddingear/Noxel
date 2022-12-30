//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "NObjectInterface.generated.h"

class ANoxelPart;
class UCraftDataHandler;

USTRUCT(BlueprintType)
struct FNoxelReplicatedAttachmentData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FTransform AttachOffset;
	UPROPERTY(BlueprintReadWrite)
	USceneComponent* ParentComponent = nullptr;
	UPROPERTY(BlueprintReadWrite)
	bool valid = false;
};
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

	//Used to set replicated data used for clients when they attach the components to the noxel
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void SetReplicatedAttachmentData(FNoxelReplicatedAttachmentData data);
	virtual void SetReplicatedAttachmentData_Implementation(FNoxelReplicatedAttachmentData data);

	//As a client, check if the component has received valid attachment data yet. May be used to enable noxel refreshing on first load
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		bool IsAttachedAtFinalLocation();
	virtual bool IsAttachedAtFinalLocation_Implementation();

	void SetupNodeContainerBySocket(class UStaticMeshComponent* Mesh, FString SocketRegex, class UNodesContainer* Target);

	//Everything is in world space
	bool ComputeCOMFromComponents(TArray<AActor*> &Actors, FVector &COM, float &Mass, FVector& InertiaTensor);
protected:
	bool IsAttachmentValid() const;
	void CheckNetworkAttachment(FString CallContext) const;
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