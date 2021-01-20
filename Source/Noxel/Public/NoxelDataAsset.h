//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "NoxelDataAsset.generated.h"

UENUM(BlueprintType)
enum class EObjectType : uint8 {
	deprecated_object,
	live_object,
	editor_object
};

USTRUCT(BlueprintType)
struct FNoxelObjectData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString ComponentID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSoftClassPtr<AActor> Class;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EObjectType ObjectType = EObjectType::editor_object;
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class NOXEL_API UNoxelDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, NoClear)
		TArray<FNoxelObjectData> Objects;
	
	UFUNCTION(BlueprintPure)
		bool HasClass(TSubclassOf<AActor> CompClass);

	UFUNCTION(BlueprintPure)
		FString getComponentIDFromClass(TSubclassOf<AActor> CompClass);

	UFUNCTION(BlueprintPure)
		TSubclassOf<AActor> getClassFromComponentID(FString CompID);

	static bool HasClass(UDataTable* Object, TSubclassOf<AActor> CompClass);

	static FString getComponentIDFromClass(UDataTable* Object, TSubclassOf<AActor> CompClass);

	static TSubclassOf<AActor> getClassFromComponentID(UDataTable* Object, FString CompID);
};
