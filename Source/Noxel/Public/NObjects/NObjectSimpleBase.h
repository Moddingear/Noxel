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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* staticMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UNodesContainer* nodesContainer;

	//NObject Interface Start
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void OnNObjectEnable(UCraftDataHandler* Craft);
	virtual void OnNObjectEnable_Implementation(UCraftDataHandler* Craft) /*override*/;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void OnNObjectDisable();
	virtual void OnNObjectDisable_Implementation() /*override*/;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		bool OnNObjectAttach(ANoxelPart* Part);
	virtual bool OnNObjectAttach_Implementation(ANoxelPart* Part) /*override*/;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		FJsonObjectWrapper OnReadMetadata(const TArray<AActor*>& Components);
	virtual FJsonObjectWrapper OnReadMetadata_Implementation(const TArray<AActor*>& Components) /*override*/;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		bool OnWriteMetadata(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components);
	virtual bool OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components) /*override*/;
	//NObject Interface End

	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
