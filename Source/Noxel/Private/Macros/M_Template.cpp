//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#if false
#include "Macros/M_Template.h"
#include "Noxel/NoxelCombatLibrary.h"
#include "EditorCharacter.h"
#include "Components/WidgetInteractionComponent.h"

AM_Template::AM_Template() 
{
	//AlternationMethod = EAlternateType::Hold;
}

// Called when the game starts
void AM_Template::BeginPlay()
{
	Super::BeginPlay();
	//UE_LOG(NoxelMacro, Warning, TEXT("Macro added"));
}

// Called every frame
void AM_Template::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (Alternate)
	{

	}
	else 
	{

	}
	
}

void AM_Template::leftClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Left click"));
	
}

void AM_Template::leftClickReleased_Implementation()
{
}

void AM_Template::middleClickPressed_Implementation()
{
}

void AM_Template::middleClickReleased_Implementation()
{
}

void AM_Template::rightClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Right click"));
}

void AM_Template::rightClickReleased_Implementation()
{
}
#endif