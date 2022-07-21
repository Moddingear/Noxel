//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovementNObject.h"
#include "Connectors/ForceConnector.h"
#include "Wheel.generated.h"

class UPhysicsConstraintComponent;
/**
 * 
 */
UCLASS()
class NOBJECTS_API AWheel : public AMovementNObject
{
	GENERATED_BODY()

public:
	AWheel();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UPhysicsConstraintComponent* PhysicsJoint;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* WheelMesh;
	
	UPROPERTY(BlueprintReadWrite)
	float ForwardFrictionCoefficient = 0.7;
	float MaxForce = 50000.f * 100.f;//cN

	float LateralFrictionCoefficient = 0.5;
	float MaxLateralFriction = 50000.f * 100.f; //cn
	

	UPROPERTY(BlueprintReadWrite)
	float MaxSpeed = 100.f*100.f;//cm/s

	float WheelRadius;
	float SuspensionLength = 100.f;//cm
	float MaxSuspensionWeight = 500;//kg
	float Damping = 0.4;//s

	TArray<FTorsor> GetMaxTorsor();

private:
	float PreviousExtension;
	float PreviousWheelSpeed;
	bool IsContactingGround(FHitResult& OutHit);
	float GetSuspensionForce(float NewDistanceToGround, float dt);
public:

	virtual void OnNObjectEnable_Implementation(UCraftDataHandler* Craft) override;
	virtual void OnNObjectDisable_Implementation() override;

	virtual FJsonObjectWrapper OnReadMetadata_Implementation(const TArray<AActor*>& Components) override;
	virtual bool OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components) override;

	//Force Connector Interface
	UFUNCTION()
	void OnGetReceived();
	UFUNCTION()
	void OnSetReceived();
};
