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

UENUM(BlueprintType)
enum class ECraftDiagnosisSeverity : uint8
{
	SetupError	UMETA(DisplayName = "Setup Error"),
	Error		UMETA(DisplayName = "Error"),
	Warning		UMETA(DisplayName = "Warning"),
	Info		UMETA(DisplayName = "Information"),
	Verbose		UMETA(DisplayName = "Verbose")
};

USTRUCT(BlueprintType)
struct FCraftDiagnosisData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	ECraftDiagnosisSeverity Severity;
	UPROPERTY(BlueprintReadWrite)
	FText Message;

	FCraftDiagnosisData()
		:Severity(ECraftDiagnosisSeverity::Verbose), Message()
	{
		
	}

	FCraftDiagnosisData(ECraftDiagnosisSeverity InSeverity, const FText &InMessage)
		:Severity(InSeverity), Message(InMessage)
	{
		
	}

	FORCEINLINE bool operator<(FCraftDiagnosisData other) const
	{
		return Severity < other.Severity;
	}
};

class UConnectorBase;

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

	static bool HasAnyDataComponentConnected(const AActor* Component);

	static bool HasAnyConnectorConnected(const AActor* Component);
	
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

	//save craft to file
	UFUNCTION(BlueprintCallable)
		FCraftSave saveCraft() const;

	//spawn craft components from save, load all nodes and noxels
	UFUNCTION(BlueprintCallable)
		void loadCraft(FCraftSave Craft, FTransform transform);

	UFUNCTION(BlueprintCallable)
	TArray<FCraftDiagnosisData> DiagnoseCraft() const;

	//attach components, enable physics
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
	
	static void saveNodesContainer(const UNodesContainer* NodesContainer, int32 parentIndex, int32 nodesContainerIndex, FNodesContainerSave& SavedData, TMap<FNodeID, FNodeSavedRedirector>& RedirectorMap);

	static void saveNoxelContainer(const UNoxelContainer* NoxelContainer, TMap<FNodeID, FNodeSavedRedirector>& RedirectorMap, FNoxelContainerSave& SavedData);

	static void saveConnector(const UConnectorBase* Connector, const TArray<AActor*>& Components, FConnectorSavedRedirector& SavedData);

	static bool loadNodesContainer(UNodesContainer* NodesContainer, int32 parentIndex, int32 nodesContainerIndex, FNodesContainerSave SavedData, TMap<FNodeSavedRedirector, FNodeID>& RedirectorMap);

	static bool loadNoxelContainer(UNoxelContainer* NoxelContainer, TMap<FNodeSavedRedirector, FNodeID>& RedirectorMap, FNoxelContainerSave SavedData);

	static void loadConnector(UConnectorBase* Connector, TArray<AActor*>& Components, FConnectorSavedRedirector SavedData);


public:

	static void nodeSaveToText(FNodesContainerSave SavedData);

	static void noxelSaveToText(FNoxelContainerSave SavedData);

	static void noxelNetworkToText(FNoxelNetwork SavedData);

	static void craftSaveToText(FCraftSave save);
};
