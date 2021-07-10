//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once


#include "NoxelDataStructs.generated.h"

class UNodesContainer;
class UNoxelContainer;

UENUM(BlueprintType)
enum class ECraftSpawnContext : uint8 {
	None	UMETA(DisplayName = "Blank spawn context"),
	Editor	UMETA(DisplayName = "Loaded in editor"),
	Battle	UMETA(DisplayName = "Loaded in battle")
};

USTRUCT(BlueprintType)
struct NOXEL_API FNodeID
{

	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	UNodesContainer* Object;
	UPROPERTY(BlueprintReadWrite)
	FVector Location;

	FNodeID()
		: Object(),
		Location(FVector(0.0f, 0.0f, 0.0f))
	{}

	FNodeID(UNodesContainer* InObject, const FVector InLocation)
		: Object(InObject),
		Location(InLocation)
	{}

	FORCEINLINE bool operator== (const FNodeID& Other) const
	{
		return (Other.Object == Object && Other.Location == Location);
	}

	friend uint32 GetTypeHash(const FNodeID& Other)
	{
		return GetTypeHash(Other.Location);
	}

	static FNodeID FromWorld(UNodesContainer* InObject, FVector WorldLocation);

	FVector ToWorld() const;

	operator FVector&()
	{
		return Location;
	}
	operator FVector() const
	{
		return Location;
	}

	bool IsValid() const;

	FString ToString() const;

};

USTRUCT(BlueprintType)
struct NOXEL_API FPanelID
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	UNoxelContainer* Object;
	UPROPERTY(BlueprintReadWrite)
	int32 PanelIndex;

	FPanelID()
		:Object(nullptr),
		PanelIndex(INT32_MAX) 
	{}

	FPanelID(UNoxelContainer* InObject)
		:Object(InObject),
		PanelIndex(INT32_MAX)
	{}

	FPanelID(UNoxelContainer* InObject, const int32 InPanelIndex)
		:Object(InObject),
		PanelIndex(InPanelIndex)
	{}

	operator int32&()
	{
		return PanelIndex;
	}

	operator int32() const
	{
		return PanelIndex;
	}
};

//Data structure used by the nodes container to store nodes

USTRUCT(BlueprintType)
struct NOXEL_API FNodeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector Location;

	UPROPERTY(BlueprintReadWrite)
	FColor Color;

	UPROPERTY(BlueprintReadWrite)
	TArray<int32> ConnectedPanels;

	FNodeData()
		:Location(),
		Color(),
		ConnectedPanels()
	{}

	FNodeData(const FVector InLocation)
		:Location(InLocation),
		Color(),
		ConnectedPanels()
	{}

	FNodeData(const FVector InLocation, const FColor InColor)
		:Location(InLocation),
		Color(InColor),
		ConnectedPanels()
	{}

	FNodeData(const FVector InLocation, const FColor InColor, const TArray<FPanelID>& InConnectedPanels)
		:Location(InLocation),
		Color(InColor),
		ConnectedPanels(InConnectedPanels)
	{}

	FORCEINLINE bool operator== (const FNodeData& Other) const
	{
		return Other.Location == Location;
	}

	FORCEINLINE FNodeID ToNodeID(UNodesContainer* InObject)
	{
		return FNodeID(InObject, Location);
	}
};

USTRUCT(BlueprintType)
struct NOXEL_API FPanelData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 PanelIndex;
	UPROPERTY(BlueprintReadWrite)
	TArray<FNodeID> Nodes;
	UPROPERTY(BlueprintReadWrite)
	TArray<int32> ConnectedPanels; //Array of the other panels' PanelIndex, not index in the array
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThicknessNormal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThicknessAntiNormal;
	UPROPERTY(BlueprintReadWrite)
	FVector Center;
	UPROPERTY(BlueprintReadWrite)
	FVector Normal;
	UPROPERTY(BlueprintReadWrite)
	float Area;
	UPROPERTY(BlueprintReadWrite)
	bool Virtual;

	FPanelData() 
		:PanelIndex(INT32_MAX),
		Nodes(),
		ConnectedPanels(),
		ThicknessNormal(0.5f),
		ThicknessAntiNormal(0.5f),
		Center(FVector::ZeroVector),
		Normal(FVector(0, 0, 1)),
		Area(1.0f),
		Virtual(false)
	{}

	FPanelData(const TArray<FNodeID>& InNodes, const float InThickness, const bool InVirtual = false) 
		:PanelIndex(INT32_MAX),
		Nodes(InNodes),
		ConnectedPanels(),
		ThicknessNormal(InThickness/2.f),
		ThicknessAntiNormal(InThickness/2.f),
		Center(FVector::ZeroVector),
		Normal(FVector(0,0,1)),
		Area(1.0f),
		Virtual(InVirtual)
	{}

	FPanelData(const TArray<FNodeID>& InNodes, const float InThicknessNormal, const float InThicknessAntiNormal, const bool InVirtual = false)
		:PanelIndex(INT32_MAX),
		Nodes(InNodes),
		ConnectedPanels(),
		ThicknessNormal(InThicknessNormal),
		ThicknessAntiNormal(InThicknessAntiNormal),
		Center(FVector::ZeroVector),
		Normal(FVector(0, 0, 1)),
		Area(1.0f),
		Virtual(InVirtual)
	{}
};

//

USTRUCT(BlueprintType)
struct NOXEL_API FPanelNetworkingData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FPanelID Panel_ID;
	UPROPERTY(BlueprintReadWrite)
	FPanelData Panel_Data;
	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> Convexes;
	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> Nodes;
};


////////////////////////////////////////////////////////////////
// Networking --------------------------------------------------
////////////////////////////////////////////////////////////////

/*
* UPROPERTY is important, so that the conversion to JSON goes smoothly
*
*/

// Nodes ----------------------------------------------------------------
USTRUCT(BlueprintType)
struct NOXEL_API FNodeSavedRedirector {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 parentIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 nodesContainerIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 nodeIndex;

	FNodeSavedRedirector() {}

	FNodeSavedRedirector(const int32 InParentIndex, const int32 InNodesContainerIndex, const int32 InNodeIndex)
		: parentIndex(InParentIndex),
		nodesContainerIndex(InNodesContainerIndex),
		nodeIndex(InNodeIndex)
	{
	}

	FORCEINLINE bool operator== (const FNodeSavedRedirector& Other) const
	{
		return (Other.parentIndex == parentIndex && Other.nodesContainerIndex == nodesContainerIndex && Other.nodeIndex == nodeIndex);
	}

	friend uint32 GetTypeHash(const FNodeSavedRedirector& Other)
	{
		return GetTypeHash(FString::FromInt(Other.parentIndex) + " " + FString::FromInt(Other.nodesContainerIndex) + " " + FString::FromInt(Other.nodeIndex));
	}
};

USTRUCT(BlueprintType)
struct NOXEL_API FNodesContainerSave {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ComponentName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NodeSize = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> Nodes;

	FNodesContainerSave() {}

	FNodesContainerSave(const FString InComponentName, const float InNodeSize = 10.0f)
		: ComponentName(InComponentName),
		NodeSize(InNodeSize)
	{
	}
};

// Noxel ----------------------------------------------------------------
USTRUCT(BlueprintType)
struct NOXEL_API FPanelSavedData {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PanelIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FNodeSavedRedirector> Nodes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThicknessNormal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThicknessAntiNormal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Virtual;

	FPanelSavedData() {}

	FPanelSavedData(const FPanelData data)
	{
		PanelIndex = data.PanelIndex;
		ThicknessNormal = data.ThicknessNormal;
		ThicknessAntiNormal = data.ThicknessAntiNormal;
		Virtual = data.Virtual;
	}
};

USTRUCT(BlueprintType)
struct NOXEL_API FNoxelContainerSave {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ComponentName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPanelSavedData> Panels;

	FNoxelContainerSave() {}

	FNoxelContainerSave(FString name) {
		ComponentName = name;
	};
};

// Connectors ----------------------------------------------------------------

/* Holds on the male side about what connector is connected, so these are refs to the females */
USTRUCT(BlueprintType)
struct NOXEL_API FConnectorSavedRedirector {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) //Name of the (male) connector
	FString ConnectorName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) //Index of the parent object
	TArray<int32> parentIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) //Name of the (female) connector that it is connected to
	TArray<FString> OtherConnectorName;

	FConnectorSavedRedirector() {}

	FConnectorSavedRedirector(const FString InConnectorName)
		: ConnectorName(InConnectorName)
	{
	}

	FORCEINLINE bool operator== (const FConnectorSavedRedirector& Other) const
	{
		return (Other.ConnectorName == ConnectorName/* && Other.parentIndex == parentIndex && Other.OtherConnectorName == OtherConnectorName*/);
	}

	friend uint32 GetTypeHash(const FConnectorSavedRedirector& Other)
	{
		return GetTypeHash(Other.ConnectorName);
	}
};


// Networking ----------------------------------------------------------------
USTRUCT(BlueprintType)
struct NOXEL_API FNoxelNetwork {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UNodesContainer*> NodesConnected;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FNodesContainerSave> NodesSave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNoxelContainer* Noxel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FNoxelContainerSave NoxelSave;
};

USTRUCT(BlueprintType)
struct FNodesNetwork {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNodesContainer* Nodes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FNodesContainerSave NodesSave;
};

// Saving ----------------------------------------------------------------
USTRUCT(BlueprintType)
struct NOXEL_API FComponentSave {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ComponentID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform ComponentLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FNodesContainerSave> SavedNodes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FNoxelContainerSave> SavedNoxels;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FConnectorSavedRedirector> SavedConnectors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SavedMetadata;

	FComponentSave() {}

	FComponentSave(FString InComponentID, FTransform InComponentLocation)
	{
		ComponentID = InComponentID;
		ComponentLocation = InComponentLocation;
	}

	FComponentSave(const class AActor* InComponent)
		: ComponentLocation(InComponent->GetTransform()) {}
};

USTRUCT(BlueprintType)
struct NOXEL_API FCraftSave {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CraftName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CraftScale = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FComponentSave> Components;

	FCraftSave() {}

	FCraftSave(FString InCraftName, float InCraftScale = 10.0f) {
		CraftName = InCraftName;
		CraftScale = InCraftScale;
	}
};