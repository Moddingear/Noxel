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

	UNoxelContainer();

protected:

	void OnRegister() override;
	// Called when the game starts
	virtual void BeginPlay() override;

	
	
	UFUNCTION()
	void OnRep_ConnectedNodesContainers();

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	//UPROPERTY(BlueprintReadWrite)
	TArray<FPanelData> Panels;

	UPROPERTY(ReplicatedUsing=OnRep_ConnectedNodesContainers)
	TArray<UNodesContainer*> ConnectedNodesContainers; //Used for networking, automatically populated by the nodes container, though unlikely to run out

	UNoxelRMCProvider* NoxelProvider;

	TArray<int32> UnusedIndices; //Reuse PanelIndex indices when they got deleted
	TArray<int32> ReservedIndices;
	int32 MaxIndex; //Maximum given index, to use when UnusedIndices is empty

	TArray<int32> ModifiedPanels;

	TArray<int32> DifferedPanels;

	//OutPanels is an array of PanelIndex, outOccurences is the number of nodes this panel shares with this collection,
	//OutNodesAttachedBy are the nodes shared, IgnoreFilter is an array of PanelIndex to ignore
	static bool FindPanelsByNodes(TArray<FNodeID>& Nodes, TArray<int32>& OutPanels, TArray<int32>& OutOccurences,
	                              TArray<TArray<FNodeID>>& OutNodesAttachedBy, TArray<int32> IgnoreFilter);

	//Gives the index in Panels of the panel with the wanted PanelIndex
	bool GetIndexOfPanelByPanelIndex(int32 PanelIndex, int32& OutIndexInArray);

private:
	
	bool IsPanelValid(FPanelData &data) const;

	//Checks panel validity and outputs intermediate adjacency computations
	bool IsPanelValid(FPanelData &data, TArray<int32> &AdjacentPanels,TArray<int32> &Occurrences, TArray<TArray<FNodeID>> &NodesAttachedBy) const;

	//Fits a plane, reorder nodes and computes area
	void ComputePanelGeometricData(FPanelData &data);

	//Fills the adjacent panels information from given adjacency computations 
	void GetAdjacentPanelsFromNodes(FPanelData &data, const TArray<int32> &AdjacentPanels, const TArray<int32> &Occurrences, const TArray<TArray<FNodeID>> &NodesAttachedBy);

	//Pops an unused index or gives a new one
	int32 GetNewPanelIndex();

public:
	TArray<int32> ReservePanelIndices(int32 Num);
	bool ReservePanelIndices(TArray<int32> IndicesToReserve);
	//Allocates a panel for use with differing functions
	bool AddPanelDiffered(int32 Index);
	//Checks the panel for validity
	bool FinishAddPanel(int32 Index);

	//Connect a node to a panel, checks that the NodeContainer can
	bool ConnectNodeDiffered(int32 Index, FNodeID Node);

	//disconnects a node from a panel
	bool DisconnectNodeDiffered(int32 Index, FNodeID Node);

	bool SetPanelPropertiesDiffered(int32 Index, float ThicknessNormal, float ThicknessAntiNormal, bool Virtual);
	

	//Removes a panel, only works if the panel is already disconnected
	bool RemovePanelDiffered(int32 Index);

	//Only use for saving or loading
	//Automatically allocates an index for it
	UFUNCTION(BlueprintCallable)
	bool AddPanel(FPanelData data);

	//Only use for saving or loading
	UFUNCTION(BlueprintCallable)
	bool RemovePanel(int32 index);

	//Does not allow modification
	UFUNCTION(BlueprintCallable)
	bool GetPanelByPanelIndex(int32 PanelIndex, FPanelData& FoundPanel);

	UFUNCTION(BlueprintCallable)
	bool GetPanelByNodes(TArray<FNodeID> Nodes, int32& PanelIndex);

	UFUNCTION(BlueprintPure)
	TArray<FPanelData> GetPanels() const;

	UFUNCTION(BlueprintCallable)
	static bool GetPanelHit(FHitResult hit, FPanelID& PanelHit);

	UFUNCTION(BlueprintPure)
	TArray<UNodesContainer*> GetConnectedNodesContainers();

	void Empty();

	

private:
	void UpdateProviderData();

	//virtual UBodySetup* GetBodySetup() override;
	
public:
	virtual bool IsConnected() override;

	virtual bool CheckDataValidity() override;

	virtual void UpdateMesh() override;
	
	friend class UNodesContainer;
	friend class ANoxelPlayerController;
};
