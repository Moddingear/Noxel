// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Tests/NoxelContainerTester.h"

#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelDataStructs.h"

ANoxelContainerTester::ANoxelContainerTester()
{
	TArray<FVector> Nodes = { FVector(0,0,0), FVector(100,0,0), FVector(0,100,0), FVector(100,100,-200), FVector(100,100,-50) };
	nodesContainer->SetNodesDefault(Nodes, true);
}

void ANoxelContainerTester::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);
	noxelContainer->Empty();
	TArray<FNodeID> Nodes;
	TArray<FVector> NodesLoc = { FVector(0,0,0), FVector(100,0,0), FVector(0,100,0), FVector(100,100,-200), FVector(100,100,-50) };
	for (FVector NodeLoc : NodesLoc)
	{
		FNodeID Node;
		nodesContainer->FindNode(NodeLoc, Node);
		Nodes.Add(Node);
	}
	FPanelData panel0({Nodes[0], Nodes[1], Nodes[2]}, 10);
	noxelContainer->AddPanel(panel0);
	FPanelData panel1({ Nodes[3], Nodes[1], Nodes[2] }, 10);
	noxelContainer->AddPanel(panel1);
	FPanelData panel2({ Nodes[4], Nodes[1], Nodes[2] }, 10);
	noxelContainer->AddPanel(panel2);
}
