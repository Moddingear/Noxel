//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Noxel/NoxelDataStructs.h"
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
	UPROPERTY(ReplicatedUsing=OnRep_NoxelSave)
	FNoxelNetwork NoxelSave;
	
	UFUNCTION(BlueprintPure)
	UNoxelContainer* GetNoxelContainer();

	UFUNCTION(BlueprintPure)
	UNodesContainer* GetNodesContainer();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnRep_NoxelSave();

	UFUNCTION()
	void OnNoxelHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit );
	
};
