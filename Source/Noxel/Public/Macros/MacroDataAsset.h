//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"
#include "MacroDataAsset.generated.h"

class ANoxelMacroBase;

USTRUCT(BlueprintType)
struct NOXEL_API FMacroInfo {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText macroName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<class ANoxelMacroBase> macro;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool editorOnly;
};

/**
 * 
 */
UCLASS(BlueprintType)
class NOXEL_API UMacroDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FMacroInfo> Macros;
	
	UFUNCTION(BlueprintCallable)
		TArray<FMacroInfo> getMacros();
};
