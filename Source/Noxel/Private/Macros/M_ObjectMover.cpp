//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_ObjectMover.h"
#include "Noxel/NoxelCombatLibrary.h"
#include "EditorCharacter.h"
#include "Components/WidgetInteractionComponent.h"

AM_ObjectMover::AM_ObjectMover() 
{
	//AlternationMethod = EAlternateType::Hold;
}

// Called when the game starts
void AM_ObjectMover::BeginPlay()
{
	Super::BeginPlay();
	//UE_LOG(NoxelMacro, Warning, TEXT("Macro added"));
}

// Called every frame
void AM_ObjectMover::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (Alternate)
	{

	}
	else 
	{

	}
	
}

void AM_ObjectMover::leftClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Left click"));
	
}

void AM_ObjectMover::leftClickReleased_Implementation()
{
}

void AM_ObjectMover::middleClickPressed_Implementation()
{
}

void AM_ObjectMover::middleClickReleased_Implementation()
{
}

void AM_ObjectMover::rightClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Right click"));
}

void AM_ObjectMover::rightClickReleased_Implementation()
{
}
