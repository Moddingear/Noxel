//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_ObjectMover.h"
#include "Noxel/NoxelCombatLibrary.h"
#include "EditorCharacter.h"
#include "Components/WidgetInteractionComponent.h"
#include "NObjects/NoxelPart.h"
#include "Noxel/NoxelNetworkingAgent.h"

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
	FVector location, direction, end;
	GetRayFromFollow(location, direction);
	end = location + direction * ray_length;
	AActor* FoundComponent;
	if (TraceNObjects(location, end, FoundComponent))
	{
		if (FoundComponent->IsA<ANoxelPart>())
		{
			GetOwningActor()->SetCurrentPart(Cast<ANoxelPart>(FoundComponent));
		}
	}
}

void AM_ObjectMover::middleClickReleased_Implementation()
{
}

void AM_ObjectMover::rightClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Right click"));
	FVector location, direction, end;
	GetRayFromFollow(location, direction);
	end = location + direction * ray_length;
	AActor* FoundComponent;
	if (TraceNObjects(location, end, FoundComponent))
	{
		FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
		queue->AddObjectRemoveOrder(GetCraft(), FoundComponent);
		GetNoxelNetworkingAgent()->SendCommandQueue(queue);
	}
}

void AM_ObjectMover::rightClickReleased_Implementation()
{
}
