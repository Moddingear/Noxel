//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "NoxelPlayerController.h"

#include "Noxel.h"

#include "Noxel/CraftDataHandler.h"

ANoxelPlayerController::ANoxelPlayerController()
{
	
}

void ANoxelPlayerController::SynchroniseNoxel(UNoxelContainer * NoxelContainer)
{
	if (!NoxelContainer) {
		UE_LOG(NoxelDataNetwork, Warning, TEXT("{SynchroniseNoxel} UNoxelContainer reference is null"));
		return;
	}
	Server_NoxelSync(NoxelContainer);
}

void ANoxelPlayerController::SynchroniseUnconnectedNodes(UNodesContainer* NodesContainer)
{
	if (!NodesContainer) {
		UE_LOG(NoxelDataNetwork, Warning, TEXT("{SynchroniseUnconnectedNodes} UNodesContainer reference is null"));
		return;
	}
	Server_NodesSync(NodesContainer);
}

void ANoxelPlayerController::Server_NoxelSync_Implementation(UNoxelContainer * NoxelContainer)
{
	if (!NoxelContainer) {
		UE_LOG(NoxelDataNetwork, Warning, TEXT("{Server_NoxelSync_Implementation} UNoxelContainerReference is null"));
		return;
	}
	Client_NoxelSync(UCraftDataHandler::saveNoxelNetwork(NoxelContainer));
}

bool ANoxelPlayerController::Server_NoxelSync_Validate(UNoxelContainer * NoxelContainer)
{
	return true; //kick the player if return false
}

void ANoxelPlayerController::Client_NoxelSync_Implementation(FNoxelNetwork Save)
{
	if (UCraftDataHandler::loadNoxelNetwork(Save))
	{
		//Synchronisation success
	}
	else
	{
		UE_LOG(NoxelDataNetwork, Warning, TEXT("[ANoxelPlayerController::Client_NoxelSync_Implementation] Failed to load panels from network!"));
	}
}

void ANoxelPlayerController::Server_NodesSync_Implementation(UNodesContainer * NodesContainer)
{
	if (!NodesContainer) {
		UE_LOG(NoxelDataNetwork, Warning, TEXT("{Server_NodesSync_Implementation} UNoxelContainerReference is null"));
		return;
	}
	Client_NodesSync(UCraftDataHandler::saveNodesNetwork(NodesContainer));
}

bool ANoxelPlayerController::Server_NodesSync_Validate(UNodesContainer * NodesContainer)
{
	return true; //kick the player if return false
}

void ANoxelPlayerController::Client_NodesSync_Implementation(FNodesNetwork Save)
{
	UCraftDataHandler::loadNodesNetwork(Save);
}