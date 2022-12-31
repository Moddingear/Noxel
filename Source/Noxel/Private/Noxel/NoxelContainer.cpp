//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/NoxelContainer.h"
#include "Noxel.h"
#include "Noxel/NoxelDataStructs.h"
#include "Noxel/NodesContainer.h"
#include "NoxelRMCProvider.h"
#include "Kismet/KismetMathLibrary.h"

#include "NoxelPlayerController.h"
#include "Net/UnrealNetwork.h"


UNoxelContainer::UNoxelContainer()
	: Super()
{
	SetIsReplicatedByDefault(true);
	SetCollisionProfileName(TEXT("Noxel"));
	MaxIndex = INT32_MIN;

	NoxelProvider = CreateDefaultSubobject<UNoxelRMCProvider>("NoxelProvider");

	
}

void UNoxelContainer::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(UNoxelContainer, ConnectedNodesContainers, COND_InitialOnly);
}

void UNoxelContainer::OnRegister()
{
	Super::OnRegister();
	Initialize(NoxelProvider);
}

void UNoxelContainer::BeginPlay()
{
	Super::BeginPlay();
	
	/*switch (SpawnContext)
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
	}*/
}

void UNoxelContainer::OnRep_ConnectedNodesContainers()
{
	if (GetWorld()->IsServer())
	{
		return;
	}
	for (int i = 0; i < ConnectedNodesContainers.Num(); ++i)
	{
		if (!IsValid(ConnectedNodesContainers[i]))
		{
			return;
		}
	}
	if (static_cast<ANoxelPlayerController*>(GetWorld()->GetFirstPlayerController())) {
		static_cast<ANoxelPlayerController*>(GetWorld()->GetFirstPlayerController())->SynchroniseNoxel(this);
	}
	else
	{
		UE_LOG(NoxelDataNetwork, Error, TEXT("[UNoxelContainer::OnRep_ConnectedNodesContainers] Failed to get player controller for noxel synchronisation!"))
	}
}

void UNoxelContainer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UNoxelContainer::FindPanelsByNodes(TArray<FNodeID>& Nodes, TArray<int32>& OutPanels, TArray<int32>& OutOccurences, 
	TArray<TArray<FNodeID>>& OutNodesAttachedBy, TArray<int32> IgnoreFilter)
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
				if (IgnoreFilter.Contains(AttachedPanelIndex))
				{
					continue;
				}
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

bool UNoxelContainer::IsPanelValid(FPanelData &data)
{
	TArray<int32> AdjacentPanels, Occurrences;
	TArray<TArray<FNodeID>> NodesAttachedBy;
	return IsPanelValid(data, AdjacentPanels, Occurrences, NodesAttachedBy);
}

bool UNoxelContainer::IsPanelValid(FPanelData &data, TArray<int32> &AdjacentPanels,TArray<int32> &Occurrences, TArray<TArray<FNodeID>> &NodesAttachedBy)
{
	const int32 NumNodes = data.Nodes.Num();
    if (data.ThicknessNormal < 0 || data.ThicknessAntiNormal < 0 || (data.ThicknessNormal == 0 && data.ThicknessAntiNormal == 0 ))//Invalid thickness
    {
    	UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::IsPanelValid] Panel has invalid thicknesses"));
    	return false;
    }
    if (NumNodes < 3) //Can't have less than 3 nodes
    {
    	UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::IsPanelValid] Panel has not enough nodes"));
    	return false;
    }
    for (FNodeID Node : data.Nodes) //Check that nodes can connect to this container
    {
    	if (Node.Object)
    	{
    		UNoxelContainer* AttachedNoxel = Node.Object->GetAttachedNoxel();
    		if (AttachedNoxel != this && AttachedNoxel) //If the node is attached to another valid container
    		{
    			UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::IsPanelValid] Nodes container can't be attached to this panel"));
    			return false;
    		}
    	}
    	else
    	{
    		UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::IsPanelValid] Node object is invalid"));
    		return false; //Invalid node, ew
    	}
    }
	TArray<int32> IgnoreFilter;
	IgnoreFilter.Add(data.PanelIndex);
    FindPanelsByNodes(data.Nodes, AdjacentPanels, Occurrences, NodesAttachedBy, IgnoreFilter);
    int32 IndexOfMaxOccurrences, MaxValue;
    UKismetMathLibrary::MaxOfIntArray(Occurrences, IndexOfMaxOccurrences, MaxValue);
    if (MaxValue > 2) //Two panel can't have more than 2 verts in common //TODO
    {
    	UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::IsPanelValid] Another panel already has more than two of the nodes"));
    	return false;
    }
	return true;
}

void UNoxelContainer::ComputePanelGeometricData(FPanelData& data)
{
	const int32 NumNodes = data.Nodes.Num();
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
}

void UNoxelContainer::GetAdjacentPanelsFromNodes(FPanelData& data, const TArray<int32>& AdjacentPanels,
                                                 const TArray<int32>& Occurrences, const TArray<TArray<FNodeID>>& NodesAttachedBy)
{
	
	const int32 NumNodes = data.Nodes.Num();

	//Clear current connections
	for (int PanelIdx = data.ConnectedPanels.Num() - 1; PanelIdx >= 0; --PanelIdx)
	{
		int32 OtherPanelIndex = data.ConnectedPanels[PanelIdx]; //This is the PanelIndex, not the index of the panel in the Panels array
		int32 OtherIdx;
		GetIndexOfPanelByPanelIndex(OtherPanelIndex, OtherIdx);
		Panels[OtherIdx].ConnectedPanels.Remove(data.PanelIndex);
		data.ConnectedPanels.Remove(Panels[OtherIdx].PanelIndex);
		//UE_LOG(NoxelData, Log, TEXT("[UNoxelContainer::GetAdjacentPanelsFromNodes] Removing connection between panels %i and %i"), data.PanelIndex, OtherPanelIndex);
	}

	//Make new connections
	for (int32 PanelIdx = 0; PanelIdx < AdjacentPanels.Num(); PanelIdx++) //index of the arrays AdjacentPanels, OccurencesTArray and NodesAttachedBy
	{
		if (Occurrences[PanelIdx] == 2)
		{
			int32 OtherPanelIndex = AdjacentPanels[PanelIdx]; //This is the PanelIndex, not the index of the panel in the Panels array
			int32 OtherIdx;
			GetIndexOfPanelByPanelIndex(OtherPanelIndex, OtherIdx);
			const int32 NumNodesOther = Panels[OtherIdx].Nodes.Num();
			FNodeID Node0 = NodesAttachedBy[PanelIdx][0];
			FNodeID Node1 = NodesAttachedBy[PanelIdx][1];
			const int32 Node0IdxOther = Panels[OtherIdx].Nodes.Find(Node0);
			const int32 Node1IdxOther = Panels[OtherIdx].Nodes.Find(Node1);
			const int32 Node0Idx = data.Nodes.Find(Node0);
			const int32 Node1Idx = data.Nodes.Find(Node1);
			const int32 AbsDeltaOtherNodes = abs(Node0IdxOther - Node1IdxOther); //Either 1 or NumNodesOther-1 if adjacent
			const int32 AbsDeltaNodes = abs(Node0Idx - Node1Idx); //Either 1 or NumNode-1 if adjacent
			const bool OtherAdjacent = (AbsDeltaOtherNodes == 1) || (AbsDeltaOtherNodes == (NumNodesOther - 1));
			const bool Adjacent = (AbsDeltaNodes == 1) || (AbsDeltaNodes == (NumNodes - 1));
			if (Adjacent && OtherAdjacent) //Panels share an edge
			{
				//UE_LOG(NoxelData, Log, TEXT("[UNoxelContainer::GetAdjacentPanelsFromNodes] Panel %i shares an edge with another panel %i"), data.PanelIndex, OtherPanelIndex);
				Panels[OtherIdx].ConnectedPanels.AddUnique(data.PanelIndex);
				data.ConnectedPanels.AddUnique(Panels[OtherIdx].PanelIndex);
			}
		}
	}
}

int32 UNoxelContainer::GetNewPanelIndex()
{
	if (UnusedIndices.Num() != 0)
	{
		//UE_LOG(NoxelData, Log, TEXT("[UNoxelContainer::AddPanel] Index given is recycled"));
		return UnusedIndices.Pop();
	}
	else
	{
		//UE_LOG(NoxelData, Log, TEXT("[UNoxelContainer::AddPanel] Index given is new"));
		return ++MaxIndex;
	}
}

TArray<int32> UNoxelContainer::ReservePanelIndices(int32 Num)
{
	TArray<int32> NewReserved;
	for (int i = 0; i < Num; ++i)
	{
		NewReserved.Add(GetNewPanelIndex());
	}
	ReservedIndices.Append(NewReserved);
	return NewReserved;
}

bool UNoxelContainer::ReservePanelIndices(TArray<int32> IndicesToReserve)
{
	for (int32 Index : IndicesToReserve)
	{
		int32 IndexInArray;
		if (GetIndexOfPanelByPanelIndex(Index, IndexInArray))
		{
			return false;
		}
		if (ReservedIndices.Contains(Index))
		{
			return false;
		}
	}
	for (int32 Index : IndicesToReserve)
	{
		UnusedIndices.RemoveSwap(Index);
		ReservedIndices.Add(Index);
	}
	return true;
}


bool UNoxelContainer::AddPanelDiffered(int32 Index)
{
	FPanelData found;
	if (GetPanelByPanelIndex(Index, found))
	{
		return false;
	}
	FPanelData Data;
	Data.PanelIndex = Index;
	Panels.Add(Data);
	DifferedPanels.AddUnique(Index);
	ReservedIndices.RemoveSwap(Index);
	return true;
}

bool UNoxelContainer::FinishAddPanel(int32 Index)
{
	int32 IndexInArray;
	bool found = GetIndexOfPanelByPanelIndex(Index, IndexInArray);
	if (found)
	{
		FPanelData &data = Panels[IndexInArray];
		TArray<int32> AdjacentPanels, Occurrences;
		TArray<TArray<FNodeID>> NodesAttachedBy;
		if(!IsPanelValid(data, AdjacentPanels, Occurrences, NodesAttachedBy))
		{
			return false;
		}

		ComputePanelGeometricData(data);
		GetAdjacentPanelsFromNodes(data, AdjacentPanels, Occurrences, NodesAttachedBy);
		DifferedPanels.Remove(Index);
		return true;
	}
	return false;
}

bool UNoxelContainer::ConnectNodeDiffered(int32 Index, FNodeID Node)
{
	if (Node.Object)
	{
		UNoxelContainer* AttachedNoxel = Node.Object->GetAttachedNoxel();
		if (AttachedNoxel != this && AttachedNoxel) //If the node is attached to another valid container
		{
			UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::ConnectNodeDiffered] Nodes container can't be attached to this panel"));
			return false;
		}
	}
	else
	{
		UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::ConnectNodeDiffered] Node object is invalid"));
		return false; //Invalid node, ew
	}
	int32 IndexInArray;
	bool found = GetIndexOfPanelByPanelIndex(Index, IndexInArray);
	if (found)
	{
		bool modified = Node.Object->AttachNode(Node, FPanelID(this, Index));
		if (modified)
		{
			FPanelData &data = Panels[IndexInArray];
            data.Nodes.Add(Node);
            DifferedPanels.AddUnique(Index);
			return true;
		}
		return false;
	}
	return false;
}

bool UNoxelContainer::DisconnectNodeDiffered(int32 Index, FNodeID Node)
{
	int32 IndexInArray;
	bool found = GetIndexOfPanelByPanelIndex(Index, IndexInArray);
	if (found && Node.Object)
	{
		bool modified = Node.Object->DetachNode(Node, FPanelID(this, Index)); //Detach nodes
		if (modified)
		{
			FPanelData &data = Panels[IndexInArray];
			data.Nodes.Remove(Node);
			DifferedPanels.AddUnique(Index);
			return true;
		}
		return false;
	}
	return false;
}

bool UNoxelContainer::SetPanelPropertiesDiffered(int32 Index, float ThicknessNormal, float ThicknessAntiNormal,
	bool Virtual)
{
	int32 IndexInArray;
	bool found = GetIndexOfPanelByPanelIndex(Index, IndexInArray);
	if (found)
	{
		FPanelData &data = Panels[IndexInArray];
		data.ThicknessNormal = ThicknessNormal;
		data.ThicknessAntiNormal = ThicknessAntiNormal;
		data.Virtual = Virtual;
	}
	return found;
}

bool UNoxelContainer::RemovePanelDiffered(int32 Index)
{
	int32 IndexInArray;
	bool found = GetIndexOfPanelByPanelIndex(Index, IndexInArray);
	if (found)
	{
		FPanelData &data = Panels[IndexInArray];
		if (data.Nodes.Num() == 0)
		{
			TArray<int32> AdjacentPanels, Occurrences;
         	TArray<TArray<FNodeID>> NodesAttachedBy;
         	TArray<int32> IgnoreFilter;
         	IgnoreFilter.Add(data.PanelIndex);
         	FindPanelsByNodes(data.Nodes, AdjacentPanels, Occurrences, NodesAttachedBy, IgnoreFilter);
         	GetAdjacentPanelsFromNodes(data, AdjacentPanels, Occurrences, NodesAttachedBy); //remove connected
			
			UnusedIndices.Add(Index);
			Panels.RemoveAt(IndexInArray);
			DifferedPanels.Remove(Index);
			
			return true;
		}
	}
	return false;
}

bool UNoxelContainer::AddPanel(FPanelData data)
{
	//UE_LOG(NoxelData, Log, TEXT("[UNoxelContainer::AddPanel] Adding panel"));
	data.PanelIndex = GetNewPanelIndex();
	if(AddPanelDiffered(data.PanelIndex))
	{
		if(SetPanelPropertiesDiffered(data.PanelIndex, data.ThicknessNormal, data.ThicknessAntiNormal, data.Virtual))
		{
			bool valid = true; int i;
			const int32 NumNodes = data.Nodes.Num();
			for (i = 0; i < NumNodes; ++i)
			{
				if(!ConnectNodeDiffered(data.PanelIndex, data.Nodes[i]))
				{
					valid = false;
					break;
				}
			}
			if (valid)
			{
				valid = FinishAddPanel(data.PanelIndex);
			}
			if (!valid)
			{
				for (int j = i - 1; j >= 0; --j)
				{
					DisconnectNodeDiffered(data.PanelIndex, data.Nodes[j]);
				}
				RemovePanelDiffered(data.PanelIndex);
			}
			return valid;
		}
	}
	return false;
}

bool UNoxelContainer::RemovePanel(int32 index)
{
	int32 IndexInArray;
	if(GetIndexOfPanelByPanelIndex(index, IndexInArray))
	{
		//Find the panel
		FPanelData Panel = Panels[IndexInArray];
		for (FNodeID NodeID : Panel.Nodes)
		{
			DisconnectNodeDiffered(index, NodeID);
		}
		Panel = Panels[IndexInArray];
		RemovePanelDiffered(index);
		return true;
		
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

bool UNoxelContainer::GetPanelHit(FHitResult hit, FPanelID& PanelHit)
{
	if (hit.GetComponent()->IsA<UNoxelContainer>())
	{
		UNoxelContainer* HitComp = Cast<UNoxelContainer>(hit.GetComponent());
		PanelHit.Object = HitComp;
		if (HitComp->NoxelProvider->GetPanelIndexHit(hit.FaceIndex, PanelHit.PanelIndex))
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
	}
	NoxelProvider->SetNodes(NodeData);
	NoxelProvider->SetPanels(PanelsData);
}

UBodySetup* UNoxelContainer::GetBodySetup()
{
	if (GetRuntimeMesh())
	{
		UBodySetup* setup = GetRuntimeMesh()->GetBodySetup();
		TArray<FText> validityErrors;
		if (!setup)
		{
			UE_LOG(NoxelData, Warning, TEXT("[%s/%s::GetBodySetup] Body setup is invalid on %s"), *GetOwner()->GetName(), *GetName(), GetWorld()->IsServer() ? TEXT("server") : TEXT("client"))
		}
		else
		{
			if (setup->IsDataValid(validityErrors) != EDataValidationResult::Valid)
			{
				UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::GetBodySetup] Body setup has invalid data"));
			}
			if (setup->AggGeom.GetElementCount() == 0)
			{
				UE_LOG(NoxelData, Warning, TEXT("[UNoxelContainer::GetBodySetup] Body setup has no colliders"))
			}
		}
		return setup;
	}
	return nullptr;
}

bool UNoxelContainer::IsConnected()
{
	return ConnectedNodesContainers.Num() >0;
}

bool UNoxelContainer::CheckDataValidity()
{
	TArray<int32> diffcopy(DifferedPanels);
	for (int32 PanelIdx : diffcopy)
	{
		if (!FinishAddPanel(PanelIdx))
		{
			return false;
		}
	}
	return true;
}

void UNoxelContainer::UpdateMesh()
{
	//UE_LOG(NoxelData, Log, TEXT("[UNodesContainer::UpdateMesh@%s] Called on %s, SpawnContext=%d"), *GetPathName(), GetWorld()->IsServer() ? TEXT("server") : TEXT("client"), SpawnContext);
	UpdateProviderData();
}