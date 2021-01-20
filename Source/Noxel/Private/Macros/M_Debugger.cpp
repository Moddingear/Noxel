//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_Debugger.h"
#include "Noxel/CraftDataHandler.h"

AM_Debugger::AM_Debugger() 
{

}

void AM_Debugger::BeginPlay()
{
	Super::BeginPlay();
}

void AM_Debugger::leftClickPressed_Implementation()
{
	FVector start, end;
	getTrace(start, end);
	ANoxelPart* part;
	if (tracePart(start, end, part)) {
		FNodesContainerSave save;
		FNoxelContainerSave nsave;
		TMap<FNodeID, FNodeSavedRedirector> redirmap;
		UCraftDataHandler::noxelNetworkToText(UCraftDataHandler::saveNoxelNetwork(getSelectedNoxelContainer()));
	}
}

void AM_Debugger::middleClickPressed_Implementation()
{
}

void AM_Debugger::rightClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Saving craft..."));
	FCraftSave saved = GetCraft()->saveCraft();
	UCraftDataHandler::craftSaveToText(saved);
	GetCraft()->loadCraft(saved, FTransform(FVector(0,0,100)));
	UE_LOG(NoxelMacro, Log, TEXT("Load successful"));
}
