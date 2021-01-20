//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/AStarNoxel.h"
#include "Noxel.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"
#include "Noxel/NoxelCombatLibrary.h"

AStarNoxel::AStarNoxel()
{
}

AStarNoxel::~AStarNoxel()
{
}

bool AStarNoxel::runAStar(FNodeID start, FNodeID end)
{
	//In case we use multiple times the same instance
	if ((VisitedNodes.Contains(start) || UnvisitedNodes.Contains(start)) && (VisitedNodes.Contains(end) || UnvisitedNodes.Contains(end)))
	{
		return true;
	}
	else
	{
		VisitedNodes.Empty(20);
		NodeDistances.Empty(20);
		UnvisitedNodes.Empty(20);
	}

	target = end;
	getDistanceStored(start);
	FNodeID last = start;
	while (NodeDistances.Num() > 0) { //End if there no panels to discover

		if (last == target) { //If the last closest found is the target
			return true;
		}

		if (!(last == FNodeID())) { //if the last find is valid, keep the same track
			last = discoverClosestNode(last);
		}
		else { //Find a new index

			int32 minindex;
			float min;
			UKismetMathLibrary::MinOfFloatArray(NodeDistances, minindex, min); //Start from the lowest distance
			last = discoverClosestNode(UnvisitedNodes[minindex]);
		}
	}
	return false;
}

FNodeID AStarNoxel::discoverClosestNode(FNodeID node)
{
	TArray<FNodeID> Connected = getConnectedNodes(node);
	for (FNodeID nodev : VisitedNodes)
	{
		Connected.Remove(nodev);
	}
	float mindist = -1.0f;
	FNodeID closest;
	for (FNodeID conn : Connected)
	{
		float dist = getDistanceStored(conn);
		if (mindist == -1.0f || dist < mindist)
		{
			mindist = dist;
			closest = conn;
		}
	}
	int index = UnvisitedNodes.Find(node);
	UnvisitedNodes.RemoveAt(index);
	NodeDistances.RemoveAt(index);
	VisitedNodes.Add(node);
	return closest;
}

float AStarNoxel::getDistanceStored(FNodeID node)
{
	if (VisitedNodes.Contains(node)) {
		return -1.0f;
	}
	if (UnvisitedNodes.Contains(node)) {
		return NodeDistances[UnvisitedNodes.Find(node)];
	}
	else {
		float dist = getDistance(node);
		NodeDistances.Add(dist);
		UnvisitedNodes.Add(node);
		return dist;
	}
}

float AStarNoxel::getDistance(FNodeID node)
{
	if (node.Object == target.Object) {
		return FVector::Distance(node.Location,target.Location);
	}
	else {
		
		return FVector::Distance(
			node.Location, 
			node.Object->GetComponentToWorld().InverseTransformPosition(
				target.Object->GetComponentToWorld().TransformPosition(
					target.Location
				)
			)
		);
	}
}

TArray<FNodeID> AStarNoxel::getConnectedNodes(FNodeID node)
{
	TArray<FNodeID> ConnectedRaw;
	if (!node.Object->IsPlayerEditable()) //If object doesn't allow write, all nodes are considered connected
	{
		if (Cache.Contains(FActorCache(node.Object->GetOwner())))
		{
			int32 cacheIdx;
			Cache.Find(node.Object->GetOwner(), cacheIdx);
			ConnectedRaw = Cache[cacheIdx].Nodes;
		}
		else
		{
			TArray<UNodesContainer*> nc;
			AActor* owner = node.Object->GetOwner();
			owner->GetComponents<UNodesContainer>(nc);
			for (UNodesContainer* container : nc)
			{
				TArray<FNodeID> nodes = container->GenerateNodesKeyArray();
				ConnectedRaw.Append(nodes);
			}
			Cache.Emplace(owner, ConnectedRaw);
		}
	}
	TArray<int32> ConnectedPanels = node.Object->GetAttachedPanels(node);
	for (int32 connectedpanel : ConnectedPanels)
	{
		FPanelData ConnectedPanelData;
		if (node.Object->GetAttachedNoxel()->GetPanelByPanelIndex(connectedpanel, ConnectedPanelData))
		{
			ConnectedRaw.Append(ConnectedPanelData.Nodes);
		}
	}
	TArray<FNodeID> Connected;
	for (FNodeID connected : ConnectedRaw)
	{
		Connected.AddUnique(connected);
	}
	return Connected;
}
