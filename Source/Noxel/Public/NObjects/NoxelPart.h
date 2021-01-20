//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoxelPart.generated.h"

class UNoxelContainer;
class UNodesContainer;

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnable))
class NOXEL_API ANoxelPart : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANoxelPart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, BlueprintGetter = GetNoxelContainer)
	UNoxelContainer* noxelContainer;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, BlueprintGetter = GetNodesContainer)
	UNodesContainer* nodesContainer;

public:
	UFUNCTION(BlueprintPure)
	UNoxelContainer* GetNoxelContainer();

	UFUNCTION(BlueprintPure)
	UNodesContainer* GetNodesContainer();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	
};
