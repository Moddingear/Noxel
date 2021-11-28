//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Macros/NoxelMacroBase.h"
#include "Noxel/NoxelNetworkingAgent.h"


#include "M_ObjectPlacer.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = "Noxel Macros")
class NOXEL_API AM_ObjectPlacer : public ANoxelMacroBase
{
	GENERATED_BODY()
	
public:
	AM_ObjectPlacer();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	float placementDistance = 100.0f;
	FVector BoundsCenter;
private:
	bool InventoryDisplayed = false;
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
		TSubclassOf<class UUserWidget> wInventory;

	UUserWidget* Inventory;

	FObjectPermissionDelegate onObjectDelegate;
	
private:
	FNoxelObjectData SelectedObject;
	
	AActor* ObjectSpawned;
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void leftClickPressed_Implementation() override;

	virtual void leftClickReleased_Implementation() override;

	virtual void middleClickPressed_Implementation() override;

	virtual void rightClickPressed_Implementation() override;

	FBox GetDefaultBounds(TSubclassOf<AActor> Class);
	
	UFUNCTION(BlueprintCallable)
	void ObjectSelected(FNoxelObjectData Object);

	UFUNCTION(BlueprintCallable)
	void NothingSelected();

	static bool LoadObjectClassSynchronous(TSoftClassPtr<AActor> SoftClass, TSubclassOf<AActor>& ObjectClass);

	UFUNCTION()
	void onObjectCall(AActor* Actor);

	FVector getObjectLocation();
};
