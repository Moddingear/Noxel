// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NObjects/BruteForceSolver.h"
#include "GameFramework/Actor.h"
#include "BruteForceTester.generated.h"

UCLASS(BlueprintType)
class NOXEL_API ABruteForceTester : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY()
	UBruteForceSolver* Solver;

	TArray<FForceSource> Sources;
	TArray<bool> PrintedResults;
	// Sets default values for this actor's properties
	ABruteForceTester();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
