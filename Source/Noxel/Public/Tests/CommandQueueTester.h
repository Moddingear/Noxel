// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NObjects/NoxelPart.h"
#include "CommandQueueTester.generated.h"

UCLASS()
class NOXEL_API ACommandQueueTester : public ANoxelPart
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACommandQueueTester();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
