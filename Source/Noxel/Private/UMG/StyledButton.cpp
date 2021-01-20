//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "UMG/StyledButton.h"
//#include "..\..\Public\UMG\StyledButton.h"


UStyledButton::UStyledButton() {
	static ConstructorHelpers::FObjectFinder<UUMGThemeDataAsset> DataConstructor(TEXT("UMGThemeDataAsset'/Game/UMG_Theme/UMG_Theme.UMG_Theme'"));
	if (DataConstructor.Succeeded()) {
		Theme = DataConstructor.Object;
	}
}

void UStyledButton::PostLoad()
{
	if (Theme) {
		SetStyle(Theme->ButtonStyle);
	}
	Super::PostLoad();
}
