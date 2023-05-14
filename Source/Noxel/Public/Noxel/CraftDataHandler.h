//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Noxel/NoxelDataStructs.h"

#include "Engine/World.h"
#include "Components/ActorComponent.h"

#include "CraftDataHandler.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCraftLoadedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FComponentReplicatedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEditorQueueExternalRunEvent);

UENUM()
enum ECraftDiagnosisSeverity
{
	Error,
	Warning,
	Info,
	Verbose
};

USTRUCT()
struct FCraftDiagnosisData
{
	GENERATED_BODY()

	ECraftDiagnosisSeverity Severity;
	FText Message;

	FORCEINLINE operator<(FCraftDiagnosisData other) const
	{
		
	}
};

UCLASS(ClassGroup = "Noxel", Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class NOXEL_API UCraftDataHandler: public UActorComponent
{
public:

	GENERATED_BODY()

	UCraftDataHandler();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	class UDataTable* DataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECraftSpawnContext SpawnContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated)
	float Scale;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Components)
	TArray<AActor*> Components;

	UPROPERTY(BlueprintAssignable)
	FCraftLoadedEvent OnCraftLoadedEvent;
	
	UPROPERTY(BlueprintAssignable)
	FComponentReplicatedEvent OnComponentsReplicatedEvent;

	//Called when a queue will be run from another player
	UPROPERTY(BlueprintAssignable)
	FEditorQueueExternalRunEvent OnReceiveQueueStart;
	//Called after a queue was run from another player
	UPROPERTY(BlueprintAssignable)
	FEditorQueueExternalRunEvent OnReceiveQueueEnd;

	////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintPure)
	bool AreAllComponentsValid();

	UFUNCTION(BlueprintPure)
	TArray<AActor*> GetComponents();

	UFUNCTION(BlueprintPure)
	TArray<class ANoxelPart*> GetParts();

	bool HasAnyDataComponentConnected(AActor* Component);

	bool HasAnyConnectorConnected(AActor* Component);
	
	//Add a component and registers it in the craft
	AActor* AddComponent(TSubclassOf<AActor> Class, FTransform Location, FActorSpawnParameters SpawnParameters, bool bSetSpawnContext = true, bool bFinishSpawning = true);

	//Removes a component from the craft
	bool RemoveComponent(AActor* Component);

	AActor* AddComponentFromComponentID(FString ComponentID, FTransform Location);

	bool MoveComponent(AActor* Component, FTransform Location);

	bool RemoveComponentIfUnconnected(AActor* Component);
	
	////////////////////////////////////////////////////////////////

	void SetNodesContainersVisibility(bool NewVisibility);

	void SetNoxelContainersVisibility(bool NewVisibility);

	////////////////////////////////////////////////////////////////

	void destroyCraft();

	UFUNCTION(BlueprintCallable)
		FCraftSave saveCraft();

	UFUNCTION(BlueprintCallable)
		void loadCraft(FCraftSave Craft, FTransform transform);

	UFUNCTION(BlueprintCallable)
		void enableCraft();

	////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintCallable)
		static FNoxelNetwork saveNoxelNetwork(UNoxelContainer* noxel);

	UFUNCTION(BlueprintCallable)
		static bool loadNoxelNetwork(FNoxelNetwork save);


	UFUNCTION(BlueprintCallable)
		static FNodesNetwork saveNodesNetwork(UNodesContainer* nodes);

	UFUNCTION(BlueprintCallable)
		static void loadNodesNetwork(FNodesNetwork save);

	////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintPure)
	static FString getCraftSaveLocation();

	UFUNCTION(BlueprintCallable)
	static TArray<FString> getSavedCrafts();

	UFUNCTION(BlueprintCallable)
	static bool getCraftSave(FString path, FCraftSave& Save);

	UFUNCTION(BlueprintCallable)
	static void setCraftSave(FString path, FCraftSave Save);

	UFUNCTION(BlueprintPure)
	static FCraftSave GetDefaultCraftSave();

private:

	UFUNCTION()
	virtual void OnRep_Components();
	
	static void saveNodesContainer(UNodesContainer* NodesContainer, int32 parentIndex, int32 nodesContainerIndex, FNodesContainerSave& SavedData, TMap<FNodeID, FNodeSavedRedirector>& RedirectorMap);

	static void saveNoxelContainer(UNoxelContainer* NoxelContainer, TMap<FNodeID, FNodeSavedRedirector>& RedirectorMap, FNoxelContainerSave& SavedData);

	static void saveConnector(class UConnectorBase* Connector, TArray<AActor*>& Components, FConnectorSavedRedirector& SavedData);

	static bool loadNodesContainer(UNodesContainer* NodesContainer, int32 parentIndex, int32 nodesContainerIndex, FNodesContainerSave SavedData, TMap<FNodeSavedRedirector, FNodeID>& RedirectorMap);

	static bool loadNoxelContainer(UNoxelContainer* NoxelContainer, TMap<FNodeSavedRedirector, FNodeID>& RedirectorMap, FNoxelContainerSave SavedData);

	static void loadConnector(class UConnectorBase* Connector, TArray<AActor*>& Components, FConnectorSavedRedirector SavedData);


public:

	static void nodeSaveToText(FNodesContainerSave SavedData);

	static void noxelSaveToText(FNoxelContainerSave SavedData);

	static void noxelNetworkToText(FNoxelNetwork SavedData);

	static void craftSaveToText(FCraftSave save);
};
