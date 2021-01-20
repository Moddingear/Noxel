//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Noxel/NoxelDataStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Internationalization/Text.h"
#include "NoxelCombatLibrary.generated.h"

/**
 * 
 */
class UCraftDataHandler;
class UConnectorBase;

UCLASS()
class NOXEL_API UNoxelCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
		static bool AreNodeConnected(FNodeID start, FNodeID end);
	
	UFUNCTION(BlueprintPure)
		static UCraftDataHandler* GetCraftDataHandler(AActor* CraftComponent);

	UFUNCTION(BlueprintCallable)
		static TArray<UConnectorBase*> GetConnectorsFromActor(AActor* Actor);

	//UFUNCTION(BlueprintCallable)
	//	static bool DiagnoseCraft(UCraftDataHandler* Craft, FText& OutCritical, FText& OutWarning, FText& OutLog);
};