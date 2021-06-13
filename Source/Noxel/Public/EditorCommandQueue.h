// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Noxel.h"
#include "Noxel/NoxelDataStructs.h"

#include "EditorCommandQueue.generated.h"

class UCraftDataHandler;
class UConnectorBase;
DECLARE_LOG_CATEGORY_EXTERN(LogEditorCommandQueue, Log, All);

UENUM()
enum class EEditorQueueOrderType : uint8
{
	None				UMETA(DisplayName = "None"),
	NodeReference		UMETA(DisplayName = "Reference Nodes"),
	NodeAdd 			UMETA(DisplayName = "Add Node"), //Elementary actions
	NodeRemove 			UMETA(DisplayName = "Remove Node"),
	NodeConnect			UMETA(DisplayName = "Connect Node"),
	NodeDisconnect 		UMETA(DisplayName = "Disconnect Node"),
	PanelReference		UMETA(DisplayName = "Reference Panels"),
	PanelAdd			UMETA(DisplayName = "Add Panel"),
	PanelRemove 		UMETA(DisplayName = "Remove Panel"),
	PanelProperties 	UMETA(DisplayName = "Change Panel Properties"),
	ObjectAdd			UMETA(DisplayName = "Add Object"), //TODO
	ObjectRemove		UMETA(DisplayName = "Remove Object"),
	ConnectorConnect	UMETA(DisplayName = "Connect Connector"), //TODO
	ConnectorDisconnect	UMETA(DisplayName = "Disconnect Connector")
};

class UNoxelDataComponent;
class UEditorCommandQueue;
class UNodesContainer;
class UNoxelContainer;

struct FEditorQueueOrderNetworkable;
struct FEditorQueueNetworkable;
struct FEditorQueueOrderTemplate;
struct FEditorQueue;

USTRUCT()
struct NOXEL_API FEditorQueueOrderNetworkable
{
	GENERATED_BODY()

	UPROPERTY()
	EEditorQueueOrderType OrderType;

	UPROPERTY()
	TArray<int32> Args;

	FEditorQueueOrderNetworkable()
		:OrderType(EEditorQueueOrderType::None),
		Args()
	{}

	FEditorQueueOrderNetworkable(const EEditorQueueOrderType InType)
		:OrderType(InType)
	{}

	FEditorQueueOrderNetworkable(const EEditorQueueOrderType InType, const TArray<int32>& InArgs)
		:OrderType(InType),
		Args(InArgs)
	{}

	void AddFloat(float InFloat)
	{
		int32 i = *reinterpret_cast<int32*>(&InFloat);
		Args.Add(i);
	}

	float GetFloat(int32 index)
	{
		int32 i = Args[index];
		return *reinterpret_cast<float*>(&i);
	}

	float PopFloat()
	{
		int32 i = Args.Pop();
		return *reinterpret_cast<float*>(&i);
	}

	void AddVector(FVector InVector)
	{
		AddFloat(InVector.X);
		AddFloat(InVector.Y);
		AddFloat(InVector.Z);
	}

	FVector PopVector()
	{
		FVector vec;
		vec.Z = PopFloat();
		vec.Y = PopFloat();
		vec.X = PopFloat();
		return vec;
	}

	void AddNode(FVector Location, int32 ObjectPtrIdx)
	{
		AddVector(Location);
		Args.Add(ObjectPtrIdx);
	}

	void PopNode(FVector& Location, int32& ObjectPtrIdx)
	{
		ObjectPtrIdx = Args.Pop();
		Location = PopVector();
	}

	FString ToString()
	{
		FString ArgsString;
		for (int32 Arg : ArgsString)
		{
			ArgsString += FString::Printf(TEXT("%d; "), Arg);
		}
		return FString::Printf(TEXT("OrderType = %d; Args.Num() = %d; Args : {%s}"), OrderType, Args.Num(), *ArgsString);
	}
};

USTRUCT()
struct NOXEL_API FEditorQueueNetworkable
{
	GENERATED_BODY()

	UPROPERTY()
	int32 OrderNumber;

	UPROPERTY()
	TArray<UObject*> Pointers;
	
	UPROPERTY()
	TArray<FEditorQueueOrderNetworkable> Orders;

	FEditorQueueNetworkable()
		:OrderNumber(-1)
	{}

	int32 AddPointer(UObject* InPtr);
	UObject* GetPointer(int32 InIndex);

	//Decodes the instruction from a networkable, allocates inherited structs
	bool OrderFromNetworkable(int32 OrderIndex, FEditorQueueOrderTemplate** OutOrder);
	//Decodes all of the instructions, allocates space
	bool DecodeQueue(FEditorQueue** Decoded);
};

struct NOXEL_API FEditorQueue
{
	TArray<FEditorQueueOrderTemplate*> Orders;

	TArray<FNodeID> NodeReferences;
	TArray<FPanelID> PanelReferences;

	int32 OrderNumber;

	FEditorQueue()
		:OrderNumber(-1)
	{}

	~FEditorQueue();

	bool RunQueue(bool bShouldExecute);
	//Executes queue. If fail at some points, undoes
	bool ExecuteQueue();
	//Undo on queue. If fail at some point, redoes. Should not be called before Execute
	bool UndoQueue();

	void AddNodeReferenceOrder(TArray<FVector> Locations, UNodesContainer* Container);

	TMap<FNodeID, int32> CreateNodeReferenceOrdersFromNodeList(TArray<FNodeID> Nodes);

	static TArray<int32> NodeListToNodeReferences(TArray<FNodeID> Nodes, TMap<FNodeID, int32> NodeMap);

	void AddNodeAddOrder(TArray<int32> NodeToAdd);
	void AddNodeRemoveOrder(TArray<int32> NodeToRemove);

	void AddPanelReferenceOrder(TArray<int32> PanelIndices, UNoxelContainer* Container);

	void AddPanelAddOrder(TArray<int32> PanelIndexRef);
	void AddPanelRemoveOrder(TArray<int32> PanelIndexRef);
	void AddPanelPropertiesOrder(TArray<int32> PanelIndexRef, float ThicknessNormal, float ThicknessAntiNormal, bool Virtual);

	void AddNodeConnectOrder(TArray<int32> Nodes, TArray<int32> Panels);
	void AddNodeDisconnectOrder(TArray<int32> Nodes, TArray<int32> Panels);

	bool ToNetworkable(FEditorQueueNetworkable& Networkable);

	//Returns the reserved panels that were used in the Execute direction
	TArray<FPanelID> GetReservedPanelsUsed();
};



struct NOXEL_API FEditorQueueOrderTemplate
{
	EEditorQueueOrderType OrderType;

	FEditorQueueOrderTemplate()
		: OrderType(EEditorQueueOrderType::None)
	{}

	FEditorQueueOrderTemplate(const EEditorQueueOrderType InOrderType)
		: OrderType(InOrderType)
	{}

	virtual ~FEditorQueueOrderTemplate() {}
	
	virtual bool ExecuteOrder(FEditorQueue* Parent) //66
	{
		return false;
	};

	virtual bool UndoOrder(FEditorQueue* Parent)
	{
		return true;
	};

	virtual FEditorQueueOrderNetworkable ToNetworkable(FEditorQueueNetworkable* Parent)
	{
		return FEditorQueueOrderNetworkable(OrderType);
	}

	virtual bool FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex)
	{
		OrderType = Parent->Orders[OrderIndex].OrderType;
		return true;
	}

	virtual TArray<UNoxelDataComponent*> GetAffectedDataComponents(FEditorQueue* Parent)
	{
		return {};
	}

	virtual FString ToString()
	{
		return FString::Printf(TEXT("QueueOrderType = %i"), OrderType);
	}

	virtual TArray<FPanelID> GetReservedPanelsUsed(FEditorQueue* Parent)
	{
		return {};
	}
};

struct NOXEL_API FEditorQueueOrderArrayTemplate : public FEditorQueueOrderTemplate
{
	FEditorQueueOrderArrayTemplate()
		: FEditorQueueOrderTemplate()
	{}
	
	FEditorQueueOrderArrayTemplate(const EEditorQueueOrderType InOrderType)
		: FEditorQueueOrderTemplate(InOrderType)
	{}

	virtual bool PreArray(FEditorQueue* Parent)
	{
		UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueOrderArrayTemplate::PreArray] Not overload by order %d, will result in a fail"), OrderType);
		return false;
	}
	
	virtual int GetArrayLength(FEditorQueue* Parent)
	{
		UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueOrderArrayTemplate::GetArrayLength] Not overload by order %d, will result in a fail"), OrderType);
		return 0;
	}
	
	virtual bool ExecuteInArray(FEditorQueue* Parent, int i)
	{
		UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueOrderArrayTemplate::ExecuteInArray] Not overload by order %d, will result in a fail"), OrderType);
		return false;
	}

	virtual bool UndoInArray(FEditorQueue* Parent, int i)
	{
		UE_LOG(LogEditorCommandQueue, Warning, TEXT("[FEditorQueueOrderArrayTemplate::UndoInArray] Not overload by order %d, will result in a fail"), OrderType);
		return false;
	}

	virtual bool ExecuteOrder(FEditorQueue* Parent) override;

	virtual bool UndoOrder(FEditorQueue* Parent) override;

};

//Creates the references to node belonging to one nodes container
struct NOXEL_API FEditorQueueOrderNodeReference : public FEditorQueueOrderTemplate
{
	UNodesContainer* Container;
	TArray<FVector> Locations;

	FEditorQueueOrderNodeReference()
		: FEditorQueueOrderTemplate(EEditorQueueOrderType::NodeReference), Container(nullptr)
	{
	}

	FEditorQueueOrderNodeReference(TArray<FVector> InLocations, UNodesContainer* InContainer)
		:FEditorQueueOrderTemplate(EEditorQueueOrderType::NodeReference),
		Container(InContainer), Locations(InLocations)
	{}

	virtual ~FEditorQueueOrderNodeReference() override {}

	virtual bool ExecuteOrder(FEditorQueue* Parent) override;

	virtual bool UndoOrder(FEditorQueue* Parent) override;

	virtual FEditorQueueOrderNetworkable ToNetworkable(FEditorQueueNetworkable* Parent) override;

	virtual bool FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex) override;

	virtual TArray<UNoxelDataComponent*> GetAffectedDataComponents(FEditorQueue* Parent) override;

	virtual FString ToString() override;
};

//Adds or removes a number of nodes already referenced
struct NOXEL_API FEditorQueueOrderNodeAddRemove : public FEditorQueueOrderArrayTemplate
{
	TArray<int32> NodesToAddRemove;
	bool Add;

	FEditorQueueOrderNodeAddRemove()
		:FEditorQueueOrderArrayTemplate(EEditorQueueOrderType::NodeAdd),
		NodesToAddRemove(),
		Add(true)
	{}

	FEditorQueueOrderNodeAddRemove(TArray<int32> InNodesToAddRemove, bool InAdd)
		:FEditorQueueOrderArrayTemplate(InAdd ? EEditorQueueOrderType::NodeAdd : EEditorQueueOrderType::NodeRemove),
		NodesToAddRemove(InNodesToAddRemove),
		Add(InAdd)
	{}

	virtual ~FEditorQueueOrderNodeAddRemove() override {}

	virtual bool PreArray(FEditorQueue* Parent) override;

	virtual int GetArrayLength(FEditorQueue* Parent) override;

	virtual bool ExecuteInArray(FEditorQueue* Parent, int i) override;

	virtual bool UndoInArray(FEditorQueue* Parent, int i) override;
	
	virtual FEditorQueueOrderNetworkable ToNetworkable(FEditorQueueNetworkable* Parent) override;

	virtual bool FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex) override;

	virtual TArray<UNoxelDataComponent*> GetAffectedDataComponents(FEditorQueue* Parent) override;

	virtual FString ToString() override;

};

//Connects or disconnects a number of nodes to panels
struct NOXEL_API FEditorQueueOrderNodeDisConnect : public FEditorQueueOrderArrayTemplate
{
	TArray<int32> Nodes;
	TArray<int32> Panels;
	bool Connect;

	FEditorQueueOrderNodeDisConnect()
		:FEditorQueueOrderArrayTemplate(EEditorQueueOrderType::NodeConnect),
		Nodes(), Panels(),
		Connect(true)
	{}

	FEditorQueueOrderNodeDisConnect(TArray<int32> InNodes, TArray<int32> InPanels, bool InConnect)
		:FEditorQueueOrderArrayTemplate(InConnect ? EEditorQueueOrderType::NodeConnect : EEditorQueueOrderType::NodeDisconnect),
		Nodes(InNodes), Panels(InPanels),
		Connect(InConnect)
	{}

	virtual ~FEditorQueueOrderNodeDisConnect() override {}

	virtual bool PreArray(FEditorQueue* Parent) override;

	virtual int GetArrayLength(FEditorQueue* Parent) override;

	virtual bool ExecuteInArray(FEditorQueue* Parent, int i) override;

	virtual bool UndoInArray(FEditorQueue* Parent, int i) override;
	
	virtual FEditorQueueOrderNetworkable ToNetworkable(FEditorQueueNetworkable* Parent) override;

	virtual bool FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex) override;

	virtual TArray<UNoxelDataComponent*> GetAffectedDataComponents(FEditorQueue* Parent) override;

	virtual FString ToString() override;

};

//Creates the references to panels belonging to one NoxelContainer
struct NOXEL_API FEditorQueueOrderPanelReference : public FEditorQueueOrderTemplate
{
	UNoxelContainer* Container;
	TArray<int32> PanelIndices;

	FEditorQueueOrderPanelReference()
		: FEditorQueueOrderTemplate(EEditorQueueOrderType::PanelReference), Container(nullptr)
	{
	}

	FEditorQueueOrderPanelReference(TArray<int32> InPanelIndices, UNoxelContainer* InContainer)
		:FEditorQueueOrderTemplate(EEditorQueueOrderType::PanelReference),
		Container(InContainer), PanelIndices(InPanelIndices)
	{}

	virtual ~FEditorQueueOrderPanelReference() override {}

	virtual bool ExecuteOrder(FEditorQueue* Parent) override;

	virtual bool UndoOrder(FEditorQueue* Parent) override;

	virtual FEditorQueueOrderNetworkable ToNetworkable(FEditorQueueNetworkable* Parent) override;

	virtual bool FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex) override;

	virtual TArray<UNoxelDataComponent*> GetAffectedDataComponents(FEditorQueue* Parent) override;

	virtual FString ToString() override;
};

//Adds or removes a number of panels already referenced
struct NOXEL_API FEditorQueueOrderPanelAddRemove : public FEditorQueueOrderArrayTemplate
{
	TArray<int32> PanelIndexRef;
	bool Add;

	//Default constructor
	FEditorQueueOrderPanelAddRemove()
		: FEditorQueueOrderArrayTemplate(EEditorQueueOrderType::PanelAdd), Add(true)
	{
	}

	FEditorQueueOrderPanelAddRemove(TArray<int32> InPanelIndexRef, bool InAdd)
		: FEditorQueueOrderArrayTemplate(InAdd ? EEditorQueueOrderType::PanelAdd : EEditorQueueOrderType::PanelRemove),
		PanelIndexRef(InPanelIndexRef), Add(InAdd)
	{
	}

	virtual ~FEditorQueueOrderPanelAddRemove() override {}
	
	virtual bool PreArray(FEditorQueue* Parent) override;

	virtual int GetArrayLength(FEditorQueue* Parent) override;

	virtual bool ExecuteInArray(FEditorQueue* Parent, int i) override;

	virtual bool UndoInArray(FEditorQueue* Parent, int i) override;

	virtual FEditorQueueOrderNetworkable ToNetworkable(FEditorQueueNetworkable* Parent) override;

	virtual bool FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex) override;
	
	virtual TArray<UNoxelDataComponent*> GetAffectedDataComponents(FEditorQueue* Parent) override;

	virtual FString ToString() override;

	virtual TArray<FPanelID> GetReservedPanelsUsed(FEditorQueue* Parent) override;
};

//Sets new data to a number of panels
struct NOXEL_API FEditorQueueOrderPanelProperties : public FEditorQueueOrderArrayTemplate
{
	TArray<int32> PanelIndexRef;
	
	TArray<float> ThicknessNormalBefore;
	TArray<float> ThicknessAntiNormalBefore;
	TArray<bool> VirtualBefore;
	
	float ThicknessNormalAfter;
	float ThicknessAntiNormalAfter;
	bool VirtualAfter;

	FEditorQueueOrderPanelProperties()
		: FEditorQueueOrderArrayTemplate(EEditorQueueOrderType::PanelProperties),
		ThicknessNormalAfter(0), ThicknessAntiNormalAfter(0), VirtualAfter(false)
	{
	}

	FEditorQueueOrderPanelProperties(TArray<int32> InPanelIndexRef, float InThicknessNormalAfter, float InThicknessAntiNormalAfter, bool InVirtualAfter)
		:FEditorQueueOrderArrayTemplate(EEditorQueueOrderType::PanelProperties),
	PanelIndexRef(InPanelIndexRef),
	ThicknessNormalAfter(InThicknessNormalAfter), ThicknessAntiNormalAfter(InThicknessAntiNormalAfter),
	VirtualAfter(InVirtualAfter)
	{}

	virtual ~FEditorQueueOrderPanelProperties() override {}

	virtual bool PreArray(FEditorQueue* Parent) override;

	virtual int GetArrayLength(FEditorQueue* Parent) override;

	virtual bool ExecuteInArray(FEditorQueue* Parent, int i) override;

	virtual bool UndoInArray(FEditorQueue* Parent, int i) override;

	virtual FEditorQueueOrderNetworkable ToNetworkable(FEditorQueueNetworkable* Parent) override;

	virtual bool FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex) override;

	virtual TArray<UNoxelDataComponent*> GetAffectedDataComponents(FEditorQueue* Parent) override;

	virtual FString ToString() override;
};

//Connects or disconnects a number of connectors
struct NOXEL_API FEditorQueueOrderConnectorDisConnect : public FEditorQueueOrderArrayTemplate
{
	TArray<UConnectorBase*> A;
	TArray<UConnectorBase*> B;
	bool Connect;

	FEditorQueueOrderConnectorDisConnect()
		: FEditorQueueOrderArrayTemplate(EEditorQueueOrderType::ConnectorConnect), Connect(true)
	{
	}

	FEditorQueueOrderConnectorDisConnect(TArray<UConnectorBase*> InA, TArray<UConnectorBase*> InB, bool InConnect)
		:FEditorQueueOrderArrayTemplate(InConnect ? EEditorQueueOrderType::ConnectorConnect : EEditorQueueOrderType::ConnectorDisconnect),
	A(InA), B(InB), Connect(InConnect)
	{}

	virtual ~FEditorQueueOrderConnectorDisConnect() override {}

	virtual bool PreArray(FEditorQueue* Parent) override;
	
	virtual int GetArrayLength(FEditorQueue* Parent) override;

	virtual bool ExecuteInArray(FEditorQueue* Parent, int i) override;

	virtual bool UndoInArray(FEditorQueue* Parent, int i) override;

	virtual FEditorQueueOrderNetworkable ToNetworkable(FEditorQueueNetworkable* Parent) override;

	virtual bool FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex) override;

	virtual FString ToString() override;
};

struct NOXEL_API FEditorQueueOrderAddObject : public FEditorQueueOrderTemplate
{
	UCraftDataHandler* Craft; //TODO : Supply the craft elsewhere
	FString ObjectClass;
	FTransform ObjectTransform;
	AActor* SpawnedObject;

	FEditorQueueOrderAddObject()
		: FEditorQueueOrderTemplate(EEditorQueueOrderType::ObjectAdd), Craft(nullptr), SpawnedObject(nullptr)
	{
	}

	FEditorQueueOrderAddObject(FString InObjectClass, FTransform InObjectTransform)
		: FEditorQueueOrderTemplate(EEditorQueueOrderType::ObjectAdd), Craft(nullptr),
		  ObjectClass(InObjectClass), ObjectTransform(InObjectTransform), SpawnedObject(nullptr)
	{
	}

	virtual bool ExecuteOrder(FEditorQueue* Parent) override;

	virtual bool UndoOrder(FEditorQueue* Parent) override;

	virtual FEditorQueueOrderNetworkable ToNetworkable(FEditorQueueNetworkable* Parent) override;

	virtual bool FromNetworkable(FEditorQueueNetworkable* Parent, int32 OrderIndex) override;

	
};


