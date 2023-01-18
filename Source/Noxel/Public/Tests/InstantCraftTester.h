#pragma once

#include "CoreMinimal.h"
#include "Actor.h"
#include "InstantCraftTester.generated.h"

UCLASS(BlueprintType)
class NOXEL_API AInstantCraftTester : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CraftPath;

	UPROPERTY(Transient)
	bool HasLoaded;

	UPROPERTY(EditAnywhere)
	bool Enable;
	
	AInstantCraftTester();
	
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;
};
