//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once


#include "NoxelDataStructs.h" //Needed for SpawnContext

#include "CoreMinimal.h"
#include "RuntimeMeshComponent.h"
#include "NoxelDataComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = "Noxel")
class NOXEL_API UNoxelDataComponent : public URuntimeMeshComponent
{
	GENERATED_BODY()
	
public:

	UNoxelDataComponent(const FObjectInitializer& ObjectInitializer);

protected:

	UPROPERTY(Replicated)
	ECraftSpawnContext SpawnContext;

public:
	/*
	Sets the spawn context for the components to react accordingly (for example, the nodes won't create a mesh in battle)
	*/
	virtual void SetSpawnContext(ECraftSpawnContext Context);
};
