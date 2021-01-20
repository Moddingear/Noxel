//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "NoxelHangarBase.h"

#include "Noxel.h"

#include "Noxel/CraftDataHandler.h"
#include "Voxel/VoxelComponent.h"
#include "Components/ArrowComponent.h"
#include "EditorGameState.h"

// Sets default values
ANoxelHangarBase::ANoxelHangarBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bNetLoadOnClient = true;

	RootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = (USceneComponent*) RootMesh;
	Craft = CreateDefaultSubobject<UCraftDataHandler>(TEXT("CraftDataHandler"));
	Voxel = CreateDefaultSubobject<UVoxelComponent>(TEXT("Voxel"));
	Voxel->SetupAttachment(RootComponent);
	CraftSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Point"));
	CraftSpawnPoint->SetupAttachment(RootComponent);
	CraftSpawnPoint->SetRelativeLocation(FVector(3000, 0, 0));

	Craft->SpawnContext = ECraftSpawnContext::Editor;
}

// Called when the game starts or when spawned
void ANoxelHangarBase::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(Noxel, Log, TEXT("[ANoxelHangarBase::BeginPlay] Loading default craft"));
	Craft->SpawnContext = ECraftSpawnContext::Editor;
	Craft->loadCraft(Craft->GetDefaultCraftSave(), FTransform(FVector(0,0,100)));
	UE_LOG(Noxel, Log, TEXT("[ANoxelHangarBase::BeginPlay] %s : %d components"), *Craft->GetFullName(), Craft->Components.Num());

	AGameStateBase* gs = GetWorld()->GetGameState();
	if (gs)
	{
		AEditorGameState* egs = (AEditorGameState*)gs;
		if (egs)
		{
			egs->Hangar = this;
		}
	}
}

// Called every frame
void ANoxelHangarBase::Tick(float DeltaTime)
{

	Super::Tick(DeltaTime);
	//UE_LOG(Noxel, Log, TEXT("[ANoxelHangarBase::Tick] %s : %d components"), *Craft->GetFullName(), Craft->Components.Num());
}

