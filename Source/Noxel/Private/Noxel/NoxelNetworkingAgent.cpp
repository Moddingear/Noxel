//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/NoxelNetworkingAgent.h"

#include "Noxel.h"
#include "EditorGameState.h"

#include "Noxel/CraftDataHandler.h"
#include "NoxelHangarBase.h"
#include "NObjects/NoxelPart.h"

// Sets default values for this component's properties
UNoxelNetworkingAgent::UNoxelNetworkingAgent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1;
	static ConstructorHelpers::FObjectFinder<UDataTable> DataConstructor(OBJECTLIBRARY_PATH);
	if (DataConstructor.Succeeded()) {
		DataTable = DataConstructor.Object;
	}
	// ...
}


// Called when the game starts
void UNoxelNetworkingAgent::BeginPlay()
{
	Super::BeginPlay();
	AEditorGameState* gs = (AEditorGameState*)GetWorld()->GetGameState();
	if (gs)
	{
		ANoxelHangarBase* hangar = gs->GetHangar();
		if (hangar)
		{
			Craft = hangar->GetCraftDataHandler();
			UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::BeginPlay] Craft found"));
			Craft->OnComponentsReplicatedEvent.AddDynamic(this, &UNoxelNetworkingAgent::OnCraftComponentsReplicated);
		}
		else {
			UE_LOG(NoxelDataNetwork, Error, TEXT("[UNoxelNetworkingAgent::BeginPlay] Hangar pointer broken, please fix in Game State"));
		}
	}
	else
	{
		UE_LOG(NoxelDataNetwork, Error, TEXT("[UNoxelNetworkingAgent::BeginPlay] Game State is bad, should be a AEditorGameState, plz fix :)"));
	}
}


// Called every frame
void UNoxelNetworkingAgent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (ANoxelPart* Part : Craft->GetParts()) //Refill reserved panels
	{
		if (!Part)
		{
			continue;
		}
		UNoxelContainer* Container = Part->GetNoxelContainer();
		if (!Container)
		{
			continue;
		}
		if (ReservedWaiting.Contains(Container))
		{
			continue;
		}
		TArray<int32> Reserved = GetReservedPanels(Container);
		if (Reserved.Num() < 256)
		{
			ReservedWaiting.Add(Container);
			ReservePanels(256 + 128 - Reserved.Num(), Container);
		}
	}
	// ...
}

void UNoxelNetworkingAgent::AddQueueToBuffer(FEditorQueue* Queue)
{
	if (QueuesBuffer.Num() == NOXELEDITORQUEUEBUFFERLENGTH)
	{
		delete QueuesBuffer[0];
		QueuesBuffer.RemoveAt(0);
	}
	QueuesBuffer.Add(Queue);
}

void UNoxelNetworkingAgent::RemoveQueueFromBuffer(int32 OrderIndex)
{
	for (int i = 0; i < QueuesBuffer.Num(); ++i)
	{
		if (QueuesBuffer[i]->OrderNumber == OrderIndex)
		{
			delete QueuesBuffer[i];
			QueuesBuffer.RemoveAt(i);
			return;
		}
	}
}

bool UNoxelNetworkingAgent::GetQueueFromBuffer(int32 OrderIndex, FEditorQueue** Queue)
{
	for (int i = 0; i < QueuesBuffer.Num(); ++i)
	{
		if (QueuesBuffer[i]->OrderNumber == OrderIndex)
		{
			*Queue = QueuesBuffer[i];
			return true;
		}
	}
	return false;
}

TArray<int32> UNoxelNetworkingAgent::GetReservedPanels(UNoxelContainer* Container)
{
	TArray<int32>* Reserved = ReservedPanels.Find(Container);
	if (Reserved != nullptr)
	{
		return *Reserved;
	}
	return {};
}

void UNoxelNetworkingAgent::UseReservedPanels(TArray<FPanelID> Used)
{
	for (FPanelID Panel : Used)
	{
		TArray<int32>* Reserved = ReservedPanels.Find(Panel.Object);
		if (Reserved != nullptr)
		{
			Reserved->RemoveSwap(Panel.PanelIndex);
		}
	}
}

void UNoxelNetworkingAgent::AddReservedPanels(TArray<FPanelID> Add)
{
	for (FPanelID Panel : Add)
	{
		TArray<int32>* Reserved = ReservedPanels.Find(Panel.Object);
		if (Reserved != nullptr)
		{
			Reserved->Add(Panel.PanelIndex);
		}
	}
}

void UNoxelNetworkingAgent::ReservePanels(int32 NumToReserve, UNoxelContainer* Container)
{
	if (IsValid(Container))
	{
		ReservePanelsServer(NumToReserve, Container);
	}
}

void UNoxelNetworkingAgent::ReservePanelsServer_Implementation(int32 NumToReserve, UNoxelContainer* Container)
{
	if (IsValid(Container))
	{
		TArray<int32> Reserved = Container->ReservePanelIndices(NumToReserve);
		ReservePanelsClient(Container, Reserved);
	}
}

bool UNoxelNetworkingAgent::ReservePanelsServer_Validate(int32 NumToReserve, UNoxelContainer* Container)
{
	return NumToReserve <= 512; //Limit the maximum amount of panels that can be reserved at once
}

void UNoxelNetworkingAgent::ReservePanelsClient_Implementation(UNoxelContainer* Container, const TArray<int32> &Reserved)
{
	TArray<int32> Already = GetReservedPanels(Container);
	Already.Append(Reserved);
	ReservedPanels.Add(Container, Already);
	ReservedWaiting.Remove(Container);
}

FEditorQueue* UNoxelNetworkingAgent::CreateEditorQueue()
{
	FEditorQueue* Queue = new FEditorQueue();
	Queue->OrderNumber = NextQueueIndex++;
	AddQueueToBuffer(Queue);
	return Queue;
}

void UNoxelNetworkingAgent::SendCommandQueue(FEditorQueue* Queue)
{
	UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::SendCommandQueue] Running queue locally"));
	bool bValid = Queue->ExecuteQueue();
	if (bValid)
	{
		UseReservedPanels(Queue->GetReservedPanelsUsed());
		QueuesWaiting.Add(Queue->OrderNumber);
		FEditorQueueNetworkable Networkable;
		if (Queue->ToNetworkable(Networkable))
		{
			if (GetWorld()->IsServer()) //skip verification
            {
            	ClientsReceiveCommandQueue(Networkable);
            }
            else
            {
                ServerReceiveCommandQueue(Networkable);
            }
		}
	}
}

void UNoxelNetworkingAgent::ServerReceiveCommandQueue_Implementation(FEditorQueueNetworkable Networkable)
{
	FEditorQueue* Queue;
	bool bValid = Networkable.DecodeQueue(&Queue);
	bool bDecoded = false;
	if (bValid)
	{
		UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::ServerReceiveCommandQueue_Implementation] Running queue from client"));
		bDecoded = true;
		bValid = Queue->ExecuteQueue();
	}
	
	if (bValid)
	{
		QueuesWaiting.Add(Queue->OrderNumber);
		ClientsReceiveCommandQueue(Networkable);
		AddQueueToBuffer(Queue);
	}
	else
	{
		ClientRectifyCommandQueue(Networkable.OrderNumber, false);
		if (bDecoded)
		{
			delete Queue;
		}
	}
}

bool UNoxelNetworkingAgent::ServerReceiveCommandQueue_Validate(FEditorQueueNetworkable Networkable)
{
	return true;
}

void UNoxelNetworkingAgent::ClientsReceiveCommandQueue_Implementation(FEditorQueueNetworkable Networkable)
{
	if (QueuesWaiting.Contains(Networkable.OrderNumber))
	{
		QueuesWaiting.Remove(Networkable.OrderNumber);
	}
	else
	{
		UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::ClientsReceiveCommandQueue_Implementation] Running queue from other player. IsServer = %s"), GetWorld()->IsServer() ? TEXT("true") : TEXT("false"));
		FEditorQueue* Queue;
		if(Networkable.DecodeQueue(&Queue))
		{
			AddQueueToBuffer(Queue);
			Queue->ExecuteQueue();
		}
	}
}

void UNoxelNetworkingAgent::ClientRectifyCommandQueue_Implementation(int32 IndexToRectify, bool ShouldExecute)
{
	QueuesWaiting.Remove(IndexToRectify);
	FEditorQueue* Queue;
	if(GetQueueFromBuffer(IndexToRectify, &Queue))
	{
		if (!ShouldExecute)
		{
			AddReservedPanels(Queue->GetReservedPanelsUsed()); //Can get reserved panels before undoing
		}
		UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::ClientRectifyCommandQueue_Implementation] Running queue as rectification"));
		Queue->RunQueue(ShouldExecute);
		if (ShouldExecute)
		{
			UseReservedPanels(Queue->GetReservedPanelsUsed()); //Can get reserved panels after doing
		}
	}
}

void UNoxelNetworkingAgent::AddBlock(UVoxelComponent* Container, FIntVector Location)
{
	if(!Container)
	{
		UE_LOG(NoxelDataNetwork, Log, TEXT("{AddBlock} Container reference is broken, dropping call..."));
		return;
	}
	Container->addCube(Location);
	Server_Voxel(Container, Location, EVoxelOperation::Add);
}

void UNoxelNetworkingAgent::RemoveBlock(UVoxelComponent* Container, FIntVector Location)
{
	if(!Container)
	{
		UE_LOG(NoxelDataNetwork, Log, TEXT("{RemoveBlock} Container reference is broken, dropping call..."));
		return;
	}
	Container->removeCube(Location);
	Server_Voxel(Container, Location, EVoxelOperation::Remove);
}

void UNoxelNetworkingAgent::AddObject(FObjectPermissionDelegate Callback, TSubclassOf<AActor> Class, FTransform Location, bool bNeedsOwnership)
{
	FDateTime now = FDateTime::UtcNow();
	if (Callback.IsBound())
	{
		ObjectCallbacks.Add(now, Callback);
	}
	Server_Object(now, EObjectOperation::Add, Class, nullptr, Location, bNeedsOwnership);
	
}

void UNoxelNetworkingAgent::MoveObject(FObjectPermissionDelegate Callback, FTransform Location, AActor * Object, bool bNeedsOwnership)
{
	FDateTime now = FDateTime::UtcNow();
	if (Callback.IsBound())
	{
		ObjectCallbacks.Add(now, Callback);
	}
	Server_Object(now, EObjectOperation::Move, nullptr, Object, Location, bNeedsOwnership);
}

void UNoxelNetworkingAgent::RemoveObject(AActor * Object)
{
	FDateTime now = FDateTime::UtcNow();
	Server_Object(now, EObjectOperation::Remove, nullptr, Object, FTransform::Identity, false);
}

void UNoxelNetworkingAgent::ResignObject()
{
	FDateTime now = FDateTime::UtcNow();
	Server_Object(now, EObjectOperation::Resign, nullptr, nullptr, FTransform::Identity, false);
}

void UNoxelNetworkingAgent::MoveObjectUnreliable_Implementation(AActor * Object, FTransform Location)
{
	if (OwnedActor)
	{
		if (OwnedActor == Object)
		{
			Object->SetActorTransform(Location);
		}
		else
		{
			UE_LOG(Noxel, Log, TEXT("[UNoxelNetworkingAgent::MoveObjectUnreliable_Implementation] OwnedActor isn't Object"));
		}
	}
	else 
	{
		UE_LOG(Noxel, Log, TEXT("[UNoxelNetworkingAgent::MoveObjectUnreliable_Implementation] OwnedActor invalid"));
	}
}

bool UNoxelNetworkingAgent::MoveObjectUnreliable_Validate(AActor * Object, FTransform Location)
{
	return canEdit(Object);
}

void UNoxelNetworkingAgent::ConnectConnector(UConnectorBase * A, UConnectorBase * B)
{
	if (A && B && UConnectorBase::CanBothConnect(A, B))
	{
		Server_Connector(A, B, true);
	}
}

void UNoxelNetworkingAgent::DisconnectConnector(UConnectorBase * A, UConnectorBase * B)
{
	if (A && B && UConnectorBase::AreConnected(A,B))
	{
		Server_Connector(A, B, false);
	}
}

void UNoxelNetworkingAgent::SpawnAndPossessCraft_Implementation()
{
	ANoxelHangarBase* Hangar = (ANoxelHangarBase*)Craft->GetOwner();
	if (Hangar)
	{
		//CompClass can be a BP
		UCraftDataHandler* NewComp = NewObject<UCraftDataHandler>(UGameplayStatics::GetGameMode(this));
		if (!NewComp)
		{
			return;
		}
		//~~~~~~~~~~~~~
		NewComp->SpawnContext = ECraftSpawnContext::Battle;
		NewComp->loadCraft(Craft->saveCraft(), Hangar->getCraftSpawnPoint()->GetComponentTransform());
		APawn* Seat = nullptr;
		for (AActor* NObject : NewComp->Components)
		{
			if (NObject->IsA<APawn>())
			{
				Seat = (APawn*)NObject;
				break;
			}
		}
		if (Seat)
		{
			APawn* Owner = ((APawn*)GetOwner());
			for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				if (UGameplayStatics::GetPlayerPawn(this, Iterator.GetIndex()) == Owner)
				{
					Owner->Destroy();
					Iterator->Get()->Possess(Seat);
				}
			}
		}
		NewComp->enableCraft();
	}
}

bool UNoxelNetworkingAgent::SpawnAndPossessCraft_Validate()
{
	return true;
}

/*
* Client -> Server ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

//Voxel --------------------------------------------------------------------------------------------------------------------------------
void UNoxelNetworkingAgent::Server_Voxel_Implementation(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType)
{
	if(!Container){
		UE_LOG(NoxelDataNetwork, Warning, TEXT("[UNoxelNetworkingAgent::Server_Voxel_Implementation] Illegal voxel reference, aborting..."));
		return;
	}
	Clients_Voxel(Container, Location, OperationType);
}

bool UNoxelNetworkingAgent::Server_Voxel_Validate(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType)
{
	return true;
}

//Objects --------------------------------------------------------------------------------------------------------------------------------
void UNoxelNetworkingAgent::Server_Object_Implementation(FDateTime RequestTime, EObjectOperation OperationType, TSubclassOf<AActor> Class, AActor* Object, FTransform Location, bool bNeedsOwnership)
{
	if (OwnedActor)
	{
		OwnedActor->SetOwner(nullptr);
		OwnedActor = nullptr;
	}
	switch (OperationType)
	{
	case EObjectOperation::Add:
		if (Class && UNoxelDataAsset::HasClass(DataTable, Class) && Craft) {
			FActorSpawnParameters SpawnParams = FActorSpawnParameters();
			if (bNeedsOwnership)
			{
				SpawnParams.Owner = GetOwner();
			}
			AActor* Spawned = Craft->AddComponent(Class, Location, SpawnParams);
			if (bNeedsOwnership)
			{
				OwnedActor = Spawned;
			}
			int32 IndexSpawned;
			Craft->Components.Find(Spawned, IndexSpawned);
			Client_Object(RequestTime, EObjectOperation::Add, Spawned, IndexSpawned);
		}
		else
		{
			if (!Class)
			{
				UE_LOG(Noxel, Warning, TEXT("[UNoxelNetworkingAgent::Server_Object_Implementation] Class is invalid"));
			}
			if (!UNoxelDataAsset::HasClass(DataTable, Class))
			{
				UE_LOG(Noxel, Warning, TEXT("[UNoxelNetworkingAgent::Server_Object_Implementation] Class not found in DataTable"));
			}
			Client_Object(RequestTime, EObjectOperation::Resign, nullptr);
		}
		return;
	case EObjectOperation::Move:
		if (Object && Craft)
		{
			if (Craft->Components.Contains(Object) && Object->GetOwner() == nullptr)
			{
				Object->SetActorTransform(Location);
				if (bNeedsOwnership)
				{
					Object->SetOwner(GetOwner());
					OwnedActor = Object;
				}
				Client_Object(RequestTime, EObjectOperation::Move, Object);
				return;
			}
		}
		Client_Object(RequestTime, EObjectOperation::Resign, nullptr);
		return;
	case EObjectOperation::Remove:
		if (Object && Craft)
		{
			if (Craft->Components.Contains(Object) && Object->GetOwner() == nullptr)
			{
				Craft->Components.Remove(Object);
				Object->Destroy();
				Client_Object(RequestTime, EObjectOperation::Remove, nullptr);
				return;
			}
		}
		Client_Object(RequestTime, EObjectOperation::Resign, nullptr);
		return;
	case EObjectOperation::Resign:
		return;
	default:
		break;
	}
}

bool UNoxelNetworkingAgent::Server_Object_Validate(FDateTime RequestTime, EObjectOperation OperationType, TSubclassOf<AActor> Class, AActor* Object, FTransform Location, bool bNeedsOwnership)
{
	return canEdit(Object);
}

void UNoxelNetworkingAgent::Server_Connector_Implementation(UConnectorBase* A, UConnectorBase* B, bool bIsConnecting)
{
	if (A && B)
	{
		if (bIsConnecting)
		{
			A->Connect(B);
		}
		else 
		{
			A->Disconnect(B);
		}
	}
}

bool UNoxelNetworkingAgent::Server_Connector_Validate(UConnectorBase* A, UConnectorBase* B, bool bIsConnecting)
{
	return canEdit(A->GetOwner()) && B->GetOwner();
}

/*
* Server -> All clients ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void UNoxelNetworkingAgent::Clients_Voxel_Implementation(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType)
{
	Voxel(Container, Location, OperationType);
}

/*
* Server -> Client ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void UNoxelNetworkingAgent::Client_Object_Implementation(FDateTime RequestTime, EObjectOperation OperationType, AActor* Object, int32 ActorIndex)
{
	//UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::Client_Object_Implementation]"));
	if (OperationType != EObjectOperation::Resign)
	{
		//UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::Client_Object_Implementation] Operation is correct"));
		if (ObjectCallbacks.Contains(RequestTime))
		{
			//UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::Client_Object_Implementation] Callback found"));
			FObjectPermissionDelegate Callback = ObjectCallbacks.FindAndRemoveChecked(RequestTime);
			if (!Object) //If object replication hasn't come yet, put the spawn on hold
			{
				ObjectsWaiting.Add(ActorIndex, Callback);
			}
			else if (Callback.IsBound())
			{
				//UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::Client_Object_Implementation] Broadcasting callback"));
				Callback.Broadcast(Object);
			}
		}
	}
}

/*
* Generic functions to agree to the server ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void UNoxelNetworkingAgent::Voxel(UVoxelComponent* Container, FIntVector Location, EVoxelOperation OperationType)
{
	if(!Container){
		UE_LOG(NoxelDataNetwork, Warning, TEXT("{Voxel} Received a broken container reference, aborting..."));
		return;
	}
	switch(OperationType){
	case (EVoxelOperation::Add):
		Container->addCube(Location);
		break;
	case (EVoxelOperation::Remove):
		Container->removeCube(Location);
		break;
	}
	
}

/*
* Checks ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

bool UNoxelNetworkingAgent::canEdit(AActor * Target)
{
	if (!Target)
	{
		return true;
	}
	if (!Craft)
	{
		return false;
	}
	return Craft->Components.Contains(Target);
}

void UNoxelNetworkingAgent::OnCraftComponentsReplicated()
{
	for (const TPair<int32, FObjectPermissionDelegate> ComponentPair : ObjectsWaiting)
	{
		if (Craft->Components.IsValidIndex(ComponentPair.Key))
		{
			if (ComponentPair.Value.IsBound())
			{
				ComponentPair.Value.Broadcast(Craft->Components[ComponentPair.Key]);
			}
		}
	}
	ObjectsWaiting.Empty();
}