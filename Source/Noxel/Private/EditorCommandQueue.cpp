// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "EditorCommandQueue.h"

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
 	case EEditorQueueOrderType::PanelAdd:
 	case EEditorQueueOrderType::PanelRemove:
 		order = new FEditorQueueOrderPanelAddRemove();
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
	for (int i = 0; i < Orders.Num(); ++i)
	{
		delete Orders[i];
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
		bool success;
		if (bShouldExecute)
		{
			success = Orders[OrderIdx]->ExecuteOrder(this);
		}
		else
		{
			success = Orders[OrderIdx]->UndoOrder(this);
		}
		TArray<UNoxelDataComponent*> Affected = Orders[OrderIdx]->GetAffectedDataComponents(this);
		for (UNoxelDataComponent* NoxelDataComponent : Affected)
		{
			AffArr.AddUnique(NoxelDataComponent);
		}
		if (!success)
		{
			UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueue::RunQueue(%d)@%p] Failed at %d on instruction %s"),
				bShouldExecute, this, OrderIdx, *Orders[OrderIdx]->ToString());
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
			if (bShouldExecute)
			{
				Orders[OrderIdx]->UndoOrder(this);
			}
			else
			{
				Orders[OrderIdx]->ExecuteOrder(this);
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
			ContainerIdx = Containers.Add(Node.Object);
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

void FEditorQueue::AddPanelAddOrder(UNoxelContainer* InObject,
	const float InThicknessNormal, const float InThicknessAntiNormal, const bool InVirtual)
{
	FEditorQueueOrderPanelAddRemove* order = new FEditorQueueOrderPanelAddRemove(InObject, InThicknessNormal, InThicknessAntiNormal, InVirtual, true);
	Orders.Add(order);
}

void FEditorQueue::AddNodeConnectOrder(TArray<int32> Nodes, TArray<int32> Panels)
{
	FEditorQueueOrderNodeDisConnect* order = new FEditorQueueOrderNodeDisConnect(Nodes, Panels, true);
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
	Net.Args.Add(Locations.Num());
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
	FEditorQueueOrderNetworkable& InData = Parent->Orders[OrderIndex];
	if (InData.Args.Num()<2)
	{
		return false;
	}
	int32 NumLocations = InData.Args[0];
	if (InData.Args.Num() != NumLocations * 3 + 2)
	{
		return false;
	}
	Container = Cast<UNodesContainer>(Parent->GetPointer(InData.Args[1]));
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
	FEditorQueueOrderNetworkable& InData = Parent->Orders[OrderIndex];
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
	return FEditorQueueOrderTemplate::ToString(); //TODO
}

bool FEditorQueueOrderPanelAddRemove::DoAdd(FEditorQueue* Parent)
{
	if (Object)
	{
		int32 Index;
		if(Object->AddPanelDiffered(FPanelData(TArray<FNodeID>(), ThicknessNormal, ThicknessAntiNormal, Virtual), Index))
		{
			if (Add) //Execute, store data for undo
			{
				PanelIndex = Parent->PanelReferences.Add(FPanelID(Object, Index));
			}
			return true;
		}
	}
	return false;
}

bool FEditorQueueOrderPanelAddRemove::DoRemove(FEditorQueue* Parent)
{
	if (Object)
	{
		FPanelData data;
		if(Object->GetPanelByPanelIndex(Parent->PanelReferences[PanelIndex], data))
		{
			if (Object->RemovePanelDiffered(Parent->PanelReferences[PanelIndex]))
			{
				if (!Add) //Execute, store data for undo
				{
					ThicknessNormal = data.ThicknessNormal;
					ThicknessAntiNormal = data.ThicknessAntiNormal;
					Virtual = data.Virtual;
				}
			}
		}
		
	}
	return false;
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
	Net.Args.Add(Parent->AddPointer(Object));
	Net.AddFloat(ThicknessNormal); Net.AddFloat(ThicknessAntiNormal);
	Net.Args.Append({ Virtual, PanelIndex });
	return Net;
}

bool FEditorQueueOrderPanelAddRemove::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex)) { return false; }
	FEditorQueueOrderNetworkable& InData = Parent->Orders[OrderIndex];
	int32 arglen = InData.Args.Num();
	if (arglen<5) {
		return false;
	}
	Object = Cast<UNoxelContainer>(Parent->GetPointer( InData.Args[0]));
	if (!IsValid(Object))
	{
		return false;
	}
	ThicknessNormal = InData.GetFloat(1);
	ThicknessAntiNormal = InData.GetFloat(2);
	Virtual = InData.Args[3];
	PanelIndex = InData.Args[4];
	Add = OrderType == EEditorQueueOrderType::PanelAdd;
	return true;
}

TArray<UNoxelDataComponent*> FEditorQueueOrderPanelAddRemove::GetAffectedDataComponents(FEditorQueue* Parent)
{
	TArray<UNoxelDataComponent*> affected;
	affected.Add(Cast<UNoxelDataComponent>(Object));
	return affected;
}

FString FEditorQueueOrderPanelAddRemove::ToString()
{
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; Object Name = %s; ThicknessNormal = %f; ThicknessAntiNormal = %f; Virtual = %d; Add = %d"),
		*Object->GetName(), ThicknessNormal, ThicknessAntiNormal, Virtual, Add);
}
