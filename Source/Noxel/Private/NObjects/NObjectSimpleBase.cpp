//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "NObjects/NObjectSimpleBase.h"


// Sets default values
ANObjectSimpleBase::ANObjectSimpleBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicatingMovement(true);
	staticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	RootComponent = (USceneComponent*)staticMesh;
	staticMesh->bEditableWhenInherited = true;

	nodesContainer = CreateDefaultSubobject<UNodesContainer>(TEXT("Nodes Container"));
	nodesContainer->SetupAttachment(RootComponent);
	nodesContainer->bEditableWhenInherited = true;
}

// Called when the game starts or when spawned
void ANObjectSimpleBase::BeginPlay()
{
	Super::BeginPlay();
	
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

FString ANObjectSimpleBase::OnReadMetadata_Implementation()
{
	return FString();
}

bool ANObjectSimpleBase::OnWriteMetadata_Implementation(const FString & Metadata)
{
	return false;
}

// Called every frame
void ANObjectSimpleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

