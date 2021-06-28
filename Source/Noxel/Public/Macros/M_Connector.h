//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Macros/NoxelMacroBase.h"
#include "M_Connector.generated.h"

UCLASS(ClassGroup = "Noxel Macros")
class NOXEL_API AM_Connector : public ANoxelMacroBase
{
	GENERATED_BODY()

public:
	AM_Connector();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	class UConnectorBase* SelectedConnector;

	float SphereSizeMin = 20;
	float SphereSizeMax = 100;

	float DistMin = 0;
	float DistMax = 2000;

	UMaterial* ConnectorMaterial;

private:

	TMap<int32, class UConnectorBase*> ConnectorSectionMap;

public:

	UConnectorBase* GetConnectorClicked();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void leftClickPressed_Implementation() override;

	virtual void leftClickReleased_Implementation() override;

	virtual void middleClickPressed_Implementation() override;

	virtual void middleClickReleased_Implementation() override;

	virtual void rightClickPressed_Implementation() override;

	virtual void rightClickReleased_Implementation() override;

};