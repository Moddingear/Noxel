//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/NoxelCombatLibrary.h"
#include "Noxel.h"

#include "Noxel/AStarNoxel.h"
#include "EditorGameState.h"
#include "Noxel/CraftDataHandler.h"
#include "NoxelHangarBase.h"
#include "Connectors/ConnectorBase.h"

bool UNoxelCombatLibrary::AreNodeConnected(FNodeID start, FNodeID end)
{
	AStarNoxel instance;
	return instance.runAStar(start, end);
}

UCraftDataHandler * UNoxelCombatLibrary::GetCraftDataHandler(AActor * CraftComponent)
{
	AGameStateBase* gs = CraftComponent->GetWorld()->GetGameState();
	if ((AEditorGameState*) gs) 
	{
		AEditorGameState* egs = (AEditorGameState*) gs;
		return egs->GetCraft();
	}
	return nullptr;
}

TArray<UConnectorBase*> UNoxelCombatLibrary::GetConnectorsFromActor(AActor * Actor)
{
	TArray<UConnectorBase*> ConnectorsActor;
	Actor->GetComponents<UConnectorBase>(ConnectorsActor);
	return ConnectorsActor;
}

/*bool UNoxelCombatLibrary::DiagnoseCraft(UCraftDataHandler * Craft, FText & OutCritical, FText & OutWarning, FText & OutLog)
{
	bool HasCritical = false;
	UE_LOG(Noxel, Warning, TEXT("[UNoxelCombatLibrary::DiagnoseCraft] TODO"));
	//Check if it has a possessable component
	APawn* Seat = nullptr;
	for (AActor* Component : Craft->Components)
	{
		if (Component->IsA(APawn::StaticClass()))
		{
			Seat = (APawn*) Component;
			break;
		}
	}


	if (Seat)
	{

	}
	//Check if all components are non-floating
	
	bool HasWarning = false;
	bool HasLog = false;
	return false;
}*/