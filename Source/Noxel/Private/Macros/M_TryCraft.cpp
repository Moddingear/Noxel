//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_TryCraft.h"
#include "Noxel/NoxelCombatLibrary.h"
#include "Noxel/NoxelNetworkingAgent.h"
#include "EditorCharacter.h"
#include "Components/WidgetInteractionComponent.h"

AM_TryCraft::AM_TryCraft() 
{
	//AlternationMethod = EAlternateType::Hold;
}

// Called when the game starts
void AM_TryCraft::BeginPlay()
{
	Super::BeginPlay();
	//UE_LOG(NoxelMacro, Warning, TEXT("Macro added"));
}

// Called every frame
void AM_TryCraft::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	GetNoxelNetworkingAgent()->SpawnAndPossessCraft();
	Destroy();
	
}