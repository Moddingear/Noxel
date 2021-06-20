//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/NoxelNetworkingAgent.h"

#include "Noxel.h"
#include "EditorGameState.h"

#include "Noxel/CraftDataHandler.h"
#include "NoxelHangarBase.h"
#include "Engine/DemoNetDriver.h"
#include "NObjects/NoxelPart.h"

// Sets default values for this component's properties
UNoxelNetworkingAgent::UNoxelNetworkingAgent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1;
	static ConstructorHelpers::FObjectFinder<UDataTable> DataConstructor(OBJECTLIBRARY_PATH);
	if (DataConstructor.Succeeded()) {
		DataTable = DataConstructor.Object;
	}
	TempObjects.Init(nullptr, 10);
	// ...
}


// Called when the game starts
void UNoxelNetworkingAgent::BeginPlay()
{
	Super::BeginPlay();
	AEditorGameState* gs = GetWorld()->GetGameState<AEditorGameState>();
	if (gs)
	{
		ANoxelHangarBase* hangar = gs->GetHangar();
		if (hangar)
		{
			Craft = hangar->GetCraftDataHandler();
			UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::BeginPlay] Craft found"));
			Craft->OnComponentsReplicatedEvent.AddDynamic(this, &UNoxelNetworkingAgent::OnCraftComponentsReplicated);
			if (!GetWorld()->IsServer())
			{
				Craft->OnReceiveQueueStart.AddDynamic(this, &UNoxelNetworkingAgent::UndoWaitingQueues);
				Craft->OnReceiveQueueEnd.AddDynamic(this, &UNoxelNetworkingAgent::RedoWaitingQueues);
			}
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


void UNoxelNetworkingAgent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNoxelNetworkingAgent, TempObjects, COND_None);
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

void UNoxelNetworkingAgent::UndoWaitingQueues()
{
	for (int i = QueuesWaiting.Num() - 1; i >= 0; --i)
	{
		FEditorQueue* Queue;
		if(GetQueueFromBuffer(QueuesWaiting[i], &Queue))
		{
			Queue->UndoQueue();
		}
	}
}

void UNoxelNetworkingAgent::RedoWaitingQueues()
{
	for (int i = 0; i < QueuesWaiting.Num(); ++i)
	{
		FEditorQueue* Queue;
		if(GetQueueFromBuffer(QueuesWaiting[i], &Queue))
		{
			Queue->ExecuteQueue();
		}
	}
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
		if (!GetWorld()->IsServer())
		{
			Craft->OnReceiveQueueStart.Broadcast();
		}
		UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::ClientsReceiveCommandQueue_Implementation] Running queue from other player. IsServer = %s"), GetWorld()->IsServer() ? TEXT("true") : TEXT("false"));
		FEditorQueue* Queue;
		if(Networkable.DecodeQueue(&Queue))
		{
			AddQueueToBuffer(Queue);
			Queue->ExecuteQueue();
		}
		if (!GetWorld()->IsServer())
		{
			Craft->OnReceiveQueueEnd.Broadcast();
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

void UNoxelNetworkingAgent::AddTempObject(FObjectPermissionDelegate Callback, TSubclassOf<AActor> Class, FTransform Location)
{
	if (Callback.IsBound())
	{
		ObjectCallbacks.Add(CurrentCallbackIdx, Callback);
	}
	AddTempObjectServer(Class, Location, CurrentCallbackIdx);
	CurrentCallbackIdx++;
}

void UNoxelNetworkingAgent::MoveTempObject(AActor * Object, FTransform Location)
{
	Object->SetActorTransform(Location);
	MoveTempObjectServer(Object, Location);
}

void UNoxelNetworkingAgent::RemoveTempObject(AActor * Object)
{
	RemoveTempObjectServer(Object);
}

void UNoxelNetworkingAgent::MoveTempObjectServer_Implementation(AActor * Object, FTransform Location)
{
	if (Object)
	{
		if (Object->GetOwner() == GetOwner() && TempObjects.Contains(Object))
		{
			Object->SetActorTransform(Location);
			MoveTempObjectClients(Object, Location);
		}
		else
		{
			UE_LOG(Noxel, Log, TEXT("[UNoxelNetworkingAgent::MoveTempObject_Implementation] Object moved isn't owned"));
		}
	}
	else 
	{
		UE_LOG(Noxel, Log, TEXT("[UNoxelNetworkingAgent::MoveTempObject_Implementation] OwnedActor invalid"));
	}
}

bool UNoxelNetworkingAgent::MoveTempObjectServer_Validate(AActor * Object, FTransform Location)
{
	return TempObjects.Contains(Object);
}

void UNoxelNetworkingAgent::MoveTempObjectClients_Implementation(AActor* Object, FTransform Location)
{
	if (IsValid(Object))
	{
		if (!TempObjects.Contains(Object)) //TempObjects is owner only, so if it's not in TempObjects, it's not ours
		{
			Object->SetActorTransform(Location);
		}
	}
}

void UNoxelNetworkingAgent::SpawnAndPossessCraft_Implementation()
{
	ANoxelHangarBase* Hangar = Cast<ANoxelHangarBase>(Craft->GetOwner());
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
				Seat = Cast<APawn>(NObject);
				break;
			}
		}
		if (Seat)
		{
			APawn* Owner = Cast<APawn>(GetOwner());
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

void UNoxelNetworkingAgent::AddTempObjectServer_Implementation(TSubclassOf<AActor> Class, FTransform Location, int32 CallbackIdx)
{
	if (IsValid(Class))
	{
		AActor* SpawnedActor = GetWorld()->SpawnActorDeferred<AActor>(Class, Location, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		int i;
		for (i = 0; i < TempObjects.Num(); ++i)
		{
			if (TempObjects[i] == nullptr)
			{
				TempObjects[i] = SpawnedActor;
				break;
			}
		}
		SpawnedActor->SetActorEnableCollision(false);
		SpawnedActor->SetReplicateMovement(false);
		//TODO : enable some sort of hologram effect
		SpawnedActor->FinishSpawning(Location);
		NotifyClientCallback(CallbackIdx, i);
	}
	else
	{
		UE_LOG(NoxelDataNetwork, Warning, TEXT("[UNoxelNetworkingAgent::AddTempObjectServer_Implementation] Class is invalid"));
	}
}

bool UNoxelNetworkingAgent::AddTempObjectServer_Validate(TSubclassOf<AActor> Class, FTransform Location, int32 CallbackIdx)
{
	int NumValid = 0;
	for (AActor* TempObject : TempObjects)
	{
		if (IsValid(TempObject))
		{
			NumValid++;
		}
	}
	return NumValid<10;
}


void UNoxelNetworkingAgent::RemoveTempObjectServer_Implementation(AActor* Object)
{
	if (Object)
	{
		TempObjects[TempObjects.Find(Object)] = nullptr;
		Object->Destroy();
	}
}

bool UNoxelNetworkingAgent::RemoveTempObjectServer_Validate(AActor* Object)
{
	return TempObjects.Contains(Object);
}

void UNoxelNetworkingAgent::NotifyClientCallback_Implementation(int32 CallbackIndex, int32 TempObjectsIndex)
{
	if (ObjectCallbacks.Contains(CallbackIndex))
	{
		
		if (TempObjects.IsValidIndex(TempObjectsIndex) && IsValid(TempObjects[TempObjectsIndex]))
		{
			UE_LOG(NoxelDataNetwork, Log, TEXT(
                    "[UNoxelNetworkingAgent::NotifyClientCallback_Implementation] Object is valid : TempObjectIndex = %d; CallbackIndex = %d"
                ), TempObjectsIndex, CallbackIndex);
			FObjectPermissionDelegate Callback;
         	if(ObjectCallbacks.RemoveAndCopyValue(CallbackIndex, Callback))
         	{
         		if (Callback.IsBound())
         		{
         			Callback.Broadcast(TempObjects[TempObjectsIndex]);
         		}
         	}
			
		}
		else
		{
			UE_LOG(NoxelDataNetwork, Log, TEXT(
					"[UNoxelNetworkingAgent::NotifyClientCallback_Implementation] Object is invalid, adding index to waiting list : TempObjectIndex = %d; CallbackIndex = %d"
				), TempObjectsIndex, CallbackIndex);
			ObjectsWaiting.Add(TempObjectsIndex, CallbackIndex);
		}
	}
}

void UNoxelNetworkingAgent::OnRep_TempObjects()
{
	UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::OnRepTempObjects] Called"));
	for (int i = 0; i < TempObjects.Num(); ++i)
	{
		if (IsValid(TempObjects[i]))
		{
			int32 CallbackIndex;
			if (ObjectsWaiting.RemoveAndCopyValue(i, CallbackIndex))
			{
				UE_LOG(NoxelDataNetwork, Log, TEXT("[UNoxelNetworkingAgent::OnRepTempObjects] TempObject[%d] is valid, calling callback %d"),
					i, CallbackIndex);
				FObjectPermissionDelegate Callback;
				if(ObjectCallbacks.RemoveAndCopyValue(CallbackIndex, Callback))
				{
					if (Callback.IsBound())
					{
						Callback.Broadcast(TempObjects[i]);
					}
					else
					{
						UE_LOG(NoxelDataNetwork, Warning, TEXT("[UNoxelNetworkingAgent::OnRepTempObjects] Callback isn't bound !"));
					}
				}
			}
		}
	}
}

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
	return;
}
