//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "EditorGameState.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class NOXEL_API AEditorGameState : public AGameState
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSoftObjectPtr<class ANoxelHangarBase> HangarSoft;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	class ANoxelHangarBase* Hangar;

	UFUNCTION(BlueprintPure)
	class ANoxelHangarBase* GetHangar();

	UFUNCTION(BlueprintPure)
	class UCraftDataHandler* GetCraft();
};
