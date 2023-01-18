#include "Tests/InstantCraftTester.h"

#include "EditorCharacter.h"
#include "Noxel.h"
#include "NoxelPlayerController.h"
#include "NoxelHangarBase.h"
#include "Noxel/CraftDataHandler.h"
#include "Noxel/NoxelNetworkingAgent.h"

AInstantCraftTester::AInstantCraftTester()
{
	PrimaryActorTick.bCanEverTick = true;
	HasLoaded = false;
	Enable = true;
}

void AInstantCraftTester::BeginPlay()
{
	Super::BeginPlay();
}

void AInstantCraftTester::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if WITH_EDITOR
	if (HasLoaded || !Enable)
	{
		return;
	}
	if (!GetWorld()->IsServer())
	{
		HasLoaded = true;
		return;
	}
	ANoxelPlayerController* fc = CastChecked<ANoxelPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!IsValid(fc))
	{
		return;
	}
	AEditorCharacter* charac = CastChecked<AEditorCharacter>(fc->GetCharacter());
	if (!IsValid(charac))
	{
		return;
	}
	ANoxelHangarBase* hangar = charac->GetHangar();
	if (!IsValid(hangar))
	{
		return;
	}
	UCraftDataHandler* dh = hangar->GetCraftDataHandler();
	if (!IsValid(dh))
	{
		return;
	}
	UNoxelNetworkingAgent* nna = charac->GetNetworkingAgent();
	if (!IsValid(nna))
	{
		return;
	}
	FCraftSave savedcraft;
	if (!UCraftDataHandler::getCraftSave(UCraftDataHandler::getCraftSaveLocation() + CraftPath, savedcraft))
	{
		UE_LOG(Noxel, Warning, TEXT("[AInstantCraftTester::Tick] Failed to load craft %s"), *CraftPath);
		HasLoaded = true;
		return;
	}
	dh->loadCraft(savedcraft, FTransform::Identity);
	nna->SpawnAndPossessCraft();
	HasLoaded = true;
#endif
}
