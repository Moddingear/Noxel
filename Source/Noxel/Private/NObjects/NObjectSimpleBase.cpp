//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "NObjects/NObjectSimpleBase.h"

#include "Noxel.h"
#include "Net/UnrealNetwork.h"
#include "Noxel/CraftDataHandler.h"


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
}

// Called when the game starts or when spawned
void ANObjectSimpleBase::BeginPlay()
{
	Super::BeginPlay();
	//CheckNetworkAttachment("ANObjectSimpleBase::BeginPlay");
}

// Called every frame
void ANObjectSimpleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Enabled != EnabledPrev)
	{
		if (Enabled)
		{
			INObjectInterface::Execute_OnNObjectEnable(this, ParentCraft);
		}
		else
		{
			INObjectInterface::Execute_OnNObjectDisable(this);
		}
	}
}

void ANObjectSimpleBase::OnNObjectEnable_Implementation(UCraftDataHandler* Craft)
{
	Enabled = true;
	EnabledPrev = Enabled;
	ParentCraft = Craft;
}

void ANObjectSimpleBase::OnNObjectDisable_Implementation()
{
	Enabled = false;
	EnabledPrev = Enabled;
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

