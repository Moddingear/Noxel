//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "NoxelContainer.h"

#include "CoreMinimal.h"
#include "NoxelDataComponent.h"

#include "NodesContainer.generated.h"

class UNodesRMCProvider;

UCLASS(ClassGroup = "Noxel", Blueprintable, meta=(BlueprintSpawnableComponent) )
class NOXEL_API UNodesContainer : public UNoxelDataComponent
{
	GENERATED_BODY()

public:	

	UNodesContainer(const FObjectInitializer & ObjectInitializer = FObjectInitializer::Get());

protected:

	void OnRegister() override;

	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void PostInitProperties() override;

public:

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	//Nodes belonging to this Nodes container
	UPROPERTY(EditDefaultsOnly)
	TArray<FNodeData> Nodes;

	//Nodes causing this Nodes Container to be attached to the NoxelContainer
	int32 NumNodesAttached;
	//Size of the nodes to be displayed (radius)
	UPROPERTY(EditAnywhere)
	float NodeSize;

	//Should this Nodes Container be editable by the player ?
	UPROPERTY(EditAnywhere)
	bool bPlayerEditable;

	FColor DefaultNodeColor;

	bool bIsMeshDirty;

private:
	UNodesRMCProvider* NodesProvider;

	//Noxel Container this Nodes container is attached to
	UPROPERTY(Replicated)
		UNoxelContainer* AttachedNoxel;
public:

	UPROPERTY(EditAnywhere)
	UStaticMesh* BaseNodeMesh; //Mesh used to render the nodes, has to have bAllowCPUAcess
	UPROPERTY(EditAnywhere)
	UMaterialInterface* NodeMaterial; //Material used to render the nodes

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetNodeSize() const
	{
		return NodeSize;
	}

	void SetNodeSize(float NewNodeSize);

	UNoxelContainer* GetAttachedNoxel();
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsPlayerEditable() const;

	bool AddNode(FVector Location);

	static bool AddNode(FNodeID Node);

	//Set the default configuration of nodes, use in constructor only
	bool SetNodesDefault(TArray<FVector> InNodes, bool bInPlayerEditable);

	bool AttachNode(FVector Location, FPanelID Panel);

	bool DetachNode(FVector Location, FPanelID Panel);

	//Does not allow removing a node that is connected to panels
	bool RemoveNode(FVector Location);

	static bool RemoveNode(FNodeID Node);

	bool SetNodeColor(FVector Location, FColor color);

	bool FindNode(FVector Location, FNodeID& FoundNode);

	TArray<int32> GetAttachedPanels(FVector Location); //Returns the panel index of the attached panels, belonging to the attached Noxel container

	TArray<FNodeID> GenerateNodesKeyArray();

	static bool GetNodeHit(FHitResult Hit, FNodeID& HitNode);

private:
	void MarkMeshDirty();

	void AttachToNoxelContainer(UNoxelContainer* NoxelContainer);

public:
	
	virtual void SetSpawnContext(ECraftSpawnContext Context) override;
	//Always returns true : the Nodes container blocks directly on modification
	virtual bool CheckDataValidity() override;

	virtual void UpdateMesh() override;

	//friend class UNoxelContainer; //Allow calls to AttachNode and DetachNode
};
