// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "MovementNObject.h"

#include "Connectors/ForceConnector.h"

AMovementNObject::AMovementNObject()
{
	ForceIn = CreateDefaultSubobject<UForceConnector>("Forces");
	ForceIn->SetupAttachment(staticMesh, TEXT("ForceConnector"));
	ForceIn->bIsSender = false;
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
