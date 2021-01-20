//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "EditorGameState.h"
#include "NoxelHangarBase.h"

#include "EngineUtils.h"//Needed for iterator

ANoxelHangarBase * AEditorGameState::GetHangar()
{
	if (Hangar)
	{
		return Hangar;
	}
	else 
	{
		for (TActorIterator<ANoxelHangarBase> ActorItr(GetWorld()); ActorItr; ++ActorItr)
		{
			if (HangarSoft.Get() == *ActorItr)
			{
				Hangar = *ActorItr;
				return Hangar;
			}
		}
	}
	return nullptr;
};

UCraftDataHandler * AEditorGameState::GetCraft()
{
	return GetHangar()->GetCraftDataHandler();
}
