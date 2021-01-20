//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/NoxelContainer.h"
#include "Noxel.h"
#include "Noxel/NoxelDataStructs.h"
#include "Noxel/NodesContainer.h"
#include "NoxelRMCProvider.h"
#include "Kismet/KismetMathLibrary.h"

#include "NoxelPlayerController.h"



UNoxelContainer::UNoxelContainer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	SetCollisionProfileName(TEXT("Noxel"));
	MaxIndex = INT32_MIN;

	NoxelProvider = CreateDefaultSubobject<UNoxelRMCProvider>("NoxelProvider");
}

void UNoxelContainer::OnRegister()
{
	Super::OnRegister();
	Initialize(NoxelProvider);
}

void UNoxelContainer::BeginPlay()
{
	Super::BeginPlay();
	if ((ANoxelPlayerController*)GetWorld()->GetFirstPlayerController()) {
		((ANoxelPlayerController*)GetWorld()->GetFirstPlayerController())->SynchroniseNoxel(this);
	}
	switch (SpawnContext)
	{
	case ECraftSpawnContext::None:
		UE_LOG(Noxel, Log, TEXT("[UNoxelContainer::BeginPlay] None"));
		break;
	case ECraftSpawnContext::Editor:
		UE_LOG(Noxel, Log, TEXT("[UNoxelContainer::BeginPlay] Editor"));
		break;
	case ECraftSpawnContext::Battle:
		UE_LOG(Noxel, Log, TEXT("[UNoxelContainer::BeginPlay] Battle"));
		//SetCollisionUseComplexAsSimple(false);
		break;
	default:
		break;
	}
}

void UNoxelContainer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UNoxelContainer::FindPanelsByNodes(TArray<FNodeID>& Nodes, TArray<int32>& OutPanels, TArray<int32>& OutOccurences, 
	TArray<TArray<FNodeID>>& OutNodesAttachedBy)
{
	OutPanels.Empty();
	OutOccurences.Empty();
	for (FNodeID Node : Nodes)
	{
		if (Node.Object)
		{
			TArray<int32> Attached = Node.Object->GetAttachedPanels(Node.Location); //Array of PanelIndex
			for (int32 AttachedPanelIndex : Attached)
			{
				int32 IndexInArray = OutPanels.Find(AttachedPanelIndex);
				if (IndexInArray != INDEX_NONE)
				{
					OutOccurences[IndexInArray]++;
					OutNodesAttachedBy[IndexInArray].Add(Node);
				}
				else
				{
					OutPanels.Add(AttachedPanelIndex);
					OutOccurences.Add(1);
					OutNodesAttachedBy.Add(TArray<FNodeID>({ Node }));
				}
			}
		}
	}
	return OutPanels.Num() > 0;
}

bool UNoxelContainer::GetIndexOfPanelByPanelIndex(int32 PanelIndex, int32 & OutIndexInArray)
{
	//Find the panel
	for (int32 PanelIdx = 0; PanelIdx < Panels.Num(); PanelIdx++)
	{
		FPanelData Panel = Panels[PanelIdx];
		if (Panel.PanelIndex == PanelIndex) //Panel matches indices
		{
			OutIndexInArray = PanelIdx;
			return true;
		}
	}
	return false;
}

bool UNoxelContainer::AddPanel(FPanelData data)
{
	UE_LOG(NoxelData, Log, TEXT("[UNoxelContainer::AddPanel] Adding panel"));
	int32 NumNodes = data.Nodes.Num();
	if (data.ThicknessNormal < 0 || data.ThicknessAntiNormal < 0 || (data.ThicknessNormal == 0 && data.ThicknessAntiNormal == 0 ))//Invalid thickness
	{
		UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::AddPanel] Panel has invalid thicknesses"));
		return false;
	}
	if (NumNodes < 3) //Can't have less than 3 nodes
	{
		UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::AddPanel] Panel has not enough nodes"));
		return false;
	}
	for (FNodeID Node : data.Nodes) //Check that nodes can connect to this container
	{
		if (Node.Object)
		{
			UNoxelContainer* AttachedNoxel = Node.Object->GetAttachedNoxel();
			if (AttachedNoxel != this && AttachedNoxel) //If the node is attached to another valid container
			{
				UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::AddPanel] Nodes container can't be attached to this panel"));
				return false;
			}
		}
		else
		{
			UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::AddPanel] Node object is invalid"));
			return false; //Invalid node, ew
		}
	}
	TArray<int32> AdjacentPanels, Occurences;
	TArray<TArray<FNodeID>> NodesAttachedBy;
	FindPanelsByNodes(data.Nodes, AdjacentPanels, Occurences, NodesAttachedBy);
	int32 IndexOfMaxOccurences, MaxValue;
	UKismetMathLibrary::MaxOfIntArray(Occurences, IndexOfMaxOccurences, MaxValue);
	if (MaxValue > 2) //Two panel can't have more than 2 verts in common //TODO
	{
		UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::AddPanel] Another panel already has more than two of the nodes"));
		return false;
	}

	//At this point, the starting arguments are valid
	TArray<FVector> NodeLocationsRelativeToNoxel;
	for (FNodeID Node : data.Nodes)
	{
		NodeLocationsRelativeToNoxel.Add(GetComponentTransform().InverseTransformPosition(Node.ToWorld()));
	}
	UNoxelRMCProvider::PlaneFit(NodeLocationsRelativeToNoxel, data.Center, data.Normal);
	TArray<int32> NewNodesIndex;
	UNoxelRMCProvider::ReorderNodes(NodeLocationsRelativeToNoxel, data.Center, data.Normal, NewNodesIndex);
	TArray<FNodeID> NewNodes;
	NewNodes.Reserve(NumNodes);
	for (int32 CurrentNodeIdx = 0; CurrentNodeIdx < NumNodes; CurrentNodeIdx++)
	{
		NewNodes.Add(data.Nodes[NewNodesIndex[CurrentNodeIdx]]);
	}
	data.Nodes = NewNodes;
	data.Area = UNoxelRMCProvider::ComputeTriangleFanArea(data.Center, NodeLocationsRelativeToNoxel);

	//At this point, all the needed geometry data has been computed
	//Let's give it an index :
	if (UnusedIndices.Num() != 0)
	{
		UE_LOG(NoxelData, Log, TEXT("[UNoxelContainer::AddPanel] Index given is recycled"));
		data.PanelIndex = UnusedIndices.Pop();
	}
	else
	{
		UE_LOG(NoxelData, Log, TEXT("[UNoxelContainer::AddPanel] Index given is new"));
		data.PanelIndex = MaxIndex + 1;
		MaxIndex++;
	}
	//Do connections : If the panel shares two consecutive nodes with a neighbour, they are connected
	//Also connect nodes
	for (int32 NodeIdx = 0; NodeIdx < NumNodes; NodeIdx++)
	{
		FNodeID Node = data.Nodes[NodeIdx];
		Node.Object->AttachNode(Node, FPanelID(this, data.PanelIndex));
	}
	for (int32 PanelIdx = 0; PanelIdx < AdjacentPanels.Num(); PanelIdx++) //index of the arrays AdjacentPanels, OccurencesTArray and NodesAttachedBy
	{
		if (Occurences[PanelIdx] == 2)
		{
			int32 OtherPanelIndex = AdjacentPanels[PanelIdx]; //This is the PanelIndex, not the index of the panel in the Panels array
			int32 OtherIdx;
			GetIndexOfPanelByPanelIndex(OtherPanelIndex, OtherIdx);
			int32 NumNodesOther = Panels[OtherIdx].Nodes.Num();
			FNodeID Node0 = NodesAttachedBy[PanelIdx][0];
			FNodeID Node1 = NodesAttachedBy[PanelIdx][1];
			int32 Node0IdxOther = Panels[OtherIdx].Nodes.Find(Node0);
			int32 Node1IdxOther = Panels[OtherIdx].Nodes.Find(Node1);
			int32 Node0Idx = data.Nodes.Find(Node0);
			int32 Node1Idx = data.Nodes.Find(Node1);
			int32 AbsDeltaOtherNodes = abs(Node0IdxOther - Node1IdxOther); //Either 1 or NumNodesOther-1 if adjacent
			int32 AbsDeltaNodes = abs(Node0Idx - Node1Idx); //Either 1 or NumNode-1 if adjacent
			bool OtherAdjacent = (AbsDeltaOtherNodes == 1) || (AbsDeltaOtherNodes == (NumNodesOther - 1));
			bool Adjacent = (AbsDeltaNodes == 1) || (AbsDeltaNodes == (NumNodes - 1));
			if (Adjacent && OtherAdjacent) //Panels share an edge
			{
				UE_LOG(NoxelData, Log, TEXT("[UNoxelContainer::AddPanel] Panel %i shares an edge with another panel %i"), data.PanelIndex, OtherPanelIndex);
				Panels[OtherIdx].ConnectedPanels.AddUnique(data.PanelIndex);
				data.ConnectedPanels.AddUnique(Panels[OtherIdx].PanelIndex);
			}
		}
	}
	Panels.Add(data);
	UpdateProviderData();
	return true;
}

bool UNoxelContainer::RemovePanel(int32 index)
{
	//Find the panel
	for (int32 PanelIdx = 0; PanelIdx < Panels.Num(); PanelIdx++)
	{
		FPanelData Panel = Panels[PanelIdx];
		if (Panel.PanelIndex == index) //Panel matches indices
		{
			for (int32 OtherPanelIdx = 0; OtherPanelIdx < Panels.Num(); OtherPanelIdx++)
			{
				Panels[OtherPanelIdx].ConnectedPanels.Remove(index); //Remove all traces of the panel from other panels
			}
			for (FNodeID Node : Panel.Nodes)
			{
				if (Node.Object)
				{
					Node.Object->DetachNode(Node, FPanelID(this, index)); //Detach nodes
				}
			}
			Panels.RemoveAt(PanelIdx);
			UnusedIndices.Add(PanelIdx); //Free the index for use later
			UpdateProviderData();
			return true;
		}
	}
	return false;
}

bool UNoxelContainer::GetPanelByPanelIndex(int32 PanelIndex, FPanelData& FoundPanel)
{
	//Find the panel
	int32 IndexInArray;
	bool ReturnValue = GetIndexOfPanelByPanelIndex(PanelIndex, IndexInArray);
	if (ReturnValue)
	{
		FoundPanel = Panels[IndexInArray];
	}
	return ReturnValue;
}

bool UNoxelContainer::GetPanelByNodes(TArray<FNodeID> Nodes, int32 & PanelIndex)
{
	TSet<int32> CommonPanelIndices;
	if (Nodes.Num() >= 3)
	{
		if (!Nodes[0].IsValid())
		{
			return false;
		}
		CommonPanelIndices = TSet<int32>(Nodes[0].Object->GetAttachedPanels(Nodes[0].Location));
		for (int32 i = 1; i < Nodes.Num(); i++)
		{
			if (!Nodes[i].IsValid())
			{
				return false;
			}
			if (Nodes[i].Object->GetAttachedNoxel() != Nodes[0].Object->GetAttachedNoxel()) //check that they are attached to the same noxel container
			{
				return false;
			}
			CommonPanelIndices = CommonPanelIndices.Intersect(TSet<int32>(Nodes[i].Object->GetAttachedPanels(Nodes[i].Location)));
		}
		for (auto PanelIdx : CommonPanelIndices)
		{
			PanelIndex = PanelIdx;
			return true;
		}
	}
	return false;
}

TArray<FPanelData> UNoxelContainer::GetPanels()
{
	return Panels;
}

bool UNoxelContainer::getPanelHit(FHitResult hit, FPanelID & PanelHit)
{
	
	if (hit.Component == this)
	{
		PanelHit.Object = this;
		if (NoxelProvider->GetPanelIndexHit(hit.FaceIndex, PanelHit.PanelIndex))
		{
			return true;
		}
	}
	return false;
}

TArray<UNodesContainer*> UNoxelContainer::GetConnectedNodesContainers()
{
	return ConnectedNodesContainers;
}

void UNoxelContainer::Empty()
{
	for (FPanelData panel : Panels)
	{
		for (FNodeID node : panel.Nodes)
		{
			if (node.Object)
			{
				node.Object->DetachNode(node, FPanelID(this, panel.PanelIndex));
			}
		}
	}
	Panels.Empty();
	UnusedIndices.Empty();
	ConnectedNodesContainers.Empty();
	MaxIndex = INT32_MIN;
}

void UNoxelContainer::UpdateProviderData()
{
	TArray<FVector> NodeData;
	TArray<FNoxelRendererPanelData> PanelsData;
	PanelsData.Reserve(Panels.Num());
	TMap<FNodeID, int32> NodeRedirectorMap;
	TMap<int32, int32> PanelRedirectorMap;
	for (FPanelData Panel : Panels)
	{
		TArray<int32> Nodes;
		for (FNodeID Node : Panel.Nodes)
		{
			int32* NodeIdxPtr = NodeRedirectorMap.Find(Node);
			if (NodeIdxPtr != nullptr)
			{
				Nodes.Add(*NodeIdxPtr);
			}
			else
			{
				int32 NodeIdx = NodeData.Add(GetComponentTransform().InverseTransformPosition(Node.ToWorld())); //Go to world then back to this container's location
				NodeRedirectorMap.Add(Node, NodeIdx);
				Nodes.Add(NodeIdx);
			}
		}
		FNoxelRendererPanelData PanelData(Panel.PanelIndex, Nodes, Panel.ThicknessNormal, Panel.ThicknessAntiNormal, Panel.Area, Panel.Normal, Panel.Center, Panel.ConnectedPanels);
		int32 PanelIdx = PanelsData.Add(PanelData);
		PanelRedirectorMap.Add(Panel.PanelIndex, PanelIdx);
	}
	for (int32 PanelIdx = 0; PanelIdx < PanelsData.Num(); PanelIdx++) //Convert from PanelIndex to index in the array
	{
		FNoxelRendererPanelData& Panel = PanelsData[PanelIdx];
		for (int32 i = 0; i < Panel.AdjacentPanels.Num(); i++)
		{
			Panel.AdjacentPanels[i] = PanelRedirectorMap[Panel.AdjacentPanels[i]];
		}
		PanelsData[PanelIdx] = Panel; //set it back just in case
	}
	NoxelProvider->SetNodes(NodeData);
	NoxelProvider->SetPanels(PanelsData);
}
