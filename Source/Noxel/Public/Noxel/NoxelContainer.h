//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NoxelDataComponent.h"

#include "NoxelContainer.generated.h"

class UNoxelRMCProvider;

UCLASS(ClassGroup = "Noxel", Blueprintable, meta=(BlueprintSpawnableComponent) )
class NOXEL_API UNoxelContainer : public UNoxelDataComponent
{
	GENERATED_BODY()

public:	

	UNoxelContainer(const FObjectInitializer & ObjectInitializer = FObjectInitializer::Get());

protected:

	void OnRegister() override;
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	//UPROPERTY(BlueprintReadWrite)
	TArray<FPanelData> Panels;

	TArray<UNodesContainer*> ConnectedNodesContainers; //Used for networking, automatically populated by the nodes container, though unlikely to run out

private:

	UNoxelRMCProvider* NoxelProvider;

	TArray<int32> UnusedIndices; //Reuse PanelIndex indices when they got deleted
	int32 MaxIndex; //Maximum given index, to use when UnusedIndices is empty

	//OutPanels is an array of PanelIndex
	bool FindPanelsByNodes(TArray<FNodeID>& Nodes, TArray<int32>& OutPanels, TArray<int32>& OutOccurences,
		TArray<TArray<FNodeID>>& OutNodesAttachedBy);

	//Gives the index in Panels of the panel with the wanted PanelIndex
	bool GetIndexOfPanelByPanelIndex(int32 PanelIndex, int32& OutIndexInArray);

public:
	UFUNCTION(BlueprintCallable)
	bool AddPanel(FPanelData data);

	UFUNCTION(BlueprintCallable)
	bool RemovePanel(int32 index);

	//Does not allow modification
	UFUNCTION(BlueprintCallable)
	bool GetPanelByPanelIndex(int32 PanelIndex, FPanelData& FoundPanel);

	UFUNCTION(BlueprintCallable)
	bool GetPanelByNodes(TArray<FNodeID> Nodes, int32& PanelIndex);

	UFUNCTION(BlueprintPure)
	TArray<FPanelData> GetPanels();

	UFUNCTION(BlueprintCallable)
	bool getPanelHit(FHitResult hit, FPanelID& PanelHit);

	UFUNCTION(BlueprintPure)
	TArray<UNodesContainer*> GetConnectedNodesContainers();

	void Empty();

private:
	void UpdateProviderData();

public:
	friend class UNodesContainer;
};
