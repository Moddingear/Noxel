//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "NObjects/NObjectSimpleBase.h"

#include "Noxel.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ANObjectSimpleBase::ANObjectSimpleBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	

	bReplicates = true;
	SetReplicatingMovement(false);
	staticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	staticMesh->SetCollisionProfileName(TEXT("NObject"));
	RootComponent = Cast<USceneComponent>(staticMesh);
	staticMesh->bEditableWhenInherited = true;
	staticMesh->SetIsReplicated(false); //need for attachments to work over the network

	nodesContainer = CreateDefaultSubobject<UNodesContainer>(TEXT("Nodes Container"));
	nodesContainer->SetupAttachment(RootComponent);
	nodesContainer->bEditableWhenInherited = true;
}

void ANObjectSimpleBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANObjectSimpleBase, Enabled);
	DOREPLIFETIME(ANObjectSimpleBase, ParentCraft);
	DOREPLIFETIME(ANObjectSimpleBase, AttachmentData);
}

// Called when the game starts or when spawned
void ANObjectSimpleBase::BeginPlay()
{
	Super::BeginPlay();
	CheckNetworkAttachment("ANObjectSimpleBase::BeginPlay");
}

void ANObjectSimpleBase::OnNObjectEnable_Implementation(UCraftDataHandler* Craft)
{
	Enabled = true;
	ParentCraft = Craft;
}

void ANObjectSimpleBase::OnNObjectDisable_Implementation()
{
	Enabled = false;
}

bool ANObjectSimpleBase::OnNObjectAttach_Implementation(ANoxelPart * Part)
{
	return false;
}

FJsonObjectWrapper ANObjectSimpleBase::OnReadMetadata_Implementation(const TArray<AActor*>& Components)
{
	return FJsonObjectWrapper();
}

bool ANObjectSimpleBase::OnWriteMetadata_Implementation(const FJsonObjectWrapper & Metadata, const TArray<AActor*>& Components)
{
	return false;
}

void ANObjectSimpleBase::SetReplicatedAttachmentData_Implementation(FNoxelReplicatedAttachmentData data)
{
	check(GetWorld()->IsServer());
	AttachmentData = data;
	OnRep_NoxelAttachment();
}

bool ANObjectSimpleBase::IsAttachedAtFinalLocation_Implementation()
{
	return AttachmentData.valid && IsValid(AttachmentData.ParentComponent);
}

void ANObjectSimpleBase::OnRep_NoxelAttachment()
{
	UE_LOG(NoxelDataNetwork, Log, TEXT("[ANObjectSimpleBase::OnRep_NoxelAttachment] Called on %s"), GetWorld()->IsServer() ? TEXT("Server") : TEXT("Client"));
	if (AttachmentData.valid && IsValid(AttachmentData.ParentComponent))
	{
		FAttachmentTransformRules rules(EAttachmentRule::SnapToTarget, false);
		AttachToComponent(AttachmentData.ParentComponent, rules);
		SetActorRelativeTransform(AttachmentData.AttachOffset);
	}
}

// Called every frame
void ANObjectSimpleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Enabled != EnabledPrev)
	{
		EnabledPrev = Enabled;
		//CheckNetworkAttachment("ANObjectSimpleBase::Tick/enable");
	}
	if (AttachedPrev != IsAttachmentValid())
	{
		AttachedPrev = !AttachedPrev;
		CheckNetworkAttachment("ANObjectSimpleBase::Tick/attachment");
	}
	if ((GFrameCounter%60) ==0 && Enabled)
	{
		//CheckNetworkAttachment("ANObjectSimpleBase::Tick/frame");
	}
}

