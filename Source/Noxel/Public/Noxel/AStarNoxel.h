//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NoxelLibrary.h"

struct FActorCache
{
	AActor* Actor;
	TArray<FNodeID> Nodes;

	FActorCache()
	{}

	FActorCache(AActor* InActor)
	{
		Actor = InActor;
	}

	FActorCache(AActor* InActor, TArray<FNodeID> InNodes)
	{
		Actor = InActor;
		Nodes = InNodes;
	}

	FORCEINLINE bool operator== (const FActorCache& Other) const
	{
		return Actor == Other.Actor;
	}

	friend uint32 GetTypeHash(const FActorCache& Other)
	{
		return GetTypeHash(Other.Actor);
	}
};

/**
 * 
 */
class NOXEL_API AStarNoxel
{
public:
	AStarNoxel();
	~AStarNoxel();

	TArray<FNodeID> VisitedNodes;
	TArray<float> NodeDistances;
	TArray<FNodeID> UnvisitedNodes;
	TArray<FActorCache> Cache;
	FNodeID target;


	bool runAStar(FNodeID start, FNodeID end);

private:
	FNodeID discoverClosestNode(FNodeID node);

	float getDistanceStored(FNodeID node);

	float getDistance(FNodeID node);

	TArray<FNodeID> getConnectedNodes(FNodeID node);
};
