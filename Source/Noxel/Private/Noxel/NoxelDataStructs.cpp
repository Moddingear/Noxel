//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/NoxelDataStructs.h"
#include "Noxel/NodesContainer.h"
#include "Kismet/KismetSystemLibrary.h"

FNodeID FNodeID::FromWorld(UNodesContainer* InObject, FVector WorldLocation)
{
	if (!InObject)
	{
		return FNodeID();
	}
	return FNodeID(InObject, InObject->GetComponentTransform().InverseTransformPosition(WorldLocation));
}

FVector FNodeID::ToWorld() const
{
	return Object->GetComponentTransform().TransformPosition(Location);
}

bool FNodeID::IsValid() const
{
	return UKismetSystemLibrary::IsValid(Object);
}

FString FNodeID::ToString() const
{
	return FString::Printf(TEXT("Object Name = %s; Location = %s"), *(Object->GetName()), *Location.ToString());
}
