//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_Voxel.h"
#include "Noxel/NoxelNetworkingAgent.h"
#include "Voxel/VoxelComponent.h"

AM_Voxel::AM_Voxel()
{
	AlternationMethod = EAlternateType::None;
}

void AM_Voxel::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AM_Voxel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "AddBlock", "Add block");
	RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "RemoveBlock", "Remove block");
}

void AM_Voxel::leftClickPressed_Implementation()
{
	FVector location, direction, end;
	getRay(location, direction);
	end = location + direction * ray_length;
	UVoxelComponent* voxel = GetVoxel();
	if (!voxel) {
		UE_LOG(NoxelMacro, Warning, TEXT("[MACRO] Voxel not found, aborting..."));
		return;
	}
	FIntVector cube_hit, direction_hit, cube_place;
	if (GetVoxel()->trace(location, end, cube_hit, direction_hit)) { //If hit in the voxel
		cube_place = cube_hit + direction_hit;
		UE_LOG(NoxelMacro, Log, TEXT("Cube hit : %s | Direction : %s"), *cube_hit.ToString(), *direction_hit.ToString());
	}
	else {
		FHitResult hit;
		FCollisionQueryParams qp = FCollisionQueryParams();
		qp.bTraceComplex = true;
		qp.AddIgnoredActor(GetOwner());
		if (GetWorld()->LineTraceSingleByChannel(hit, location, end, ECollisionChannel::ECC_WorldStatic, qp))
		{
			cube_place = GetVoxel()->worldToVoxel(hit.Location);
		}
	}
	//GetNoxelNetworkingAgent()->AddBlock(GetVoxel(), cube_place);
}

void AM_Voxel::middleClickPressed_Implementation()
{
}

void AM_Voxel::rightClickPressed_Implementation()
{
	FVector location, direction;
	getRay(location, direction);
	const FVector end = location + direction * ray_length;
	UVoxelComponent* voxel = GetVoxel();
	if (!voxel) {
		UE_LOG(NoxelMacro, Warning, TEXT("[MACRO] Voxel not found, aborting..."));
		return;
	}
	FIntVector cube_hit, direction_hit;
	if (voxel->trace(location, end, cube_hit, direction_hit)) { //If hit in the voxel
		UE_LOG(NoxelMacro, Log, TEXT("Cube hit : %s | Direction : %s"), *cube_hit.ToString(), *direction_hit.ToString());
		//GetNoxelNetworkingAgent()->RemoveBlock(voxel, cube_hit);
	}
	//GetNoxelNetworkingAgent()->RemoveBlock(voxel, voxel->worldToVoxel(GetOwner()->GetTransform().GetLocation())); //Remove the cube where the player is to avoid getting stuck
}
