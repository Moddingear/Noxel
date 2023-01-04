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

private:
	struct WheelFrictionData
	{
		uint64_t frameNumber;
		bool IsOnGround;
		float DistanceToGround;
		float SuspensionForce;
		FHitResult GroundCast;
	};
	
	WheelFrictionData PreviousFrameFriction = {0};
	WheelFrictionData ThisFrameFriction = {0};
	
public:
	AWheel();

protected:
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* WheelMesh;
	
	UPROPERTY(BlueprintReadWrite)
	float ForwardFrictionCoefficient = 1.5f;
	UPROPERTY(BlueprintReadWrite)
	float MaxForce = 50000.f * 100.f;//cN

	UPROPERTY(BlueprintReadWrite)
	float LateralFrictionCoefficient = 0.5f;
	UPROPERTY(BlueprintReadWrite)
	float MaxLateralFriction = 50000.f * 100.f; //cn
	

	UPROPERTY(BlueprintReadWrite)
	float MaxSpeed = 100.f*100.f;//cm/s

	float WheelRadius; //cm
	float SuspensionLength = 100.f;//cm
	float MaxSuspensionWeight = 500;//kg
	float Damping = 0.4;//s

	TArray<FTorsor> GetMaxTorsor();

private:
	float PreviousExtension = 0;
	float PreviousWheelSpeed = 0;
	bool IsContactingGround(FHitResult& OutHit);
	float GetSuspensionForce(float NewDistanceToGround, float dt);

	static AWheel* debugactor;
	WheelFrictionData GetFrictionData();
	WheelFrictionData GetPreviousFrictionData();
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
