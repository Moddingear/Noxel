//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/NodesContainer.h"

#include "Noxel.h"

#include "NodesRMCProvider.h"

#include "Net/UnrealNetwork.h"
#include "FunctionLibrary.h"
#include "Noxel/NoxelNetworkingAgent.h"

#include "NoxelPlayerController.h"

UNodesContainer::UNodesContainer()
	: Super()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	NodeSize = 10.0f;
	bPlayerEditable = false;
	bIsMeshDirty = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh>MeshConstructor(TEXT("StaticMesh'/Game/Meshes/Nodes/Octahedron.Octahedron'"));
	BaseNodeMesh = MeshConstructor.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>MaterialConstructor(TEXT("Material'/Game/Materials/Noxel/Plastic.Plastic'"));
	NodeMaterial = MaterialConstructor.Object;

	SetCollisionProfileName(TEXT("Node"));

	NodesProvider = CreateDefaultSubobject<UNodesRMCProvider>("NodesProvider");
}

void UNodesContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Here we list the variables we want to replicate + a condition if wanted
	DOREPLIFETIME(UNodesContainer, AttachedNoxel);
}

void UNodesContainer::OnRegister()
{
	Super::OnRegister();
	DefaultNodeColor = UFunctionLibrary::getColorFromJson(ENoxelColor::NodeInactive);
    for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
    {
        Nodes[NodeIdx].Color = DefaultNodeColor;
    }
    NodesProvider->SetWantedMeshBounds(FBoxSphereBounds(FVector::ZeroVector, FVector::OneVector * NodeSize, NodeSize));
    NodesProvider->SetStaticMesh(BaseNodeMesh);
    NodesProvider->SetNodesMaterial(NodeMaterial);
    UpdateMesh();
    Initialize(NodesProvider);
}

void UNodesContainer::BeginPlay()
{
	Super::BeginPlay();

	if (SpawnContext != ECraftSpawnContext::Battle)
	{
		if (!GetWorld()->IsServer() && !AttachedNoxel) { //If it's a client and it's not conected to any noxel
			SetSpawnContext(SpawnContext);
			if ((ANoxelPlayerController*)GetWorld()->GetFirstPlayerController()) {
				((ANoxelPlayerController*)GetWorld()->GetFirstPlayerController())->SynchroniseUnconnectedNodes(this);
			}
		}
	}
	else
	{
		UpdateMesh();
	}
}

void UNodesContainer::PostInitProperties()
{
	Super::PostInitProperties();
}

void UNodesContainer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bIsMeshDirty)
	{
		//UE_LOG(NoxelData, Log, TEXT("[UNodesContainer::TickComponent] Updating mesh from tick because it's dirty"));
		UpdateMesh();
	}
}

void UNodesContainer::SetNodeSize(float NewNodeSize)
{
	NodeSize = NewNodeSize;
	NodesProvider->SetWantedMeshBounds(FBoxSphereBounds(FVector::ZeroVector, FVector::OneVector * NodeSize, NodeSize));
}

UNoxelContainer * UNodesContainer::GetAttachedNoxel()
{
	return AttachedNoxel;
}

bool UNodesContainer::AddNode(FVector Location)
{
	//TODO : Handle overlap
	if (!Nodes.Contains(Location))
	{
		Nodes.Emplace(Location, DefaultNodeColor);
		MarkMeshDirty();
		return true;
	}
	return false;
}

bool UNodesContainer::AddNode(FNodeID Node)
{
	if (Node.IsValid())
	{
		return Node.Object->AddNode(Node.Location);
	}
	return false;
}

bool UNodesContainer::SetNodesDefault(TArray<FVector> InNodes, bool bInPlayerEditable)
{
	Nodes.Empty(InNodes.Num());
	for (FVector location : InNodes)
	{
		Nodes.Emplace(location);
	}
	bPlayerEditable = bInPlayerEditable;
	return true;
}

bool UNodesContainer::AttachNode(const FVector Location, const FPanelID Panel)
{
	if (AttachedNoxel && Panel.Object != AttachedNoxel)
	{
		return false;
	}
	int32 NodeIdx = Nodes.Find(Location);
	if (NodeIdx != INDEX_NONE)
	{
		if (Nodes[NodeIdx].ConnectedPanels.Num() == 0)
		{
			NumNodesAttached++;
		}
		Nodes[NodeIdx].ConnectedPanels.AddUnique(Panel);
		AttachToNoxelContainer(Panel.Object);
		return true;
	}
	return false;
}

bool UNodesContainer::DetachNode(const FVector Location, const FPanelID Panel)
{
	int32 NodeIdx = Nodes.Find(Location);
	if (NodeIdx != INDEX_NONE)
	{
		int32 numRemoved = Nodes[NodeIdx].ConnectedPanels.Remove(Panel);
		if (Nodes[NodeIdx].ConnectedPanels.Num() == 0)
		{
			NumNodesAttached--;
			if (NumNodesAttached <= 0)
			{
				AttachToNoxelContainer(nullptr);
			}
		}
		return numRemoved != 0;
	}
	return false;
}

bool UNodesContainer::RemoveNode(FVector Location)
{
	int32 NodeIdx = Nodes.Find(Location);
	if (NodeIdx != INDEX_NONE)
	{
		if (Nodes[NodeIdx].ConnectedPanels.Num() != 0)
		{
			return false;
		}
		Nodes.RemoveAt(NodeIdx);
		MarkMeshDirty();
		return true;
	}
	return false;
}

bool UNodesContainer::RemoveNode(FNodeID Node)
{
	if (Node.IsValid())
	{
		return Node.Object->RemoveNode(Node.Location);
	}
	return false;
}

bool UNodesContainer::SetNodeColor(FVector Location, FColor color)
{
	int32 NodeIdx = Nodes.Find(Location);
	if (NodeIdx != INDEX_NONE)
	{
		Nodes[NodeIdx].Color = color;
		MarkMeshDirty();
		return true;
	}
	return false;
}

bool UNodesContainer::FindNode(FVector Location, FNodeID& FoundNode)
{
	int32 NodeIdx = Nodes.Find(Location);
	if (NodeIdx != INDEX_NONE)
	{
		FoundNode = Nodes[NodeIdx].ToNodeID(this);
		return true;
	}
	return false;
}

TArray<int32> UNodesContainer::GetAttachedPanels(FVector Location)
{
	int32 NodeIdx = Nodes.Find(Location);
	if (NodeIdx != INDEX_NONE)
	{
		return Nodes[NodeIdx].ConnectedPanels;
	}
	return TArray<int32>();
}

TArray<FNodeID> UNodesContainer::GenerateNodesKeyArray()
{
	TArray<FNodeID> Keys;
	for (FNodeData Node : Nodes)
	{
		Keys.Add(Node.ToNodeID(this));
	}
	return Keys;
}

bool UNodesContainer::GetNodeHit(FHitResult Hit, FNodeID& HitNode)
{
	if (Hit.bBlockingHit)
	{
		if (Hit.GetComponent()->IsA<UNodesContainer>())
		{
			UNodesContainer* HitComp =Cast<UNodesContainer>(Hit.GetComponent());
			if (Hit.FaceIndex != INDEX_NONE)
			{
				int32 HitNodeIdx;
				if(HitComp->NodesProvider->GetHitNodeIndex(Hit.FaceIndex, HitNodeIdx))
				{
					HitNode = FNodeID(HitComp, HitComp->Nodes[HitNodeIdx].Location);
					return true;
				}
				else
				{
					UE_LOG(NoxelData, Warning, TEXT("[UNodesContainer::GetNodeHit] Failed because it couldn't find a hit node"));
				}
			}
			else
			{
				UE_LOG(NoxelData, Warning, TEXT("[UNodesContainer::GetNodeHit] Failed because face index is INDEX_NONE"));
			}
		}
		else
		{
			UE_LOG(NoxelData, Warning, TEXT("[UNodesContainer::GetNodeHit] Failed because hit component isn't a NodesContainer"));
		}
	}
	else
	{
		UE_LOG(NoxelData, Warning, TEXT("[UNodesContainer::GetNodeHit] Failed because hit result was non-blocking"));
	}
	return false;
	
}

bool UNodesContainer::IsPlayerEditable() const
{
	return bPlayerEditable;
}

void UNodesContainer::MarkMeshDirty()
{
	bIsMeshDirty = true;
}

void UNodesContainer::UpdateMesh()
{
	Super::UpdateMesh();
	//UE_LOG(NoxelData, Log, TEXT("[UNodesContainer::UpdateMesh@%p] Called"), this);
	bIsMeshDirty = false;
	if (SpawnContext != ECraftSpawnContext::Battle)
	{
		TArray<FNoxelRendererNodeData> RendererNodes;
        RendererNodes.Reserve(Nodes.Num());
        for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
        {
        	RendererNodes.EmplaceAt(NodeIdx, Nodes[NodeIdx].Location, Nodes[NodeIdx].Color);
        }
        NodesProvider->SetNodes(RendererNodes);
	}
	else
	{
		NodesProvider->SetNodes({});
	}
}

void UNodesContainer::AttachToNoxelContainer(UNoxelContainer * NoxelContainer)
{
	if (NoxelContainer) {
		if (!AttachedNoxel) {
			AttachedNoxel = NoxelContainer; //Set the noxel
			NoxelContainer->ConnectedNodesContainers.AddUnique(this);
			if (GetWorld()->IsServer() && NoxelContainer->GetOwner() != GetOwner()) {
				GetOwner()->SetOwner(NoxelContainer->GetOwner());
				GetOwner()->bNetUseOwnerRelevancy = true;
			}
		}
	}
	else {
		if (AttachedNoxel) {
			AttachedNoxel->ConnectedNodesContainers.Remove(this);
			AttachedNoxel = nullptr;
			if (GetWorld()->IsServer()) {
				GetOwner()->bNetUseOwnerRelevancy = false;
			}
		}
	}
}

void UNodesContainer::SetSpawnContext(ECraftSpawnContext Context)
{
	Super::SetSpawnContext(Context);
	//UE_LOG(Noxel, Log, TEXT("[UNodesContainer::SetSpawnContext] Spawn context set"));
}

bool UNodesContainer::IsConnected()
{
	return IsValid(AttachedNoxel);
}

bool UNodesContainer::CheckDataValidity()
{
	return true;
}

