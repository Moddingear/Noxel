// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "EditorCommandQueue.h"

#include "Noxel.h"
#include "Noxel/NoxelDataStructs.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"

DEFINE_LOG_CATEGORY(LogEditorCommandQueue);

int32 FEditorQueueNetworkable::AddPointer(UObject* InPtr)
{
	return Pointers.AddUnique(InPtr);
}

UObject* FEditorQueueNetworkable::GetPointer(int32 InIndex)
{
	if (Pointers.IsValidIndex(InIndex))
	{
		return Pointers[InIndex];
	}
	else
	{
		UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueNetworkable::GetPointer(%d)@%p] Failed getting pointer"), InIndex, this);
		return nullptr;
	}
}

bool FEditorQueueNetworkable::OrderFromNetworkable(int32 OrderIndex, FEditorQueueOrderTemplate** OutOrder)
 {
	FEditorQueueOrderTemplate* order = nullptr;
 	switch (Orders[OrderIndex].OrderType)
 	{
 	case EEditorQueueOrderType::NodeReference:
 		order = new FEditorQueueOrderNodeReference();
 		break;
 	case EEditorQueueOrderType::NodeAdd:
 	case EEditorQueueOrderType::NodeRemove:
 		order = new FEditorQueueOrderNodeAddRemove();
 		break;
 	case EEditorQueueOrderType::NodeConnect:
 	case EEditorQueueOrderType::NodeDisconnect:
 		order = new FEditorQueueOrderNodeDisConnect();
 		break;
 	case EEditorQueueOrderType::PanelReference:
 		order = new FEditorQueueOrderPanelReference();
 		break;
 	case EEditorQueueOrderType::PanelAdd:
 	case EEditorQueueOrderType::PanelRemove:
 		order = new FEditorQueueOrderPanelAddRemove();
 		break;
 	case EEditorQueueOrderType::PanelProperties:
 		order = new FEditorQueueOrderPanelProperties();
 		break;
 	default:
 		break;
 	}
	if (order != nullptr && order->FromNetworkable(this, OrderIndex))
	{
		*OutOrder = order;
		return true;
	}
 	return false;
 }

bool FEditorQueueNetworkable::DecodeQueue(FEditorQueue** Decoded)
{
	FEditorQueue* queue = new FEditorQueue();
	queue->Orders.Empty();
	queue->Orders.SetNum(Orders.Num());
	queue->OrderNumber = OrderNumber;
	for (int i = 0; i < Orders.Num(); ++i)
	{
		FEditorQueueOrderTemplate* Order;
		bool success = OrderFromNetworkable(i, &Order);
		if (success)
		{
			queue->Orders[i]=Order;
		}
		else
		{
			UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueue::DecodeQueue@%p] Failed at %d on instruction %s"),
				this, i, *Orders[i].ToString());
			for (int j = 0; j < i; ++j)
			{
				delete queue->Orders[j];
			}
			queue->Orders.Empty();
			return false;
		}
	}
	*Decoded = queue;
	return true;
}

FEditorQueue::~FEditorQueue()
{
	for (int j = Orders.Num() - 1; j >= 0; --j)
	{
		delete Orders[j];
	}
}

bool FEditorQueue::RunQueue(bool bShouldExecute = true)
{
	bool bStillValid = true;
	TArray<UNoxelDataComponent*> AffArr;
	int OrderIdx;
	//Run orders
	for (OrderIdx = 0; OrderIdx < Orders.Num(); ++OrderIdx)
	{
		FEditorQueueOrderTemplate* Order = Orders[bShouldExecute ? OrderIdx : Orders.Num() - 1 - OrderIdx];
		bool success;
		if (bShouldExecute)
		{
			success = Order->ExecuteOrder(this);
		}
		else
		{
			success = Order->UndoOrder(this);
		}
		TArray<UNoxelDataComponent*> Affected = Order->GetAffectedDataComponents(this);
		for (UNoxelDataComponent* NoxelDataComponent : Affected)
		{
			AffArr.AddUnique(NoxelDataComponent);
		}
		if (!success)
		{
			UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueue::RunQueue(%d)@%p] Failed at %d on instruction %s"),
				bShouldExecute, this, OrderIdx, *Order->ToString());
			bStillValid = false;
			break;
		}
	}
	//Check nodes container validity
	if(bStillValid)
	{
		for (int i = 0; i < AffArr.Num(); ++i)
		{
			if (AffArr[i]->IsA<UNodesContainer>())
			{
				if (!AffArr[i]->CheckDataValidity())
				{
					bStillValid = false;
					break;
				}
			}
		}
	}
	//Check panel containers validity
	if(bStillValid)
	{
		for (int i = 0; i < AffArr.Num(); ++i)
		{
			if (AffArr[i]->IsA<UNoxelContainer>())
			{
				if (!AffArr[i]->CheckDataValidity())
				{
					bStillValid = false;
					break;
				}
			}
		}
	}
	if (bStillValid)
	{
		for (int i = 0; i < AffArr.Num(); ++i)
		{
			AffArr[i]->UpdateMesh();
		}
	}
	//if failed, undo all
	if (!bStillValid)
	{
		for (int j = OrderIdx - 1; j >= 0; --j)
		{
			FEditorQueueOrderTemplate* Order = Orders[bShouldExecute ? j : Orders.Num() - 1 - j];
			if (bShouldExecute)
			{
				Order->UndoOrder(this);
			}
			else
			{
				Order->ExecuteOrder(this);
			}
		}
	}
	return bStillValid;
}

bool FEditorQueue::ExecuteQueue()
{
	return RunQueue(true);
}

bool FEditorQueue::UndoQueue()
{
	return RunQueue(false);
}

void FEditorQueue::AddNodeReferenceOrder(TArray<FVector> Locations, UNodesContainer* Container)
{
	FEditorQueueOrderNodeReference* order = new FEditorQueueOrderNodeReference(Locations, Container);
	Orders.Add(order);
}

TMap<FNodeID, int32> FEditorQueue::CreateNodeReferenceOrdersFromNodeList(TArray<FNodeID> Nodes)
{
	TArray<UNodesContainer*> Containers;
	TArray<TArray<FNodeID>> SortedNodes;
	TMap<FNodeID, int32> Nodemap;
	for (FNodeID Node : Nodes)
	{
		int32 ContainerIdx = Containers.Find(Node.Object);
		if (ContainerIdx != INDEX_NONE)
		{
			SortedNodes[ContainerIdx].Add(Node);
		}
		else
		{
			Containers.Add(Node.Object);
			SortedNodes.Add({Node});
		}
	}
	int32 NodeRefIdx = 0;
	for (int ContainerIdx = 0; ContainerIdx < Containers.Num(); ++ContainerIdx)
	{
		TArray<FVector> Locations;
		for (FNodeID Node : SortedNodes[ContainerIdx])
		{
			Locations.Add(Node.Location);
			Nodemap.Add(Node, NodeRefIdx++);
		}
		AddNodeReferenceOrder(Locations, Containers[ContainerIdx]);
	}
	return Nodemap;
}

TArray<int32> FEditorQueue::NodeListToNodeReferences(TArray<FNodeID> Nodes, TMap<FNodeID, int32> NodeMap)
{
	TArray<int32> NodeRef;
	for (FNodeID Node : Nodes)
	{
		NodeRef.Add(*NodeMap.Find(Node));
	}
	return NodeRef;
}

void FEditorQueue::AddNodeAddOrder(TArray<int32> NodeToAdd)
{
	FEditorQueueOrderNodeAddRemove* order = new FEditorQueueOrderNodeAddRemove(NodeToAdd, true);
	Orders.Add(order);
}

void FEditorQueue::AddNodeRemoveOrder(TArray<int32> NodeToRemove)
{
	FEditorQueueOrderNodeAddRemove* order = new FEditorQueueOrderNodeAddRemove(NodeToRemove, false);
	Orders.Add(order);
}

void FEditorQueue::AddPanelReferenceOrder(TArray<int32> PanelIndices, UNoxelContainer* Container)
{
	FEditorQueueOrderPanelReference* order = new FEditorQueueOrderPanelReference(PanelIndices, Container);
	Orders.Add(order);
}

void FEditorQueue::AddPanelAddOrder(TArray<int32> PanelIndexRef)
{
	FEditorQueueOrderPanelAddRemove* order = new FEditorQueueOrderPanelAddRemove(PanelIndexRef, true);
	Orders.Add(order);
}

void FEditorQueue::AddPanelRemoveOrder(TArray<int32> PanelIndexRef)
{
	FEditorQueueOrderPanelAddRemove* order = new FEditorQueueOrderPanelAddRemove(PanelIndexRef, false);
	Orders.Add(order);
}

void FEditorQueue::AddPanelPropertiesOrder(TArray<int32> PanelIndexRef, float ThicknessNormal,
	float ThicknessAntiNormal, bool Virtual)
{
	FEditorQueueOrderPanelProperties* order = new FEditorQueueOrderPanelProperties(PanelIndexRef, ThicknessNormal, ThicknessAntiNormal, Virtual);
	Orders.Add(order);
}

void FEditorQueue::AddNodeConnectOrder(TArray<int32> Nodes, TArray<int32> Panels)
{
	FEditorQueueOrderNodeDisConnect* order = new FEditorQueueOrderNodeDisConnect(Nodes, Panels, true);
	Orders.Add(order);
}

void FEditorQueue::AddNodeDisconnectOrder(TArray<int32> Nodes, TArray<int32> Panels)
{
	FEditorQueueOrderNodeDisConnect* order = new FEditorQueueOrderNodeDisConnect(Nodes, Panels, false);
	Orders.Add(order);
}

bool FEditorQueue::ToNetworkable(FEditorQueueNetworkable &Networkable)
{
	Networkable = FEditorQueueNetworkable();
	Networkable.OrderNumber = OrderNumber;
	for (int OrderIdx = 0; OrderIdx < Orders.Num(); ++OrderIdx)
	{
		Networkable.Orders.Add(Orders[OrderIdx]->ToNetworkable(&Networkable));
	}
	return true;
}

TArray<FPanelID> FEditorQueue::GetReservedPanelsUsed()
{
	TArray<FPanelID> Used;
	for (int32 OrderIdx = 0; OrderIdx < Orders.Num(); ++OrderIdx)
	{
		Used.Append(Orders[OrderIdx]->GetReservedPanelsUsed(this));
	}
	return Used;
}

bool FEditorQueueOrderNodeReference::ExecuteOrder(FEditorQueue* Parent)
{
	for (int i = 0; i < Locations.Num(); ++i)
	{
		Parent->NodeReferences.Add(FNodeID(Container, Locations[i]));
	}
	return true;
}

bool FEditorQueueOrderNodeReference::UndoOrder(FEditorQueue* Parent)
{
	Parent->NodeReferences.SetNum(Parent->NodeReferences.Num()-Locations.Num());
	return true;
}

FEditorQueueOrderNetworkable FEditorQueueOrderNodeReference::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderTemplate::ToNetworkable(Parent);
	Net.Args.Add(Parent->AddPointer(Container));
	for (int i = 0; i < Locations.Num(); ++i)
	{
		Net.AddVector(Locations[i]);
	}
	return Net;
}

bool FEditorQueueOrderNodeReference::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex);
	FEditorQueueOrderNetworkable InData = Parent->Orders[OrderIndex];
	if (InData.Args.Num()<1)
	{
		return false;
	}
	int32 NumLocations = (InData.Args.Num()-1)/3;
	if (InData.Args.Num() % 3 != 1)
	{
		return false;
	}
	Container = Cast<UNodesContainer>(Parent->GetPointer(InData.Args[0]));
	if (!IsValid(Container))
	{
		return false;
	}
	Locations.SetNum(NumLocations);
	for (int i = NumLocations - 1; i >= 0; --i)
	{
		Locations[i] = InData.PopVector();
	}
	return true;
}

TArray<UNoxelDataComponent*> FEditorQueueOrderNodeReference::GetAffectedDataComponents(FEditorQueue* Parent)
{
	return {};
}

FString FEditorQueueOrderNodeReference::ToString()
{
	FString LocString;
	for (int i = 0; i < Locations.Num(); ++i)
	{
		LocString+= FString::Printf(TEXT("[%d]=(%s) | "), i, *Locations[i].ToString());
	}
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; ContainerName = (%s), Vectors : "), *Container->GetName()) + LocString;
}

bool FEditorQueueOrderNodeAddRemove::ExecuteOrder(FEditorQueue* Parent)
{
	if (Add)
	{
		return DoAdd(Parent);
	}
	else
	{
		return DoRemove(Parent);
	}
}

bool FEditorQueueOrderNodeAddRemove::UndoOrder(FEditorQueue* Parent)
{
	if (Add)
	{
		return DoRemove(Parent);
	}
	else
	{
		return DoAdd(Parent);
	}
}

bool FEditorQueueOrderNodeAddRemove::DoAdd(FEditorQueue* Parent)
{
	bool valid = true;
	int i;
	for (i = 0; i < NodesToAddRemove.Num(); ++i)
	{
		int32 NodeToAdd = NodesToAddRemove[i];
		valid = Parent->NodeReferences.IsValidIndex(NodeToAdd);
		if (valid)
		{
			valid = UNodesContainer::AddNode(Parent->NodeReferences[NodeToAdd]);
		}
		if (!valid)
        {
			UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueOrderNodeAddRemove::DoAdd] Add failed"))
         	break;
        }
	}
	if (!valid)
	{
		for (int j = i - 1; j >= 0; --j)
		{
			int32 NodeToAdd = NodesToAddRemove[j];
			UNodesContainer::RemoveNode(Parent->NodeReferences[NodeToAdd]);
		}
	}
	return valid;
}

bool FEditorQueueOrderNodeAddRemove::DoRemove(FEditorQueue* Parent)
{
	bool valid = true;
	int i;
	for (i = 0; i < NodesToAddRemove.Num(); ++i)
	{
		int32 NodeToAdd = NodesToAddRemove[i];
		valid = Parent->NodeReferences.IsValidIndex(NodeToAdd);
		if (valid)
		{
			valid = UNodesContainer::RemoveNode(Parent->NodeReferences[NodeToAdd]);
		}
		if (!valid)
		{
			break;
		}
	}
	if (!valid)
	{
		for (int j = i - 1; j >= 0; --j)
		{
			int32 NodeToAdd = NodesToAddRemove[j];
			UNodesContainer::AddNode(Parent->NodeReferences[NodeToAdd]);
		}
	}
	return valid;
}

FEditorQueueOrderNetworkable FEditorQueueOrderNodeAddRemove::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net(Add ? EEditorQueueOrderType::NodeAdd : EEditorQueueOrderType::NodeRemove);
	Net.Args = NodesToAddRemove;
	return Net;
}

bool FEditorQueueOrderNodeAddRemove::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex);
	Add = OrderType == EEditorQueueOrderType::NodeAdd;
	NodesToAddRemove = Parent->Orders[OrderIndex].Args;
	return true;
}

TArray<UNoxelDataComponent*> FEditorQueueOrderNodeAddRemove::GetAffectedDataComponents(FEditorQueue* Parent)
{
	TArray<UNoxelDataComponent*> affected;
	for (int32 NodeToAdd : NodesToAddRemove)
	{
		if (Parent->NodeReferences.IsValidIndex(NodeToAdd))
		{
			affected.AddUnique(Cast<UNoxelDataComponent>(Parent->NodeReferences[NodeToAdd].Object));
		}
	}
	return affected;
}

FString FEditorQueueOrderNodeAddRemove::ToString()
{
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; Add =  %d, NodeToAddRemove.Num() = (%d)"), Add, NodesToAddRemove.Num());
}

bool FEditorQueueOrderNodeDisConnect::ExecuteOrder(FEditorQueue* Parent)
{
	if (Connect)
	{
		return DoConnect(Parent);
	}
	else
	{
		return DoDisconnect(Parent);
	}
}

bool FEditorQueueOrderNodeDisConnect::UndoOrder(FEditorQueue* Parent)
{
	if (Connect)
	{
		return DoDisconnect(Parent);
	}
	else
	{
		return DoConnect(Parent);
	}
}

bool FEditorQueueOrderNodeDisConnect::DoConnect(FEditorQueue* Parent)
{
	if (Panels.Num() != Nodes.Num())
	{
		return false;
	}
	bool valid = true;
	int i;
	for (i = 0; i < Nodes.Num(); ++i)
	{
		valid = Parent->NodeReferences.IsValidIndex(Nodes[i]) && Parent->PanelReferences.IsValidIndex(Panels[i]);
		if (valid)
		{
			valid = IsValid(Parent->PanelReferences[Panels[i]].Object);
			if(valid)
			{
				valid = Parent->PanelReferences[Panels[i]].Object->ConnectNodeDiffered(Parent->PanelReferences[Panels[i]].PanelIndex, Parent->NodeReferences[Nodes[i]]);
			}
		}
		if(!valid)
		{
			break;
		}
	}
	if (!valid)
	{
		for (int j = i - 1; j >= 0; --j)
        {
        	Parent->PanelReferences[Panels[j]].Object->DisconnectNodeDiffered(Parent->PanelReferences[Panels[j]].PanelIndex, Parent->NodeReferences[Nodes[j]]);
        }
	}
	return valid;
}

bool FEditorQueueOrderNodeDisConnect::DoDisconnect(FEditorQueue* Parent)
{
	if (Panels.Num() != Nodes.Num())
	{
		return false;
	}
	bool valid = true;
	int i;
	for (i = 0; i < Nodes.Num(); ++i)
	{
		valid = Parent->NodeReferences.IsValidIndex(Nodes[i]) && Parent->PanelReferences.IsValidIndex(Panels[i]);
		if (valid)
		{
			valid = IsValid(Parent->PanelReferences[Panels[i]].Object);
			if(valid)
			{
				valid = Parent->PanelReferences[Panels[i]].Object->DisconnectNodeDiffered(Parent->PanelReferences[Panels[i]].PanelIndex, Parent->NodeReferences[Nodes[i]]);
			}
		}
		if(!valid)
		{
			break;
		}
	}
	if (!valid)
	{
		for (int j = i - 1; j >= 0; --j)
		{
			Parent->PanelReferences[Panels[j]].Object->ConnectNodeDiffered(Parent->PanelReferences[Panels[j]].PanelIndex, Parent->NodeReferences[Nodes[j]]);
		}
	}
	return valid;
}

FEditorQueueOrderNetworkable FEditorQueueOrderNodeDisConnect::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net(Connect ? EEditorQueueOrderType::NodeConnect : EEditorQueueOrderType::NodeDisconnect);
	for (int i = 0; i < FMath::Min(Nodes.Num(), Panels.Num()); ++i)
	{
		Net.Args.Add(Nodes[i]);
		Net.Args.Add(Panels[i]);
	}
	return Net;
}

bool FEditorQueueOrderNodeDisConnect::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex);
	Connect = OrderType == EEditorQueueOrderType::NodeConnect;
	FEditorQueueOrderNetworkable InData = Parent->Orders[OrderIndex];
	if (InData.Args.Num() % 2 ==1) //must have a pair of elements
	{
		return false;
	}
	for (int i = 0; i < InData.Args.Num()/2; ++i)
	{
		Nodes.Add(InData.Args[2*i]);
		Panels.Add(InData.Args[2*i+1]);
	}
	return true;
}

TArray<UNoxelDataComponent*> FEditorQueueOrderNodeDisConnect::GetAffectedDataComponents(FEditorQueue* Parent)
{
	TArray<UNoxelDataComponent*> Affected;
	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (Parent->NodeReferences.IsValidIndex(Nodes[i]))
		{
			Affected.AddUnique(Parent->NodeReferences[Nodes[i]].Object);
		}
		
	}
	for (int i = 0; i < Panels.Num(); ++i)
	{
		if (Parent->PanelReferences.IsValidIndex(Panels[i]))
		{
			Affected.AddUnique(Parent->PanelReferences[Panels[i]].Object);
		}
	}
	Affected.Remove(nullptr);
	return Affected;
}

FString FEditorQueueOrderNodeDisConnect::ToString()
{
	FString Connections;
	for (int i = 0; i < FMath::Min( Nodes.Num(), Panels.Num()); ++i)
	{
		Connections += FString::Printf(TEXT("%d->%d; "), Nodes[i], Panels[i]);
	}
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; Connections : %s"), *Connections);
}

bool FEditorQueueOrderPanelReference::ExecuteOrder(FEditorQueue* Parent)
{
	for (int i = 0; i < PanelIndices.Num(); ++i)
	{
		Parent->PanelReferences.Emplace(Container, PanelIndices[i]);
	}
	return true;
}

bool FEditorQueueOrderPanelReference::UndoOrder(FEditorQueue* Parent)
{
	Parent->PanelReferences.SetNum(Parent->PanelReferences.Num()-PanelIndices.Num());
	return true;
}

FEditorQueueOrderNetworkable FEditorQueueOrderPanelReference::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net(EEditorQueueOrderType::PanelReference);
	Net.Args.Add(Parent->AddPointer(Container));
	Net.Args.Append(PanelIndices);
	return Net;
}

bool FEditorQueueOrderPanelReference::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex)) { return false; }
	FEditorQueueOrderNetworkable& InData = Parent->Orders[OrderIndex];
	if (InData.Args.Num() <1)
	{
		return false;
	}
	Container = Cast<UNoxelContainer>(Parent->GetPointer(InData.Args[0]));
	if (!IsValid(Container))
	{
		return false;
	}
	for (int i = 1; i < InData.Args.Num(); ++i)
	{
		PanelIndices.Add(InData.Args[i]);
	}
	return true;
}

TArray<UNoxelDataComponent*> FEditorQueueOrderPanelReference::GetAffectedDataComponents(FEditorQueue* Parent)
{
	return {};
}

FString FEditorQueueOrderPanelReference::ToString()
{
	FString PanelIndicesString;
	for (int i = 0; i < PanelIndices.Num(); ++i)
	{
		PanelIndicesString += FString::Printf(TEXT("%d; "), PanelIndices[i]);
	}
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; Container = %s; PanelIndices.Num() = %d; PanelIndices : %s"), *Container->GetName(), PanelIndices.Num(), *PanelIndicesString);
}

bool FEditorQueueOrderPanelAddRemove::DoAdd(FEditorQueue* Parent)
{
	bool valid = true; int i;
	for (i = 0; i < PanelIndexRef.Num(); ++i)
	{
		if(!Parent->PanelReferences[PanelIndexRef[i]].Object->AddPanelDiffered(Parent->PanelReferences[PanelIndexRef[i]].PanelIndex))
		{
			valid = false;
			break;
		}
	}
	if (!valid)
	{
		for (int j = i - 1; j >= 0; --j)
		{
			Parent->PanelReferences[PanelIndexRef[j]].Object->RemovePanelDiffered(Parent->PanelReferences[PanelIndexRef[j]].PanelIndex);
		}
	}
	return valid;
}

bool FEditorQueueOrderPanelAddRemove::DoRemove(FEditorQueue* Parent)
{
	bool valid = true; int i;
	for (i = 0; i < PanelIndexRef.Num(); ++i)
	{
		if(!Parent->PanelReferences[PanelIndexRef[i]].Object->RemovePanelDiffered(Parent->PanelReferences[PanelIndexRef[i]].PanelIndex))
		{
			valid = false;
			break;
		}
	}
	if (!valid)
	{
		for (int j = i - 1; j >= 0; --j)
		{
			Parent->PanelReferences[PanelIndexRef[j]].Object->AddPanelDiffered(Parent->PanelReferences[PanelIndexRef[j]].PanelIndex);
		}
	}
	return valid;
}

bool FEditorQueueOrderPanelAddRemove::ExecuteOrder(FEditorQueue* Parent)
{
	if (Add)
	{
		return DoAdd(Parent);
	}
	else
	{
		return DoRemove(Parent);
	}
}

bool FEditorQueueOrderPanelAddRemove::UndoOrder(FEditorQueue* Parent)
{
	if (Add)
	{
		return DoRemove(Parent);
	}
	else
	{
		return DoAdd(Parent);
	}
}

FEditorQueueOrderNetworkable FEditorQueueOrderPanelAddRemove::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net(Add ? EEditorQueueOrderType::PanelAdd : EEditorQueueOrderType::PanelRemove);
	Net.Args.Append(PanelIndexRef);
	return Net;
}

bool FEditorQueueOrderPanelAddRemove::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex)) { return false; }
	FEditorQueueOrderNetworkable InData = Parent->Orders[OrderIndex];
	PanelIndexRef = InData.Args;
	Add = OrderType == EEditorQueueOrderType::PanelAdd;
	return true;
}

TArray<UNoxelDataComponent*> FEditorQueueOrderPanelAddRemove::GetAffectedDataComponents(FEditorQueue* Parent)
{
	TArray<UNoxelDataComponent*> affected;
	for (auto RefIdx : PanelIndexRef)
	{
		affected.AddUnique(Cast<UNoxelDataComponent>(Parent->PanelReferences[RefIdx].Object));
	}
	return affected;
}

FString FEditorQueueOrderPanelAddRemove::ToString()
{
	FString PanelRefString;
	for (int i = 0; i < PanelIndexRef.Num(); ++i)
	{
		PanelRefString += FString::Printf(TEXT("%d; "), PanelIndexRef[i]);
	}
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; Add = %s; PanelIndexRef.Num() = %d : {%s}"),
		Add ? TEXT("True") : TEXT("False"), PanelIndexRef.Num(), *PanelRefString);
}

TArray<FPanelID> FEditorQueueOrderPanelAddRemove::GetReservedPanelsUsed(FEditorQueue* Parent)
{
	TArray<FPanelID> Used;
	if (Add)
	{
		for (int i = 0; i < PanelIndexRef.Num(); ++i)
        {
        	Used.Add(Parent->PanelReferences[PanelIndexRef[i]]);
        }
	}
	return Used;
}

bool FEditorQueueOrderPanelProperties::ExecuteOrder(FEditorQueue* Parent)
{
	const int32 NumPanels = PanelIndexRef.Num();
	ThicknessNormalBefore.SetNum(NumPanels);
	ThicknessAntiNormalBefore.SetNum(NumPanels);
	VirtualBefore.SetNum(NumPanels);
	bool bValid = true; int i;
	for (i = 0; i < NumPanels; ++i)
	{
		FPanelData Data;
        FPanelID PanelID = Parent->PanelReferences[PanelIndexRef[i]];
        if (!PanelID.Object->GetPanelByPanelIndex(PanelID.PanelIndex, Data))
        {
        	bValid = false;
			break;
        }
		ThicknessNormalBefore[i] = Data.ThicknessNormal;
        ThicknessAntiNormalBefore[i] = Data.ThicknessAntiNormal;
        VirtualBefore[i] = Data.Virtual;
        PanelID.Object->SetPanelPropertiesDiffered(PanelID.PanelIndex, ThicknessNormalAfter, ThicknessAntiNormalAfter, VirtualAfter);
	}
	if (!bValid)
	{
		for (int j = i - 1; j >= 0; --j)
		{
			FPanelID PanelID = Parent->PanelReferences[PanelIndexRef[j]];
			PanelID.Object->SetPanelPropertiesDiffered(PanelID.PanelIndex, ThicknessNormalBefore[j], ThicknessAntiNormalBefore[j], VirtualBefore[j]);
		}
	}
	return bValid;
}

bool FEditorQueueOrderPanelProperties::UndoOrder(FEditorQueue* Parent)
{
	bool bValid = true; int j;
	for (j = PanelIndexRef.Num() - 1; j >= 0; --j)
	{
		FPanelID PanelID = Parent->PanelReferences[PanelIndexRef[j]];
		if (!PanelID.Object->SetPanelPropertiesDiffered(PanelID.PanelIndex, ThicknessNormalBefore[j], ThicknessAntiNormalBefore[j], VirtualBefore[j]))
		{
			bValid = false;
			break;
		}
	}
	if (!bValid)
	{
		for (int i = j; i < PanelIndexRef.Num(); ++i)
		{
			FPanelID PanelID = Parent->PanelReferences[PanelIndexRef[i]];
			PanelID.Object->SetPanelPropertiesDiffered(PanelID.PanelIndex, ThicknessNormalAfter, ThicknessAntiNormalAfter, VirtualAfter);
		}
	}
	return bValid;
}

FEditorQueueOrderNetworkable FEditorQueueOrderPanelProperties::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net(EEditorQueueOrderType::PanelProperties);
	Net.AddFloat(ThicknessNormalAfter);
	Net.AddFloat(ThicknessAntiNormalAfter);
	Net.Args.Add(VirtualAfter);
	Net.Args.Append(PanelIndexRef);
	return Net;
}

bool FEditorQueueOrderPanelProperties::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex))
	{
		return false;
	}
	FEditorQueueOrderNetworkable InData = Parent->Orders[OrderIndex];
	if (InData.Args.Num()<3)
	{
		return false;
	}
	ThicknessNormalAfter = InData.GetFloat(0);
	ThicknessAntiNormalAfter = InData.GetFloat(1);
	VirtualAfter = InData.Args[2];
	PanelIndexRef.SetNum(InData.Args.Num()-3);
	for (int i = 0; i < PanelIndexRef.Num(); ++i)
	{
		PanelIndexRef[i] = InData.Args[i+3];
	}
	return true;
}

TArray<UNoxelDataComponent*> FEditorQueueOrderPanelProperties::GetAffectedDataComponents(FEditorQueue* Parent)
{
	TArray<UNoxelDataComponent*> affected;
	for (auto RefIdx : PanelIndexRef)
	{
		affected.AddUnique(Cast<UNoxelDataComponent>(Parent->PanelReferences[RefIdx].Object));
	}
	return affected;
}

FString FEditorQueueOrderPanelProperties::ToString()
{
	FString PanelRefString;
	for (int i = 0; i < PanelIndexRef.Num(); ++i)
	{
		PanelRefString += FString::Printf(TEXT("%d; "), PanelIndexRef[i]);
	}
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; ThicknessNormal = %f; ThicknessAntiNormal = %f; Virtual = %s; PanelIndexRef.Num() = %d : {%s}"),
		ThicknessNormalAfter, ThicknessAntiNormalAfter, VirtualAfter ? TEXT("True") : TEXT("False"), PanelIndexRef.Num(), *PanelRefString);
	
}
