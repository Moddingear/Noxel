//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Macros/NoxelMacroBase.h"
#include "M_Connector.generated.h"

class USplineMeshComponent;
class UConnectorBase;

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

	TArray<UStaticMeshComponent*> ConnectorsMeshes;
	TArray<UConnectorBase*> DisplayedConnectors;
	TArray<USplineMeshComponent*> WiresMeshes;

	UConnectorBase* SelectedConnector;

	TArray<UConnectorBase*> GetAllConnectors();

	void UpdateDisplayedConnectors();

	void ShowOnlyConnectableConnectors();

	void DestroyAllWires();

	USplineMeshComponent* MakeWire(FTransform Sender, FTransform Receiver, UStaticMesh* WireMesh);

	void MakeAllWires();

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