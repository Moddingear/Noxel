//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_ObjectPlacer.h"
#include "Runtime/UMG/Public/UMG.h"
#include "Macros/M_Nodes.h"
#include "NObjects/NoxelPart.h"

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
	InventoryDisplayed = true;
	onObjectDelegate.AddDynamic(this, &AM_ObjectPlacer::onObjectCall);
	//UE_LOG(NoxelMacro, Warning, TEXT("Macro added"));
}

void AM_ObjectPlacer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InventoryDisplayed && EndPlayReason != EEndPlayReason::EndPlayInEditor)
 	{
 		Inventory->RemoveFromParent();
		InventoryDisplayed = false;
 	}
 	if (IsValid(ObjectSpawned))
 	{
 		GetNoxelNetworkingAgent()->RemoveTempObject(ObjectSpawned);
 	}
	Super::EndPlay(EndPlayReason);
	
}

void AM_ObjectPlacer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ObjectSpawned)
	{
		GetNoxelNetworkingAgent()->MoveTempObject(ObjectSpawned, FTransform(getObjectLocation()));

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
		GetNoxelNetworkingAgent()->RemoveTempObject(ObjectSpawned);
		ObjectSpawned = nullptr;
		FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
		queue->AddObjectAddOrder(GetCraft(), SelectedObject.ComponentID, FTransform(getObjectLocation()));
		GetNoxelNetworkingAgent()->SendCommandQueue(queue);
	}
	else
	{
		SwitchMacro(AM_Nodes::StaticClass());
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
		GetNoxelNetworkingAgent()->RemoveTempObject(ObjectSpawned);
		ObjectSpawned = nullptr;
	}
}

void AM_ObjectPlacer::ObjectSelected(FNoxelObjectData Object)
{
	SelectedObject = Object;
	Inventory->RemoveFromParent();
	InventoryDisplayed = false;
	FVector Location, Direction;
	GetRayFromFollow(Location, Direction);
	FVector BoxExtent;
	TSubclassOf<AActor> SpawnClass;
	if (LoadObjectClassSynchronous(Object.Class, SpawnClass))
	{
		AActor* DefObj = SpawnClass->GetDefaultObject<AActor>();
		if (IsValid(DefObj))
		{
			DefObj->GetActorBounds(true, BoundsCenter, BoxExtent);
			placementDistance = FMath::Max(BoxExtent.Size()*1.5f, 100.f);
			UE_LOG(NoxelMacro, Log, TEXT("[AM_ObjectPlacer::ObjectSelected] Was able to get default object for placement distance"))
		}
	}
	GetNoxelNetworkingAgent()->AddTempObject(onObjectDelegate, SpawnClass, FTransform(getObjectLocation()));
}

void AM_ObjectPlacer::NothingSelected()
{
	Inventory->RemoveFromParent();
	InventoryDisplayed = false;
	SwitchMacro(AM_Nodes::StaticClass());
}

bool AM_ObjectPlacer::LoadObjectClassSynchronous(TSoftClassPtr<AActor> SoftClass, TSubclassOf<AActor>& ObjectClass)
{
	if (!SoftClass.IsNull())
	{
		if (SoftClass.IsPending())
		{
			// ReSharper disable once CppExpressionWithoutSideEffects
			SoftClass.LoadSynchronous();
		}
		if (SoftClass.IsValid())
		{
			UClass* ObjClass = SoftClass.Get();
			if (IsValid(ObjClass))
			{
				ObjectClass = ObjClass;
				return true;
			}
		}
	}
	return false;
}

void AM_ObjectPlacer::onObjectCall(AActor* Actor)
{
	UE_LOG(Noxel, Log, TEXT("[AM_ObjectPlacer::onObjectCall] ObjectCall, Server : %s"), GetWorld()->IsServer() ? TEXT("True") : TEXT("False"));
	ObjectSpawned = Actor;
	if (!ObjectSpawned)
	{
		UE_LOG(Noxel, Log, TEXT("[AM_ObjectPlacer::onObjectCall] ObjectSpawned is invalid"));
	}
}

FVector AM_ObjectPlacer::getObjectLocation()
{
	FVector Location, Direction;
	GetRayFromFollow(Location, Direction);
	return Location + Direction * placementDistance - BoundsCenter;
}