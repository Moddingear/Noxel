// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NObjects/NoxelPart.h"
#include "NoxelContainerTester.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class NOXEL_API ANoxelContainerTester : public ANoxelPart
{
	GENERATED_BODY()
	
public:
	ANoxelContainerTester();

	virtual void OnConstruction(const FTransform& Transform) override;
};
