//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "NObjects/NoxelPart.h"
#include "Noxel.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"
#include "..\..\Public\NObjects\NoxelPart.h"


// Sets default values
ANoxelPart::ANoxelPart()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	bReplicates = true;
	SetReplicatingMovement(true);
	PrimaryActorTick.bCanEverTick = true;
	noxelContainer = CreateDefaultSubobject<UNoxelContainer>(TEXT("Noxel Container"));
	RootComponent = noxelContainer;

	nodesContainer = CreateDefaultSubobject<UNodesContainer>(TEXT("Nodes Container"));
	nodesContainer->SetupAttachment(RootComponent);
	nodesContainer->SetNodesDefault(TArray<FVector>(), true);

}

// Called when the game starts or when spawned
void ANoxelPart::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(Noxel, Log, TEXT("[(%s)ANoxelPart::BeginPlay] Server : %s; Part location : %s"), *GetName(), GetWorld()->IsServer() ? TEXT("True") : TEXT("False"), *GetActorLocation().ToString());
}

UNoxelContainer * ANoxelPart::GetNoxelContainer()
{
	return noxelContainer;
}

UNodesContainer * ANoxelPart::GetNodesContainer()
{
	return nodesContainer;
}

// Called every frame
void ANoxelPart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

