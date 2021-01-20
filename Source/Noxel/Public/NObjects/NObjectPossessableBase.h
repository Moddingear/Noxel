//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Noxel/NodesContainer.h"
#include "NObjectInterface.h"
#include "Components/StaticMeshComponent.h"
#include "NObjectPossessableBase.generated.h"

UCLASS()
class NOXEL_API ANObjectPossessableBase : public APawn, public INObjectInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANObjectPossessableBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* staticMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UNodesContainer* nodesContainer;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UArrowComponent* CameraRotationPoint;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UCameraComponent* Camera;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OrbitDistance;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//NObject Interface Start
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		void OnNObjectEnable(UCraftDataHandler* Craft);
	virtual void OnNObjectEnable_Implementation(UCraftDataHandler* Craft) /*override*/;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		void OnNObjectDisable();
	virtual void OnNObjectDisable_Implementation() /*override*/;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		bool OnNObjectAttach(ANoxelPart* Part);
	virtual bool OnNObjectAttach_Implementation(ANoxelPart* Part) /*override*/;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		FString OnReadMetadata();
	virtual FString OnReadMetadata_Implementation() /*override*/;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		bool OnWriteMetadata(const FString& Metadata);
	virtual bool OnWriteMetadata_Implementation(const FString& Metadata) /*override*/;
	//NObject Interface End

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	float pitch; //Y
	float yaw; //Z
	float pitchInput;
	float yawInput;

	void LookPitch(float input);

	void LookYaw(float input);
};
