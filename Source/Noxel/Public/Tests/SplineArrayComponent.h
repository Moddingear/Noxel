// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshComponent.h"
#include "SplineProvider.h"
#include "SplineArrayComponent.generated.h"
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, Meta = (BlueprintSpawnableComponent))
class NOXEL_API USplineArrayComponent : public URuntimeMeshComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	USplineProvider* provider;
	
public:
	USplineArrayComponent();
	~USplineArrayComponent();

	virtual void OnRegister() override;
};
