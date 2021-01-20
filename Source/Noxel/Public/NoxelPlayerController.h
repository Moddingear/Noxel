//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Noxel/NoxelDataStructs.h"

#include "NoxelPlayerController.generated.h"

/**
 * This class cannot use Multicast since it only exists on the client and the server
 */
UCLASS()
class NOXEL_API ANoxelPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANoxelPlayerController();

public:

	UFUNCTION(BlueprintCallable)
		void SynchroniseNoxel(UNoxelContainer* NoxelContainer);

	UFUNCTION(BlueprintCallable)
		void SynchroniseUnconnectedNodes(UNodesContainer* NodesContainer);

private:

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_NoxelSync(UNoxelContainer* NoxelContainer);

	UFUNCTION(Client, Reliable)
		void Client_NoxelSync(FNoxelNetwork Save);

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_NodesSync(UNodesContainer* NodesContainer);

	UFUNCTION(Client, Reliable)
		void Client_NodesSync(FNodesNetwork Save);
};
