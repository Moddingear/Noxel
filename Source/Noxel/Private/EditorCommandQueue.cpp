// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "EditorCommandQueue.h"
#include "Noxel/NoxelDataStructs.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"

void FEditorQueueNetworkable::AddOrder(FEditorQueueOrderTemplate * Order)
{
	Orders.Add(Order->ToNetworkable());
}

bool FEditorQueueNetworkable::OrderFromNetworkable(FEditorQueueOrderNetworkable Networkable, FEditorQueueOrderTemplate * OutOrder)
{
	switch (Networkable.OrderType)
	{
	case EEditorQueueOrderType::NodeAdd:
	{
		FEditorQueueOrderNodeAdd order = FEditorQueueOrderNodeAdd();
		if (order.FromNetworkable(Networkable))
		{
			OutOrder = &order;
			return true;
		}
	}
	break;
	case EEditorQueueOrderType::NodeRemove:
	{
		FEditorQueueOrderNodeRemove order = FEditorQueueOrderNodeRemove();
		if (order.FromNetworkable(Networkable))
		{
			OutOrder = &order;
			return true;
		}
	}
	break;
	case EEditorQueueOrderType::PanelAdd:
	{
		FEditorQueueOrderPanelAdd order = FEditorQueueOrderPanelAdd();
		if (order.FromNetworkable(Networkable))
		{
			OutOrder = &order;
			return true;
		}
	}
	break;
	default:
		break;
	}
	return false;
}

void FEditorQueueNetworkable::AddNodeAddOrder(FNodeID NodeToAdd)
{
	FEditorQueueOrderNodeAdd order(NodeToAdd);
	AddOrder(&order);
}

void FEditorQueueNetworkable::AddNodeRemoveOrder(FNodeID NodeToRemove)
{
	FEditorQueueOrderNodeRemove order(NodeToRemove);
	AddOrder(&order);
}

void FEditorQueueNetworkable::AddPanelAddOrder(UNoxelContainer * InObject, const TArray<FNodeID>& InNodes, const float InThicknessNormal, const float InThicknessAntiNormal, const bool InVirtual)
{
	FEditorQueueOrderPanelAdd order(InObject, InNodes, InThicknessNormal, InThicknessAntiNormal, InVirtual);
	AddOrder(&order);
}


bool FEditorQueueOrderNodeAdd::ExecuteOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList)
{
	return UNodesContainer::AddNode(NodeToAdd);
}

bool FEditorQueueOrderNodeAdd::UndoOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList)
{
	return UNodesContainer::RemoveNode(NodeToAdd);
}

FEditorQueueOrderNetworkable FEditorQueueOrderNodeAdd::ToNetworkable()
{
	FEditorQueueOrderNetworkable Net(EEditorQueueOrderType::NodeAdd);
	Net.AddNode(NodeToAdd);
	return Net;
}

bool FEditorQueueOrderNodeAdd::FromNetworkable(FEditorQueueOrderNetworkable & InData)
{
	FEditorQueueOrderTemplate::FromNetworkable(InData);
	NodeToAdd = InData.PopNode();
	return true;
}

FString FEditorQueueOrderNodeAdd::ToString()
{
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; NodeToAdd = (%s)"), *NodeToAdd.ToString());
}

bool FEditorQueueOrderNodeRemove::ExecuteOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList)
{
	return UNodesContainer::RemoveNode(NodeToRemove);
}

bool FEditorQueueOrderNodeRemove::UndoOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList)
{
	return UNodesContainer::AddNode(NodeToRemove);
}

FEditorQueueOrderNetworkable FEditorQueueOrderNodeRemove::ToNetworkable()
{
	FEditorQueueOrderNetworkable Net(EEditorQueueOrderType::NodeRemove);
	Net.AddNode(NodeToRemove);
	return Net;
}

bool FEditorQueueOrderNodeRemove::FromNetworkable(FEditorQueueOrderNetworkable & InData)
{
	FEditorQueueOrderTemplate::FromNetworkable(InData);
	NodeToRemove = InData.PopNode();
	return true;
}

FString FEditorQueueOrderNodeRemove::ToString()
{
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; NodeToRemove = (%s)"), *NodeToRemove.ToString());
}

bool FEditorQueueOrderPanelAdd::ExecuteOrder(const TArray<FEditorQueueOrderTemplate*>& OrderList)
{
	if (Object)
	{
		return Object->AddPanel(FPanelData(Nodes, ThicknessNormal, ThicknessAntiNormal, Virtual));
	}
	return false;
}

bool FEditorQueueOrderPanelAdd::UndoOrder(const TArray<FEditorQueueOrderTemplate*>& OrderList)
{
	if (Object)
	{
		int32 PanelIndex;
		if (Object->GetPanelByNodes(Nodes, PanelIndex))
		{
			return Object->RemovePanel(PanelIndex);
		}
	}
	return false;
}

FEditorQueueOrderNetworkable FEditorQueueOrderPanelAdd::ToNetworkable()
{
	FEditorQueueOrderNetworkable Net(EEditorQueueOrderType::PanelAdd);
	Net.ptrArgs.Add(Object);
	Net.floatArgs.Append({ ThicknessNormal, ThicknessAntiNormal });
	Net.intArgs.Append({ Virtual, Nodes.Num() });
	for (int32 i = 0; i < Nodes.Num(); i++)
	{
		Net.AddNode(Nodes[i]);
	}
	return Net;
}

bool FEditorQueueOrderPanelAdd::FromNetworkable(FEditorQueueOrderNetworkable & InData)
{
	if (!FEditorQueueOrderTemplate::FromNetworkable(InData)) { return false; }
	int32 flen = InData.floatArgs.Num(), ilen = InData.intArgs.Num(), plen = InData.ptrArgs.Num();
	if (flen < 2 || ilen < 2) {
		return false;
	}
	Object = Cast<UNoxelContainer>(InData.ptrArgs[0]);
	ThicknessNormal = InData.floatArgs[0];
	ThicknessAntiNormal = InData.floatArgs[1];
	Virtual = InData.intArgs[0];
	int32 NumNodes = InData.intArgs[1];
	if (NumNodes * 3 + 2 != flen || NumNodes + 1 != plen)
	{
		return false;
	}
	Nodes.SetNumUninitialized(NumNodes, true);
	for (int32 i = 0; i < NumNodes; i++)
	{
		Nodes[NumNodes - 1 - i] = InData.PopNode();
	}
	return true;
}

FString FEditorQueueOrderPanelAdd::ToString()
{
	return FEditorQueueOrderTemplate::ToString() + FString::Printf(TEXT("; Object Name = %s; Nodes.Num() = %i; ThicknessNormal = %f; ThicknessAntiNormal = %f; Virtual = %i"),
		*Object->GetName(), Nodes.Num(), ThicknessNormal, ThicknessAntiNormal, Virtual);
}