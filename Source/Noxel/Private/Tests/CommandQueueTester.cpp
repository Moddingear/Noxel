// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Tests/CommandQueueTester.h"
#include "Noxel/NodesContainer.h"
#include "EditorCommandQueue.h"

// Sets default values
ACommandQueueTester::ACommandQueueTester()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACommandQueueTester::BeginPlay()
{
	Super::BeginPlay();
	FEditorQueueNetworkable queue;
	/*queue.AddNodeAddOrder(FNodeID(nodesContainer, FVector(10,20,30)));
	queue.AddNodeRemoveOrder(FNodeID(nodesContainer,FVector(10,20,30)));
	queue.PrintQueueDecoded();*/
}

// Called every frame
void ACommandQueueTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

