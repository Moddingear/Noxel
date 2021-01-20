//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "NoxelHangarBase.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnable))
class NOXEL_API ANoxelHangarBase : public AActor
{
	GENERATED_BODY()
	
public:	


	// Sets default values for this actor's properties
	ANoxelHangarBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* RootMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCraftDataHandler* Craft;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UVoxelComponent* Voxel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* CraftSpawnPoint;


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure)
	class UCraftDataHandler* GetCraftDataHandler() 
	{ 
		//UE_LOG(Noxel, Log, TEXT("[ANoxelHangarBase::GetCraftDataHandler] %s / %s : %d components"), *GetFullName(), *Craft->GetFullName(), Craft->Components.Num());
		return Craft; 
	}

	UFUNCTION(BlueprintPure)
	class UArrowComponent* getCraftSpawnPoint()
	{
		return CraftSpawnPoint;
	}

	UFUNCTION(BlueprintPure)
	class UVoxelComponent* GetVoxel() { return Voxel; }
};
