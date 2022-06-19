// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "MovementNObject.h"

#include "Connectors/ForceConnector.h"
#include "NObjects/NoxelPart.h"

AMovementNObject::AMovementNObject()
{
	ForceIn = CreateDefaultSubobject<UForceConnector>("Forces");
	ForceIn->SetupAttachment(staticMesh, TEXT("ForceConnector"));
	ForceIn->bIsSender = false;
	AttachParent = staticMesh;
}

void AMovementNObject::BeginPlay()
{
	Super::BeginPlay();
}

void AMovementNObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FJsonObjectWrapper AMovementNObject::OnReadMetadata_Implementation(const TArray<AActor*>& Components)
{
	return Super::OnReadMetadata_Implementation(Components);
}

bool AMovementNObject::OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components)
{
	return Super::OnWriteMetadata_Implementation(Metadata, Components);
}

bool AMovementNObject::OnNObjectAttach_Implementation(ANoxelPart* Part)
{
	AttachParent = Part->GetNoxelContainer();
	return Super::OnNObjectAttach_Implementation(Part);
}
