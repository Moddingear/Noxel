//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "UMGThemeDataAsset.h"

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "StyledButton.generated.h"

/**
 * 
 */
UCLASS()
class NOXEL_API UStyledButton : public UButton
{
	GENERATED_BODY()
	
public:

	UStyledButton();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
		UUMGThemeDataAsset* Theme;

	virtual void PostLoad() override;
};
