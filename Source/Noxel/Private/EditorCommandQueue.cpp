// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "EditorCommandQueue.h"

#include "Noxel.h"
#include "NoxelDataAsset.h"
#include "Noxel/NoxelDataStructs.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"
#include "Connectors/ConnectorBase.h"
#include "NObjects/NObjectInterface.h"
#include "Noxel/CraftDataHandler.h"

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
 	case EEditorQueueOrderType::ConnectorConnect:
 	case EEditorQueueOrderType::ConnectorDisconnect:
 		order = new FEditorQueueOrderConnectorDisConnect();
 		break;
 	case EEditorQueueOrderType::ObjectAdd:
 		order = new FEditorQueueOrderAddObject();
 		break;
 	case EEditorQueueOrderType::ObjectMove:
 		order = new FEditorQueueOrderMoveObject();
 		break;
 	case EEditorQueueOrderType::ObjectRemove:
 		order = new FEditorQueueOrderRemoveObject();
 		break;
 	default:
 		UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueNetworkable::OrderFromNetworkable] Order type is invalid or unimplemented : %d"), Orders[OrderIndex].OrderType);
 		break;
 	}
	if (order != nullptr && order->FromNetworkable(this, OrderIndex))
	{
		*OutOrder = order;
		ensureAlwaysMsgf(order->OrderType == Orders[OrderIndex].OrderType,
			TEXT("[FEditorQueueNetworkable::OrderFromNetworkable] Order type mismatch for order %d in, given %d out"),
			Orders[OrderIndex].OrderType, order->OrderType);
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
		const bool success = OrderFromNetworkable(i, &Order);
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
	if (IsRun == bShouldExecute) //Avoid running twice in the same direction
	{
		return true;
	}
	IsRun = bShouldExecute;
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
	TMap<FNodeID, int32> NodeMap;
	for (FNodeID Node : Nodes)
	{
		const int32 ContainerIdx = Containers.Find(Node.Object);
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
			NodeMap.Add(Node, NodeRefIdx++);
		}
		AddNodeReferenceOrder(Locations, Containers[ContainerIdx]);
	}
	return NodeMap;
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

void FEditorQueue::AddObjectAddOrder(UCraftDataHandler* Craft, FString ObjectClass, FTransform Location)
{
	FEditorQueueOrderAddObject* order = new FEditorQueueOrderAddObject(Craft, ObjectClass, Location);
	Orders.Add(order);
}

void FEditorQueue::AddConnectorConnectOrder(TArray<UConnectorBase*> A, TArray<UConnectorBase*> B)
{
	FEditorQueueOrderConnectorDisConnect* order = new FEditorQueueOrderConnectorDisConnect(A, B, true);
	Orders.Add(order);
}

void FEditorQueue::AddConnectorDisconnectOrder(TArray<UConnectorBase*> A, TArray<UConnectorBase*> B)
{
	FEditorQueueOrderConnectorDisConnect* order = new FEditorQueueOrderConnectorDisConnect(A, B, false);
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

unsigned long FEditorQueue::GetSize()
{
	unsigned long OrdersSize = 0;
	for (FEditorQueueOrderTemplate* Order : Orders)
	{
		OrdersSize += Order->GetSize();
	}
	return sizeof(FEditorQueue) + Orders.GetAllocatedSize()
	+ NodeReferences.GetAllocatedSize() + PanelReferences.GetAllocatedSize()
	+ OrdersSize;
}

bool FEditorQueueOrderArrayTemplate::ExecuteOrder(FEditorQueue* Parent)
{
	if (PreArray(Parent))
	{
		const int RunToNum = GetArrayLength(Parent); bool bValid = true; int i;
		for (i = 0; i < RunToNum; ++i)
		{
			if (!ExecuteInArray(Parent, i))
			{
				bValid = false;
				break;
			}
		}
		if (!bValid)
		{
			for (int j = i - 1; j >= 0; --j)
            {
            	UndoInArray(Parent, j);
            }
		}
		return bValid;
	}
	return false;
}

bool FEditorQueueOrderArrayTemplate::UndoOrder(FEditorQueue* Parent)
{
	if (PreArray(Parent))
	{
		const int RunToNum = GetArrayLength(Parent); bool bValid = true; int i;
		for (i = RunToNum - 1; i >= 0; --i)
		{
			if (!UndoInArray(Parent, i))
			{
				bValid = false;
				break;
			}
		}
		if (!bValid)
		{
			for (int j = i+1; j < RunToNum; ++j)
			{
				ExecuteInArray(Parent, j);
			}
		}
		return bValid;
	}
	return false;
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
	const int32 NumLocations = (InData.Args.Num()-1)/3;
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

unsigned long FEditorQueueOrderNodeReference::GetSize()
{
	return sizeof(FEditorQueueOrderNodeReference) + Locations.GetAllocatedSize();
}

bool FEditorQueueOrderNodeAddRemove::PreArray(FEditorQueue* Parent)
{
	return true;
}

int FEditorQueueOrderNodeAddRemove::GetArrayLength(FEditorQueue* Parent)
{
	return NodesToAddRemove.Num();
}

bool FEditorQueueOrderNodeAddRemove::ExecuteInArray(FEditorQueue* Parent, int i)
{
	const int32 NodeToAddRemove = NodesToAddRemove[i];
	if(Parent->NodeReferences.IsValidIndex(NodeToAddRemove) && Parent->NodeReferences[NodeToAddRemove].Object->IsPlayerEditable())
	{
		if (Add)
		{
			return UNodesContainer::AddNode(Parent->NodeReferences[NodeToAddRemove]);
		}
		else
		{
			return UNodesContainer::RemoveNode(Parent->NodeReferences[NodeToAddRemove]);
		}
	}
	return false;
}

bool FEditorQueueOrderNodeAddRemove::UndoInArray(FEditorQueue* Parent, int i)
{
	const int32 NodeToAddRemove = NodesToAddRemove[i];
	if(Parent->NodeReferences.IsValidIndex(NodeToAddRemove) && Parent->NodeReferences[NodeToAddRemove].Object->IsPlayerEditable())
	{
		if (Add)
		{
			return UNodesContainer::RemoveNode(Parent->NodeReferences[NodeToAddRemove]);
		}
		else
		{
			return UNodesContainer::AddNode(Parent->NodeReferences[NodeToAddRemove]);
		}
	}
	return false;
}

FEditorQueueOrderNetworkable FEditorQueueOrderNodeAddRemove::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderArrayTemplate::ToNetworkable(Parent);
	Net.Args = NodesToAddRemove;
	return Net;
}

bool FEditorQueueOrderNodeAddRemove::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	FEditorQueueOrderArrayTemplate::FromNetworkable(Parent, OrderIndex);
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

unsigned long FEditorQueueOrderNodeAddRemove::GetSize()
{
	return sizeof(FEditorQueueOrderNodeAddRemove) + NodesToAddRemove.GetAllocatedSize();
}

bool FEditorQueueOrderNodeDisConnect::PreArray(FEditorQueue* Parent)
{
	return Panels.Num() == Nodes.Num();
}

int FEditorQueueOrderNodeDisConnect::GetArrayLength(FEditorQueue* Parent)
{
	return Nodes.Num();
}

bool FEditorQueueOrderNodeDisConnect::ExecuteInArray(FEditorQueue* Parent, int i)
{
	if (Parent->NodeReferences.IsValidIndex(Nodes[i]) && Parent->PanelReferences.IsValidIndex(Panels[i]))
	{
		if(IsValid(Parent->PanelReferences[Panels[i]].Object))
		{
			if (Connect)
			{
				return Parent->PanelReferences[Panels[i]].Object->ConnectNodeDiffered(Parent->PanelReferences[Panels[i]].PanelIndex, Parent->NodeReferences[Nodes[i]]);
			}
			else
			{
				return Parent->PanelReferences[Panels[i]].Object->DisconnectNodeDiffered(Parent->PanelReferences[Panels[i]].PanelIndex, Parent->NodeReferences[Nodes[i]]);
			}
		}
	}
	return false;
}

bool FEditorQueueOrderNodeDisConnect::UndoInArray(FEditorQueue* Parent, int i)
{
	if (Parent->NodeReferences.IsValidIndex(Nodes[i]) && Parent->PanelReferences.IsValidIndex(Panels[i]))
	{
		if(IsValid(Parent->PanelReferences[Panels[i]].Object))
		{
			if (Connect)
			{
				return Parent->PanelReferences[Panels[i]].Object->DisconnectNodeDiffered(Parent->PanelReferences[Panels[i]].PanelIndex, Parent->NodeReferences[Nodes[i]]);
			}
			else
			{
				return Parent->PanelReferences[Panels[i]].Object->ConnectNodeDiffered(Parent->PanelReferences[Panels[i]].PanelIndex, Parent->NodeReferences[Nodes[i]]);
			}
		}
	}
	return false;
}

FEditorQueueOrderNetworkable FEditorQueueOrderNodeDisConnect::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderArrayTemplate::ToNetworkable(Parent);
	for (int i = 0; i < FMath::Min(Nodes.Num(), Panels.Num()); ++i)
	{
		Net.Args.Add(Nodes[i]);
		Net.Args.Add(Panels[i]);
	}
	return Net;
}

bool FEditorQueueOrderNodeDisConnect::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	FEditorQueueOrderArrayTemplate::FromNetworkable(Parent, OrderIndex);
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

unsigned long FEditorQueueOrderNodeDisConnect::GetSize()
{
	return sizeof(FEditorQueueOrderNodeDisConnect) + Nodes.GetAllocatedSize() + Panels.GetAllocatedSize();
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
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderTemplate::ToNetworkable(Parent);
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

unsigned long FEditorQueueOrderPanelReference::GetSize()
{
	return sizeof(FEditorQueueOrderPanelReference) + PanelIndices.GetAllocatedSize();
}

bool FEditorQueueOrderPanelAddRemove::PreArray(FEditorQueue* Parent)
{
	return true;
}

int FEditorQueueOrderPanelAddRemove::GetArrayLength(FEditorQueue* Parent)
{
	return PanelIndexRef.Num();
}

bool FEditorQueueOrderPanelAddRemove::ExecuteInArray(FEditorQueue* Parent, int i)
{
	if (Parent->PanelReferences.IsValidIndex(PanelIndexRef[i]))
	{
		if (IsValid(Parent->PanelReferences[PanelIndexRef[i]].Object))
		{
			if (Add)
			{
				return Parent->PanelReferences[PanelIndexRef[i]].Object->AddPanelDiffered(Parent->PanelReferences[PanelIndexRef[i]].PanelIndex);
			}
			else
			{
				return Parent->PanelReferences[PanelIndexRef[i]].Object->RemovePanelDiffered(Parent->PanelReferences[PanelIndexRef[i]].PanelIndex);
			}
		}
	}
	return false;
}

bool FEditorQueueOrderPanelAddRemove::UndoInArray(FEditorQueue* Parent, int i)
{
	if (Parent->PanelReferences.IsValidIndex(PanelIndexRef[i]))
	{
		if (IsValid(Parent->PanelReferences[PanelIndexRef[i]].Object))
		{
			if (Add)
			{
				return Parent->PanelReferences[PanelIndexRef[i]].Object->RemovePanelDiffered(Parent->PanelReferences[PanelIndexRef[i]].PanelIndex);
			}
			else
			{
				return Parent->PanelReferences[PanelIndexRef[i]].Object->AddPanelDiffered(Parent->PanelReferences[PanelIndexRef[i]].PanelIndex);
			}
		}
	}
	return false;
}

FEditorQueueOrderNetworkable FEditorQueueOrderPanelAddRemove::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderArrayTemplate::ToNetworkable(Parent);
	Net.Args.Append(PanelIndexRef);
	return Net;
}

bool FEditorQueueOrderPanelAddRemove::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderArrayTemplate::FromNetworkable(Parent, OrderIndex)) { return false; }
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

unsigned long FEditorQueueOrderPanelAddRemove::GetSize()
{
	return sizeof(FEditorQueueOrderPanelAddRemove) + PanelIndexRef.GetAllocatedSize();
}

bool FEditorQueueOrderPanelProperties::PreArray(FEditorQueue* Parent)
{
	const int32 NumPanels = GetArrayLength(Parent);
	ThicknessNormalBefore.SetNum(NumPanels);
	ThicknessAntiNormalBefore.SetNum(NumPanels);
	VirtualBefore.SetNum(NumPanels);
	return true;
}

int FEditorQueueOrderPanelProperties::GetArrayLength(FEditorQueue* Parent)
{
	return PanelIndexRef.Num();
}

bool FEditorQueueOrderPanelProperties::ExecuteInArray(FEditorQueue* Parent, int i)
{
	FPanelData Data;
	FPanelID PanelID = Parent->PanelReferences[PanelIndexRef[i]];
	if (!PanelID.Object->GetPanelByPanelIndex(PanelID.PanelIndex, Data))
	{
		return false;
	}
	ThicknessNormalBefore[i] = Data.ThicknessNormal;
	ThicknessAntiNormalBefore[i] = Data.ThicknessAntiNormal;
	VirtualBefore[i] = Data.Virtual;
	return PanelID.Object->SetPanelPropertiesDiffered(PanelID.PanelIndex, ThicknessNormalAfter, ThicknessAntiNormalAfter, VirtualAfter);
}

bool FEditorQueueOrderPanelProperties::UndoInArray(FEditorQueue* Parent, int i)
{
	FPanelID PanelID = Parent->PanelReferences[PanelIndexRef[i]];
	return PanelID.Object->SetPanelPropertiesDiffered(PanelID.PanelIndex, ThicknessNormalBefore[i], ThicknessAntiNormalBefore[i], VirtualBefore[i]);
}

FEditorQueueOrderNetworkable FEditorQueueOrderPanelProperties::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderArrayTemplate::ToNetworkable(Parent);
	Net.AddFloat(ThicknessNormalAfter);
	Net.AddFloat(ThicknessAntiNormalAfter);
	Net.Args.Add(VirtualAfter);
	Net.Args.Append(PanelIndexRef);
	return Net;
}

bool FEditorQueueOrderPanelProperties::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderArrayTemplate::FromNetworkable(Parent, OrderIndex))
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
	VirtualAfter = InData.Args[2]!=0;
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

unsigned long FEditorQueueOrderPanelProperties::GetSize()
{
	return sizeof(FEditorQueueOrderPanelProperties)
	+ PanelIndexRef.GetAllocatedSize() + VirtualBefore.GetAllocatedSize()
	+ ThicknessNormalBefore.GetAllocatedSize() + ThicknessAntiNormalBefore.GetAllocatedSize();
}

bool FEditorQueueOrderConnectorDisConnect::PreArray(FEditorQueue* Parent)
{
	return A.Num() == B.Num();
}

int FEditorQueueOrderConnectorDisConnect::GetArrayLength(FEditorQueue* Parent)
{
	return A.Num();
}

bool FEditorQueueOrderConnectorDisConnect::ExecuteInArray(FEditorQueue* Parent, int i)
{
	if (!IsValid(A[i]) || !IsValid(B[i]))
	{
		return false;
	}
	if (Connect)
	{
		return A[i]->Connect(B[i]);
	}
	else
	{
		return A[i]->Disconnect(B[i]);
	}
}

bool FEditorQueueOrderConnectorDisConnect::UndoInArray(FEditorQueue* Parent, int i)
{
	if (!IsValid(A[i]) || !IsValid(B[i]))
	{
		return false;
	}
	if (Connect)
	{
		return A[i]->Disconnect(B[i]);
	}
	else
	{
		return A[i]->Connect(B[i]);
	}
}

FEditorQueueOrderNetworkable FEditorQueueOrderConnectorDisConnect::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderArrayTemplate::ToNetworkable(Parent);
	for (int i = 0; i < FMath::Min(A.Num(), B.Num()); ++i)
	{
		Net.Args.Add(Parent->AddPointer(A[i]));
		Net.Args.Add(Parent->AddPointer(B[i]));
	}
	return Net;
}

bool FEditorQueueOrderConnectorDisConnect::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderArrayTemplate::FromNetworkable(Parent, OrderIndex))
	{
		return false;
	}
	FEditorQueueOrderNetworkable InData = Parent->Orders[OrderIndex];
	if (InData.Args.Num() % 2 != 0) //check parity
	{
		return false;
	}
	for (int i = 0; i < InData.Args.Num(); ++i)
	{
		if (!IsValid(Parent->GetPointer(InData.Args[i])))
		{
			return false;
		}
		if (!Parent->GetPointer(InData.Args[i])->IsA<UConnectorBase>())
		{
			return false;
		}
	}
	for (int i = 0; i < InData.Args.Num()/2; ++i)
	{
		A.Add(Cast<UConnectorBase>(Parent->GetPointer(InData.Args[i*2])));
		B.Add(Cast<UConnectorBase>(Parent->GetPointer(InData.Args[i*2+1])));
	}
	return true;
}

FString FEditorQueueOrderConnectorDisConnect::ToString()
{
	FString ABString;
	for (int i = 0; i < FMath::Min(A.Num(), B.Num()); ++i)
	{
		ABString += FString::Printf(TEXT("%p<=>%p; "), A[i], B[i]);
	}
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; A.Num() = %d; B.Num() = %d; A/B : {%s}"),
		A.Num(), B.Num(), *ABString);
}

unsigned long FEditorQueueOrderConnectorDisConnect::GetSize()
{
	return sizeof(FEditorQueueOrderConnectorDisConnect) + A.GetAllocatedSize() + B.GetAllocatedSize();
}

bool FEditorQueueOrderAddObject::ExecuteOrder(FEditorQueue* Parent)
{
	if (IsValid(Craft))
	{
		if (Craft->GetWorld()->IsServer())
		{
			SpawnedObject = Craft->AddComponentFromComponentID(ObjectClass, ObjectTransform);
			return IsValid(SpawnedObject);
		}
		return true;
	}
	UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueOrderAddObject::ExecuteOrder] Craft is invalid"));
	return false;
}

bool FEditorQueueOrderAddObject::UndoOrder(FEditorQueue* Parent)
{
	if (IsValid(Craft))
	{
		if (Craft->GetWorld()->IsServer())
		{
			return Craft->RemoveComponentIfUnconnected(SpawnedObject);
		}
		return true;
	}
	UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueOrderAddObject::UndoOrder] Craft is invalid"));
	return false;
}

FEditorQueueOrderNetworkable FEditorQueueOrderAddObject::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderTemplate::ToNetworkable(Parent);
	const uint32 buffer32len = ObjectClass.Len()/4 + 1;
	const uint32 bufferlen = buffer32len*4;
	int32* buffer32 = new int32(buffer32len);
	uint8* buffer = reinterpret_cast<uint8*>(buffer32);
	FString BlobStr = ObjectClass;
	StringToBytes(ObjectClass, buffer, bufferlen);
	Net.Args.Add(Parent->AddPointer(Craft));
	Net.AddTransform(ObjectTransform);
	Net.Args.Add(ObjectClass.Len());
	Net.Args.Append(buffer32, buffer32len);
	delete buffer32;
	return Net;
}

bool FEditorQueueOrderAddObject::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex))
	{
		return false;
	}
	FEditorQueueOrderNetworkable InData = Parent->Orders[OrderIndex];
	if (InData.Args.Num() < FEditorQueueOrderNetworkable::GetTransformSize()+1)
	{
		return false;
	}
	Craft = Cast<UCraftDataHandler>(Parent->GetPointer(InData.Args[0]));
	ObjectTransform = InData.GetTransform(1);
	int32 ObjectClassLen = InData.Args[1 + FEditorQueueOrderNetworkable::GetTransformSize()];
	const int32 buffer32offset = FEditorQueueOrderNetworkable::GetTransformSize()+2;
	const int32 buffer32len = InData.Args.Num() - buffer32offset;
	int32* buffer32 = new int32(buffer32len);
	for (int32 i = 0; i < buffer32len; ++i)
	{
		buffer32[i] = InData.Args[i+buffer32offset];
	}
	uint8* buffer = reinterpret_cast<uint8*>(buffer32);
	const int32 bufferlen = buffer32len*4;
	ObjectClass = BytesToString(buffer, bufferlen);
	ObjectClass.LeftInline(ObjectClassLen);
	delete buffer32;
	return true;
}

FString FEditorQueueOrderAddObject::ToString()
{
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(
		TEXT("; Craft@%p; ObjectClass = %s; ObjectTransform = %s"), Craft, *ObjectClass, *ObjectTransform.ToHumanReadableString());
}

unsigned long FEditorQueueOrderAddObject::GetSize()
{
	return sizeof(FEditorQueueOrderAddObject) + ObjectClass.GetAllocatedSize();
}

bool FEditorQueueOrderMoveObject::ExecuteOrder(FEditorQueue* Parent)
{
	if (!IsValid(Craft) || !IsValid(ObjectToMove))
	{
		return false;
	}
	if (Craft->GetWorld()->IsServer())
	{
		OldObjectTransform = ObjectToMove->GetTransform();
		return Craft->MoveComponent(ObjectToMove, NewObjectTransform);
	}
	return !Craft->HasAnyDataComponentConnected(ObjectToMove);
}

bool FEditorQueueOrderMoveObject::UndoOrder(FEditorQueue* Parent)
{
	if (!IsValid(Craft) || !IsValid(ObjectToMove))
	{
		return false;
	}
	if (Craft->GetWorld()->IsServer())
	{
		return Craft->MoveComponent(ObjectToMove, OldObjectTransform);
	}
	return !Craft->HasAnyDataComponentConnected(ObjectToMove);
}

FEditorQueueOrderNetworkable FEditorQueueOrderMoveObject::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderTemplate::ToNetworkable(Parent);
	Net.Args.Add(Parent->AddPointer(Craft));
	Net.Args.Add(Parent->AddPointer(ObjectToMove));
	Net.AddTransform(NewObjectTransform);
	return Net;
}

bool FEditorQueueOrderMoveObject::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex))
	{
		return false;
	}
	FEditorQueueOrderNetworkable InData = Parent->Orders[OrderIndex];
	if (InData.Args.Num() < InData.GetTransformSize() +2)
	{
		return false;
	}
	Craft = Cast<UCraftDataHandler>(Parent->GetPointer(InData.Args[0]));
	ObjectToMove = Cast<AActor>(Parent->GetPointer(InData.Args[1]));
	NewObjectTransform = InData.GetTransform(2);
	return true;
}

FString FEditorQueueOrderMoveObject::ToString()
{
	return FEditorQueueOrderTemplate::ToString()
	+ FString::Printf(TEXT("; Craft@%p; ObjectToMove@%p; NewObjectTransform = %s"), Craft, ObjectToMove, *NewObjectTransform.ToString());
}

unsigned long FEditorQueueOrderMoveObject::GetSize()
{
	return sizeof(FEditorQueueOrderMoveObject);
}

bool FEditorQueueOrderRemoveObject::ExecuteOrder(FEditorQueue* Parent)
{
	if (IsValid(Craft))
	{
		if (Craft->GetWorld()->IsServer())
		{
			ObjectTransform = ObjectToRemove->GetTransform();
			if (ObjectToRemove->IsA<UNObjectInterface>())
			{
				ObjectMetadata = INObjectInterface::Execute_OnReadMetadata(ObjectToRemove, Craft->GetComponents());
			}
			ObjectClass = UNoxelDataAsset::getComponentIDFromClass(Craft->DataTable, ObjectToRemove->StaticClass());
			return Craft->RemoveComponentIfUnconnected(ObjectToRemove);
		}
		return true;
	}
	UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueOrderRemoveObject::ExecuteOrder] Craft is invalid"));
	return false;
}

bool FEditorQueueOrderRemoveObject::UndoOrder(FEditorQueue* Parent)
{
	if (IsValid(Craft))
	{
		if (Craft->GetWorld()->IsServer())
		{
			ObjectToRemove = Craft->AddComponentFromComponentID(ObjectClass, ObjectTransform);
			if (IsValid(ObjectToRemove))
			{
				if (ObjectToRemove->IsA<UNObjectInterface>())
				{
					INObjectInterface::Execute_OnWriteMetadata(ObjectToRemove, ObjectMetadata, Craft->GetComponents());
				}
				return true;
			}
		}
		return true;
	}
	UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueOrderRemoveObject::UndoOrder] Craft is invalid"));
	return false;
}

FEditorQueueOrderNetworkable FEditorQueueOrderRemoveObject::ToNetworkable(FEditorQueueNetworkable* Parent)
{
	FEditorQueueOrderNetworkable Net = FEditorQueueOrderTemplate::ToNetworkable(Parent);
	Net.Args.Add(Parent->AddPointer(Craft));
	Net.Args.Add(Parent->AddPointer(ObjectToRemove));
	return Net;
}

bool FEditorQueueOrderRemoveObject::FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
{
	if (!FEditorQueueOrderTemplate::FromNetworkable(Parent, OrderIndex))
	{
		return false;
	}
	FEditorQueueOrderNetworkable InData = Parent->Orders[OrderIndex];
	if (InData.Args.Num() < 2)
	{
		return false;
	}
	Craft = Cast<UCraftDataHandler>(Parent->GetPointer(InData.Args[0]));
	ObjectToRemove = Cast<AActor>(Parent->GetPointer(InData.Args[1]));
	return true;
}

FString FEditorQueueOrderRemoveObject::ToString()
{
	return FEditorQueueOrderTemplate::ToString()
    + FString::Printf(TEXT("; Craft@%p; ObjectToRemove@%p"), Craft, ObjectToRemove);
}

unsigned long FEditorQueueOrderRemoveObject::GetSize()
{
	return sizeof(FEditorQueueOrderRemoveObject) + ObjectClass.GetAllocatedSize() + ObjectMetadata.JsonString.GetAllocatedSize();
}
