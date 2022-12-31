//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Noxel/NodesContainer.h"
#include "NObjectInterface.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
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
	UPROPERTY(Replicated)
	bool Enabled;
private:
	bool EnabledPrev = false, AttachedPrev = false;
public:

	UPROPERTY(Replicated)
	UCraftDataHandler* ParentCraft;
	
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
	virtual void OnNObjectEnable_Implementation(UCraftDataHandler* Craft) override;

	virtual void OnNObjectDisable_Implementation() override;

	virtual bool OnNObjectAttach_Implementation(ANoxelPart* Part) override;

	virtual FJsonObjectWrapper OnReadMetadata_Implementation(const TArray<AActor*>& Components) override;

	virtual bool OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components) override;
	//NObject interface end

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	float pitch; //Y
	float yaw; //Z
	float pitchInput;
	float yawInput;
	FVector TranslationInputs, RotationInputs;
	
	void MoveX(float input);
	void MoveY(float input);
	void MoveZ(float input);

	void LookPitch(float input);

	void LookYaw(float input);
};
