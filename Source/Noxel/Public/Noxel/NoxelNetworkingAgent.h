//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "NoxelLibrary.h"
#include "NoxelDataAsset.h"
#include "Engine/DataTable.h"

#include "NodesContainer.h"
#include "NoxelContainer.h"
#include "CraftDataHandler.h"
#include "EditorCommandQueue.h"
#include "Voxel/VoxelComponent.h"

#include "Connectors/ConnectorBase.h"

#include "Components/ActorComponent.h"
#include "NoxelNetworkingAgent.generated.h"

#define NOXELEDITORQUEUEBUFFERLENGTH 256

UENUM()
enum class EVoxelOperation : uint8
{
	Add 				UMETA(DisplayName = "Add Cube"),
	Remove 				UMETA(DisplayName = "Remove Cube")
};

UENUM()
enum class EObjectOperation : uint8
{
	Add 				UMETA(DisplayName = "Add Object"),
	Move 				UMETA(DisplayName = "Move Object"),
	Remove				UMETA(DisplayName = "Remove Object"),
	Resign				UMETA(DisplayName = "Resign ownership")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectPermissionDelegate, AActor*, Actor);

UCLASS(ClassGroup = "Noxel", meta=(BlueprintSpawnableComponent) )
class NOXEL_API UNoxelNetworkingAgent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UNoxelNetworkingAgent();

	AActor* OwnedActor;
	UDataTable* DataTable;
	UCraftDataHandler* Craft;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	int32 CurrentCallbackIdx;
	TMap<int32, FObjectPermissionDelegate> ObjectCallbacks;
	TMap<int32, int32> ObjectsWaiting; //Link TempObject indices to callbacks indices

	TArray<int32> QueuesWaiting;

	TMap<UNoxelContainer*, TArray<int32>> ReservedPanels;
	TArray<UNoxelContainer*> ReservedWaiting;
public:
	UPROPERTY(ReplicatedUsing= OnRep_TempObjects)
	TArray<AActor*> TempObjects;
private:
	TArray<FEditorQueue*, TInlineAllocator<NOXELEDITORQUEUEBUFFERLENGTH>> QueuesBuffer;
	
	int32 NextQueueIndex = 0;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void AddQueueToBuffer(FEditorQueue* Queue);
	void RemoveQueueFromBuffer(int32 OrderIndex);
	bool GetQueueFromBuffer(int32 OrderIndex, FEditorQueue** Queue);

public:
	TArray<int32> GetReservedPanels(UNoxelContainer* Container);
	void UseReservedPanels(TArray<FPanelID> Used);
	void AddReservedPanels(TArray<FPanelID> Add);

private:
	UFUNCTION()
	void ReservePanels(int32 NumToReserve, UNoxelContainer* Container);
	UFUNCTION(Server, WithValidation, Reliable)
	void ReservePanelsServer(int32 NumToReserve, UNoxelContainer* Container);
	UFUNCTION(Client, Reliable)
	void ReservePanelsClient(UNoxelContainer* Container, const TArray<int32> &Reserved);

public:
	//Create a queue with a unique ID
	FEditorQueue* CreateEditorQueue();
	//client executes the queue, if valid sends it 
	void SendCommandQueue(FEditorQueue* Queue);

private:
	//Send to server to check
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReceiveCommandQueue(FEditorQueueNetworkable Networkable);

	//Replicate to other players
	UFUNCTION(NetMulticast, Reliable)
	void ClientsReceiveCommandQueue(FEditorQueueNetworkable Networkable);

	//Client was wrong, should undo
	UFUNCTION(Client, Reliable)
	void ClientRectifyCommandQueue(int32 IndexToRectify, bool ShouldExecute);
	
public:

	//UFUNCTION(BlueprintCallable)
		void AddTempObject(FObjectPermissionDelegate Callback, TSubclassOf<AActor> Class, FTransform Location);

	//UFUNCTION(BlueprintCallable)
		void MoveTempObject(AActor* Object, FTransform Location);

	//UFUNCTION(BlueprintCallable)
		void RemoveTempObject(AActor* Object);

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void SpawnAndPossessCraft();


private:
	UFUNCTION(Server, Reliable, WithValidation)
	void AddTempObjectServer(TSubclassOf<AActor> Class, FTransform Location, int32 CallbackIdx);
	
	UFUNCTION(Server, Unreliable, WithValidation)
	void MoveTempObjectServer(AActor* Object, FTransform Location);

	UFUNCTION(Server, Reliable, WithValidation)
	void RemoveTempObjectServer(AActor* Object);

	UFUNCTION(NetMulticast, Unreliable)
	void MoveTempObjectClients(AActor* Object, FTransform Location);
	
public:
	UFUNCTION()
	void OnRep_TempObjects();
	
private:
	UFUNCTION(Client, Reliable)
	void NotifyClientCallback(int32 CallbackIndex, int32 TempObjectsIndex);
	//Used to kick clients if they're trying to edit something that isn't their craft
	bool canEdit(AActor* Target);

	UFUNCTION()
	void OnCraftComponentsReplicated();
};
