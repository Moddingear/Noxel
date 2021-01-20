// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Noxel/NoxelDataStructs.h"

#include "EditorCommandQueue.generated.h"

UENUM()
enum class EEditorQueueOrderType : uint8
{
	None				UMETA(DisplayName = "None"),
	NodeAdd 			UMETA(DisplayName = "Add Node"), //Elementary actions
	NodeRemove 			UMETA(DisplayName = "Remove Node"),
	PanelAdd			UMETA(DisplayName = "Add Node"),
	PanelRemove 		UMETA(DisplayName = "Remove Node"),
	NodeMove			UMETA(DisplayName = "Move Node"), //Derived actions
	PanelMove 			UMETA(DisplayName = "Move Panel"),
	PanelThickness 		UMETA(DisplayName = "Change Panel Thickness"),
	PanelVirtual 		UMETA(DisplayName = "Change Panel Virtuality")
};

class UEditorCommandQueue;
class UNodesContainer;
class UNoxelContainer;

struct FEditorQueueOrderNetworkable;
struct FEditorQueueNetworkable;
struct FEditorQueueOrderTemplate;

USTRUCT()
struct NOXEL_API FEditorQueueOrderNetworkable
{
	GENERATED_BODY()

	UPROPERTY()
	EEditorQueueOrderType OrderType;

	UPROPERTY()
	TArray<int32> intArgs;

	UPROPERTY()
	TArray<float> floatArgs;

	UPROPERTY()
	TArray<UObject*> ptrArgs;

	FEditorQueueOrderNetworkable()
		:OrderType(EEditorQueueOrderType::None),
		intArgs(),
		floatArgs(),
		ptrArgs()
	{}

	FEditorQueueOrderNetworkable(const EEditorQueueOrderType InType)
		:OrderType(InType)
	{}

	FEditorQueueOrderNetworkable(const EEditorQueueOrderType InType, const TArray<int32>& InIntArgs, const TArray<float>& InFloatArgs, const TArray<UObject*>& InPtrArgs)
		:OrderType(InType),
		intArgs(InIntArgs),
		floatArgs(InFloatArgs),
		ptrArgs(InPtrArgs)
	{}

	void AddVector(FVector InVector)
	{
		floatArgs.Append({ InVector.X, InVector.Y, InVector.Z });
	}

	FVector PopVector()
	{
		FVector vec;
		vec.Z = floatArgs.Pop();
		vec.Y = floatArgs.Pop();
		vec.X = floatArgs.Pop();
		return vec;
	}

	void AddNode(FNodeID Node)
	{
		AddVector(Node.Location);
		ptrArgs.Add(Cast<UObject>(Node.Object));
	}

	FNodeID PopNode()
	{
		FNodeID node = FNodeID();
		node.Location = PopVector();
		node.Object = Cast<UNodesContainer>(ptrArgs.Pop());
		return node;
	}
};

USTRUCT()
struct NOXEL_API FEditorQueueNetworkable
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FEditorQueueOrderNetworkable> Orders;

	FEditorQueueNetworkable()
		:Orders()
	{}

	void AddOrder(FEditorQueueOrderTemplate* Order);
	static bool OrderFromNetworkable(FEditorQueueOrderNetworkable Networkable, FEditorQueueOrderTemplate* OutOrder);

	void AddNodeAddOrder(FNodeID NodeToAdd);
	void AddNodeRemoveOrder(FNodeID NodeToRemove);

	void AddPanelAddOrder(UNoxelContainer* InObject, const TArray<FNodeID> &InNodes,
		const float InThicknessNormal, const float InThicknessAntiNormal, const bool InVirtual);
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

	virtual bool ExecuteOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList) //66
	{
		return false;
	};

	virtual bool UndoOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList)
	{
		return true;
	};

	virtual FEditorQueueOrderNetworkable ToNetworkable()
	{
		return FEditorQueueOrderNetworkable(OrderType);
	}

	virtual bool FromNetworkable(FEditorQueueOrderNetworkable& InData)
	{
		OrderType = InData.OrderType;
		return true;
	}

	virtual FString ToString()
	{
		return FString::Printf(TEXT("QueueOrderType = %i"), OrderType);
	};
};

struct NOXEL_API FEditorQueueOrderNodeAdd : public FEditorQueueOrderTemplate
{
	FNodeID NodeToAdd;

	FEditorQueueOrderNodeAdd()
		:FEditorQueueOrderTemplate(EEditorQueueOrderType::NodeAdd),
		NodeToAdd()
	{};

	FEditorQueueOrderNodeAdd(FNodeID InNodeToAdd)
		:FEditorQueueOrderTemplate(EEditorQueueOrderType::NodeAdd),
		NodeToAdd(InNodeToAdd)
	{};

	virtual ~FEditorQueueOrderNodeAdd() {};

	virtual bool ExecuteOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList) override;

	virtual bool UndoOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList) override;

	virtual FEditorQueueOrderNetworkable ToNetworkable() override;

	virtual bool FromNetworkable(FEditorQueueOrderNetworkable& InData) override;

	virtual FString ToString() override;

};

struct NOXEL_API FEditorQueueOrderNodeRemove : public FEditorQueueOrderTemplate
{
	FNodeID NodeToRemove;

	FEditorQueueOrderNodeRemove()
		:FEditorQueueOrderTemplate(EEditorQueueOrderType::NodeRemove),
		NodeToRemove()
	{};

	FEditorQueueOrderNodeRemove(const FNodeID InNodeToRemove)
		:FEditorQueueOrderTemplate(EEditorQueueOrderType::NodeRemove),
		NodeToRemove(InNodeToRemove)
	{};

	virtual ~FEditorQueueOrderNodeRemove() {};

	virtual bool ExecuteOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList) override;

	virtual bool UndoOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList) override;

	virtual FEditorQueueOrderNetworkable ToNetworkable() override;

	virtual bool FromNetworkable(FEditorQueueOrderNetworkable& InData) override;

	virtual FString ToString() override;

};


struct NOXEL_API FEditorQueueOrderPanelAdd : public FEditorQueueOrderTemplate
{
	UNoxelContainer* Object;
	TArray<FNodeID> Nodes;
	float ThicknessNormal;
	float ThicknessAntiNormal;
	bool Virtual;

	FEditorQueueOrderPanelAdd()
		:FEditorQueueOrderTemplate(EEditorQueueOrderType::PanelAdd),
		Object(),
		Nodes(),
		ThicknessNormal(0.5f),
		ThicknessAntiNormal(0.5f),
		Virtual(false)
	{};

	FEditorQueueOrderPanelAdd(UNoxelContainer* InObject, const TArray<FNodeID> &InNodes, 
		const float InThicknessNormal, const float InThicknessAntiNormal, const bool InVirtual)
		:FEditorQueueOrderTemplate(EEditorQueueOrderType::PanelAdd),
		Object(InObject),
		Nodes(InNodes),
		ThicknessNormal(InThicknessNormal),
		ThicknessAntiNormal(InThicknessAntiNormal),
		Virtual(InVirtual)
	{};

	virtual ~FEditorQueueOrderPanelAdd() {};

	virtual bool ExecuteOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList) override;

	virtual bool UndoOrder(const TArray<FEditorQueueOrderTemplate*> &OrderList) override;

	virtual FEditorQueueOrderNetworkable ToNetworkable() override;

	virtual bool FromNetworkable(FEditorQueueOrderNetworkable& InData) override;

	virtual FString ToString() override;
};
