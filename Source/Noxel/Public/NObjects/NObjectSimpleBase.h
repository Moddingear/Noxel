//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Noxel/NodesContainer.h"
#include "NObjectInterface.h"
#include "NObjectSimpleBase.generated.h"

UCLASS()
class NOXEL_API ANObjectSimpleBase : public AActor, public INObjectInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANObjectSimpleBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(Replicated)
	bool Enabled = false;
private:
	bool EnabledPrev = false, AttachedPrev = false;
public:
	UPROPERTY(Replicated)
	UCraftDataHandler* ParentCraft = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* staticMesh = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UNodesContainer* nodesContainer = nullptr;

	virtual void OnNObjectEnable_Implementation(UCraftDataHandler* Craft) override;

	virtual void OnNObjectDisable_Implementation() override;

	virtual bool OnNObjectAttach_Implementation(ANoxelPart* Part) override;

	virtual FJsonObjectWrapper OnReadMetadata_Implementation(const TArray<AActor*>& Components) override;

	virtual bool OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components) override;
	//NObject Interface End

	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
