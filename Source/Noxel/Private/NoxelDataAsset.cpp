//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "NoxelDataAsset.h"

bool UNoxelDataAsset::HasClass(TSubclassOf<AActor> CompClass)
{
	return getComponentIDFromClass(CompClass) != "";
}

FString UNoxelDataAsset::getComponentIDFromClass(TSubclassOf<AActor> CompClass)
{
    for (int i=0; i<Objects.Num(); i++){
        if(Objects[i].Class.Get() == CompClass){
            return Objects[i].ComponentID;
        }
    }
    return "";
}

TSubclassOf<AActor> UNoxelDataAsset::getClassFromComponentID(FString CompID)
{
    for (int i=0; i<Objects.Num(); i++){
        if(Objects[i].ComponentID == CompID){
            return Objects[i].Class.Get();
        }
    }
    return nullptr;
}

bool UNoxelDataAsset::HasClass(UDataTable * Object, TSubclassOf<AActor> CompClass)
{
	if (!Object)
	{
		return false;
	}
	TArray<FNoxelObjectData*> Rows;
	Object->GetAllRows<FNoxelObjectData>(FString("UNoxelDataAsset::HasClass"), Rows);
	for (FNoxelObjectData* Row : Rows)
	{
		if (Row->Class.Get() == CompClass)
		{
			return true;
		}
	}
	return false;
}

FString UNoxelDataAsset::getComponentIDFromClass(UDataTable * Object, TSubclassOf<AActor> CompClass)
{
	if (!Object)
	{
		return FString();
	}
	TArray<FNoxelObjectData*> Rows;
	Object->GetAllRows<FNoxelObjectData>(FString("UNoxelDataAsset::getComponentIDFromClass"), Rows);
	for (FNoxelObjectData* Row : Rows)
	{
		if (Row->Class.Get() == CompClass)
		{
			return Row->ComponentID;
		}
	}
	return FString();
}

TSubclassOf<AActor> UNoxelDataAsset::getClassFromComponentID(UDataTable * Object, FString CompID)
{
	if (!Object)
	{
		return NULL;
	}
	TArray<FNoxelObjectData*> Rows;
	Object->GetAllRows<FNoxelObjectData>(FString("UNoxelDataAsset::getClassFromComponentID"), Rows);
	for (FNoxelObjectData* Row : Rows)
	{
		if (Row->ComponentID.ToLower() == CompID.ToLower())
		{
			return Row->Class.LoadSynchronous(); //TODO : Load elsewhere
		}
	}
	return NULL;
}


