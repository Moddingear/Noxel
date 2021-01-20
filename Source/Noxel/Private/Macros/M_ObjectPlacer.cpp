//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_ObjectPlacer.h"
#include "Runtime/UMG/Public/UMG.h"
#include "Macros/M_Nodes.h"
#include "EditorCharacter.h"

AM_ObjectPlacer::AM_ObjectPlacer()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> hudWidgetObj(TEXT("/Game/NoxelEditor/Macros/Inventory/M_UMGObjectInventory"));
	if (hudWidgetObj.Succeeded()) 
	{
		wInventory = hudWidgetObj.Class;
	}
	
	//AlternationMethod = EAlternateType::Hold;
}

void AM_ObjectPlacer::BeginPlay()
{
	Super::BeginPlay();
	Inventory = CreateWidget<UUserWidget>(GetWorld()->GetFirstPlayerController(), wInventory);
	Inventory->AddToViewport();
	onObjectDelegate.AddDynamic(this, &AM_ObjectPlacer::onObjectCall);
	//UE_LOG(NoxelMacro, Warning, TEXT("Macro added"));
}

void AM_ObjectPlacer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (Inventory)
	{
		Inventory->RemoveFromParent();
	}
	if (IsValid(ObjectSpawned))
	{
		GetNoxelNetworkingAgent()->RemoveObject(ObjectSpawned);
	}
}

void AM_ObjectPlacer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ObjectSpawned)
	{
		ObjectSpawned->SetActorLocation(getObjectLocation());
		if (!GetWorld()->IsServer())
		{
			GetNoxelNetworkingAgent()->MoveObjectUnreliable(ObjectSpawned, FTransform(getObjectLocation()));
		}

		LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "ObjectPositionConfirm", "Confirm object placement");
		RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "ObjectCancel", "Cancel object placement");
	}
	LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "ObjectPlaceNodes", "Switch to Place Nodes");
	RightClickHint = FText();
}

void AM_ObjectPlacer::leftClickPressed_Implementation()
{
	if (ObjectSpawned)
	{
		ObjectSpawned->SetActorLocation(getObjectLocation());
		GetNoxelNetworkingAgent()->MoveObject(FObjectPermissionDelegate(), FTransform(getObjectLocation()), ObjectSpawned, false);
		ObjectSpawned = nullptr;
	}
	else
	{
		switchMacro(AM_Nodes::StaticClass());
	}
}

void AM_ObjectPlacer::leftClickReleased_Implementation()
{
}

void AM_ObjectPlacer::middleClickPressed_Implementation()
{
}

void AM_ObjectPlacer::rightClickPressed_Implementation()
{
	if (ObjectSpawned)
	{
		GetNoxelNetworkingAgent()->RemoveObject(ObjectSpawned);
		ObjectSpawned = nullptr;
	}
}

void AM_ObjectPlacer::ObjectSelected(FNoxelObjectData Object)
{
	Inventory->RemoveFromParent();
	FVector Location, Direction;
	getRay(Location, Direction);
	GetNoxelNetworkingAgent()->AddObject(onObjectDelegate, Object.Class.Get(), FTransform(Location + Direction * placementDistance), true);
}

void AM_ObjectPlacer::NothingSelected()
{
	Inventory->RemoveFromParent();
	switchMacro(AM_Nodes::StaticClass());
}

void AM_ObjectPlacer::onObjectCall(AActor* Actor)
{
	UE_LOG(Noxel, Log, TEXT("[AM_ObjectPlacer::onObjectCall] ObjectCall, Server : %s"), GetWorld()->IsServer() ? TEXT("True") : TEXT("False"));
	ObjectSpawned = Actor;
	if (!ObjectSpawned)
	{
		UE_LOG(Noxel, Log, TEXT("[AM_ObjectPlacer::onObjectCall] ObjectSpawned is invalid"));
		return;
	}
	if (ObjectSpawned->IsA<ANoxelPart>())
	{
		UE_LOG(Noxel, Log, TEXT("[AM_ObjectPlacer::onObjectCall] Setting new CurrentPart"));
		GetOwningActor()->SetCurrentPart((ANoxelPart*)ObjectSpawned);
	}
	FVector BoundCenter, BoxExtent;
	ObjectSpawned->GetActorBounds(true, BoundCenter, BoxExtent);
	placementDistance = FMath::Max(BoxExtent.Size()*1.5f, 100.f);

}

FVector AM_ObjectPlacer::getObjectLocation()
{
	FVector Location, Direction;
	getRay(Location, Direction);
	return Location + Direction * placementDistance;
}
