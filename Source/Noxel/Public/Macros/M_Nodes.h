//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Macros/NoxelMacroBase.h"
#include "M_Nodes.generated.h"

UENUM(BlueprintType)
enum class EActionStartType : uint8 {
	AimingNothing,
	AimingNode,
	AimingPanel
};
/**
 * 
 */
UCLASS(ClassGroup = "Noxel Macros")
class NOXEL_API AM_Nodes : public ANoxelMacroBase
{
	GENERATED_BODY()

public:
	AM_Nodes();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	TArray<FNodeID> selectedNodes;
	float placementDistance = 100.0f;
	/*FVector actionStartLocation;
	EActionStartType actionType;
	bool bIsLongPress;*/

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void leftClickPressed_Implementation() override;

	virtual void leftClickReleased_Implementation() override;

	virtual void middleClickPressed_Implementation() override;

	virtual void rightClickPressed_Implementation() override;

};
