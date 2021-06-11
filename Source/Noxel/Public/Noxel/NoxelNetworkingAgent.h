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

	TMap<FDateTime, FObjectPermissionDelegate> ObjectCallbacks;
	TMap<int32, FObjectPermissionDelegate> ObjectsWaiting;

	TArray<int32> QueuesWaiting;

	TMap<UNoxelContainer*, TArray<int32>> ReservedPanels;
	TArray<UNoxelContainer*> ReservedWaiting;

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
	UFUNCTION(BlueprintCallable)
		void AddBlock(UVoxelComponent* Container, FIntVector Location);

	UFUNCTION(BlueprintCallable)
		void RemoveBlock(UVoxelComponent* Container, FIntVector Location);

	//UFUNCTION(BlueprintCallable)
		void AddObject(FObjectPermissionDelegate Callback, TSubclassOf<AActor> Class, FTransform Location, bool bNeedsOwnership);

	//UFUNCTION(BlueprintCallable)
		void MoveObject(FObjectPermissionDelegate Callback, FTransform Location, AActor* Object, bool bNeedsOwnership);

	//UFUNCTION(BlueprintCallable)
		void RemoveObject(AActor* Object);

	UFUNCTION(BlueprintCallable)
		void ResignObject();

	UFUNCTION(BlueprintCallable, Server, Unreliable, WithValidation)
	void MoveObjectUnreliable(AActor* Object, FTransform Location);
	void MoveObjectUnreliable_Implementation(AActor* Object, FTransform Location);
	bool MoveObjectUnreliable_Validate(AActor* Object, FTransform Location);

	UFUNCTION(BlueprintCallable)
		void ConnectConnector(UConnectorBase* A, UConnectorBase* B);

	UFUNCTION(BlueprintCallable)
		void DisconnectConnector(UConnectorBase* A, UConnectorBase* B);

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void SpawnAndPossessCraft();
	void SpawnAndPossessCraft_Implementation();
	bool SpawnAndPossessCraft_Validate();


private:

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Voxel(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType);
	void Server_Voxel_Implementation(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType);
	bool Server_Voxel_Validate(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Object(FDateTime RequestTime, EObjectOperation OperationType, TSubclassOf<AActor> Class, AActor* Object, FTransform Location, bool bNeedsOwnership);
	void Server_Object_Implementation(FDateTime RequestTime, EObjectOperation OperationType, TSubclassOf<AActor> Class, AActor* Object, FTransform Location, bool bNeedsOwnership);
	bool Server_Object_Validate(FDateTime RequestTime, EObjectOperation OperationType, TSubclassOf<AActor> Class, AActor* Object, FTransform Location, bool bNeedsOwnership);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Connector(UConnectorBase* A, UConnectorBase* B, bool bIsConnecting);
	void Server_Connector_Implementation(UConnectorBase* A, UConnectorBase* B, bool bIsConnecting);
	bool Server_Connector_Validate(UConnectorBase* A, UConnectorBase* B, bool bIsConnecting);

	UFUNCTION(NetMulticast, Reliable)
	void Clients_Voxel(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType);
	void Clients_Voxel_Implementation(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType);

	UFUNCTION(Client, Reliable)
	void Client_Object(FDateTime RequestTime, EObjectOperation OperationType, AActor* Object, int32 ActorIndex = INDEX_NONE);
	void Client_Object_Implementation(FDateTime RequestTime, EObjectOperation OperationType, AActor* Object, int32 ActorIndex = INDEX_NONE);

	void Voxel(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType);

	//Used to kick clients if they're trying to edit something that isn't their craft
	bool canEdit(AActor* Target);

	UFUNCTION()
	void OnCraftComponentsReplicated();
};
