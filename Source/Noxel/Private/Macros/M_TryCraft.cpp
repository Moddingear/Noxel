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

void AM_TryCraft::leftClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Left click"));
	
}

void AM_TryCraft::leftClickReleased_Implementation()
{
}

void AM_TryCraft::middleClickPressed_Implementation()
{
}

void AM_TryCraft::middleClickReleased_Implementation()
{
}

void AM_TryCraft::rightClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Right click"));
}

void AM_TryCraft::rightClickReleased_Implementation()
{
}