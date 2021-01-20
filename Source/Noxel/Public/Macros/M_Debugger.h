//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Macros/NoxelMacroBase.h"
#include "M_Debugger.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = "Noxel Macros")
class NOXEL_API AM_Debugger : public ANoxelMacroBase
{
	GENERATED_BODY()
	
public:
	AM_Debugger();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
public:

	virtual void leftClickPressed_Implementation() override;

	virtual void middleClickPressed_Implementation() override;

	virtual void rightClickPressed_Implementation() override;
};
