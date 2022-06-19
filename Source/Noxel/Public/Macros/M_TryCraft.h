//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Macros/NoxelMacroBase.h"
#include "M_TryCraft.generated.h"

UCLASS(ClassGroup = "Noxel Macros")
class NOXEL_API AM_TryCraft : public ANoxelMacroBase
{
	GENERATED_BODY()

public:
	AM_TryCraft();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	// Called every frame
	virtual void Tick(float DeltaTime) override;

};