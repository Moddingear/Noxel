//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/NoxelDataComponent.h"
#include "Net/UnrealNetwork.h"
#include "Noxel/CraftDataHandler.h"

UNoxelDataComponent::UNoxelDataComponent()
	:Super()
{
	SpawnContext = ECraftSpawnContext::None;
	SetIsReplicatedByDefault(true);
}

void UNoxelDataComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNoxelDataComponent, SpawnContext);
}

void UNoxelDataComponent::SetSpawnContext(ECraftSpawnContext Context)
{
	if (GetWorld()->IsServer()) {
		if (SpawnContext == ECraftSpawnContext::None) {
			SpawnContext = Context;
		}
	}
}

bool UNoxelDataComponent::IsConnected()
{
	return false;
}

bool UNoxelDataComponent::CheckDataValidity()
{
	return true;
}

void UNoxelDataComponent::UpdateMesh()
{
}
