//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "Styling/SlateTypes.h"

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UMGThemeDataAsset.generated.h"


/**
 * 
 */
UCLASS()
class NOXEL_API UUMGThemeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
		FButtonStyle ButtonStyle;
};
