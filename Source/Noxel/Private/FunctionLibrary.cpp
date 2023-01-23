//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "FunctionLibrary.h"
#include "JsonObjectConverter.h"
#include "UObject/Object.h"
#include "NoxelDataAsset.h"
#include "Noxel.h"

const FString UFunctionLibrary::SaveDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
const FString UFunctionLibrary::ColorsFile = "Colors.json";
const TMap<ENoxelColor, FColor> UFunctionLibrary::DefaultColors = { 
	{ENoxelColor::NodeInactive, FColor(0.01*255, 0.85 * 255, 0.34 * 255, 0.75 * 255)}, //Node inactive
	{ENoxelColor::NodeFocus1, FColor(0.0 * 255, 0.0 * 255, 1.0 * 255, 0.75 * 255)}, //NodeFocus1
	{ENoxelColor::NodeFocus2, FColor(0.0 * 255, 0.38 * 255, 0.77 * 255, 0.75 * 255)}, //NodeFocus2
	{ENoxelColor::NodeSelected1, FColor(0.01 * 255, 0.0 * 255, 1.0 * 255, 0.75 * 255)}, //NodeSelected1
	{ENoxelColor::NodeSelected2, FColor(0.01 * 255, 0.0 * 255, 0.6 * 255, 0.75 * 255)}, //NodeSelected2
	{ENoxelColor::LineColor, FColor(1.0 * 255, 0.56 * 255, 0.0 * 255, 0.75 * 255)}, //LineColor
	{ENoxelColor::PlaneColor, FColor(0.01 * 255, 0.39 * 255, 0.32 * 255, 0.5 * 255)}, //PlaneColor
	{ENoxelColor::MoveColor1, FColor(0.0 * 255, 0.31 * 255, 1.0 * 255, 0.25 * 255)}, //MoveColor1
	{ENoxelColor::MoveColor2, FColor(0.0 * 255, 0.80 * 255, 1.0 * 255, 0.4 * 255)} //MoveColor2
};
TMap<ENoxelColor, FColor> UFunctionLibrary::JsonColorCache;

FString UFunctionLibrary::GetProjectVersion()
{
	FString ProjectVersion;
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GeneralProjectSettings"),
		TEXT("ProjectVersion"),
		ProjectVersion,
		GGameIni
	);
	return ProjectVersion;
}

bool UFunctionLibrary::isWithEditor()
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
}


FColor UFunctionLibrary::getColorFromJson(ENoxelColor color)
{
	//const FString colorname = UEnum::GetValueAsString(color); //Name of the enum color

	if (JsonColorCache.Num()==0)
	{
		FString AbsoluteFilePath = SaveDirectory + ColorsFile;
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (PlatformFile.CreateDirectoryTree(*SaveDirectory)) //Create or resolve folder
		{
			if (!PlatformFile.FileExists(*AbsoluteFilePath)) //If file doesn't exist
			{
				TArray<FNoxelSavedColor> Colors;
				for (auto KeyValuePair : DefaultColors)
				{
					
					FNoxelSavedColor Color(UEnum::GetValueAsString(KeyValuePair.Key), KeyValuePair.Value);
					Colors.Add(Color);
				}
				FNoxelSavedColorArray DefaultStruct(Colors);
				FString DefaultString;
				FJsonObjectConverter::UStructToJsonObjectString<FNoxelSavedColorArray>(DefaultStruct, DefaultString);
				FFileHelper::SaveStringToFile(DefaultString, *AbsoluteFilePath);
			}
		}
		FString SavedFile;
		FFileHelper::LoadFileToString(SavedFile, *AbsoluteFilePath);
		FNoxelSavedColorArray SavedColors;
		FJsonObjectConverter::JsonObjectStringToUStruct<FNoxelSavedColorArray>(SavedFile, &SavedColors, 0, 0);
		for (auto KeyValuePair : DefaultColors)
		{
			FString keytype = UEnum::GetValueAsString(KeyValuePair.Key);
			FColor col = KeyValuePair.Value;
			for (int i = 0; i < SavedColors.Colors.Num(); ++i)
			{
				if (SavedColors.Colors[i].Name.ToLower() == keytype.ToLower())
				{
					col = FColor::FromHex(SavedColors.Colors[i].HexColor);
				}
			}
			JsonColorCache.Add(KeyValuePair.Key, col);
		}
	}
	check(JsonColorCache.Contains(color));
	return JsonColorCache[color];
}

UActorComponent * UFunctionLibrary::AddActorComponent(AActor* owner, UClass * ActorComponentClass)
{
	UClass * baseClass = FindObject<UClass>(ANY_PACKAGE, TEXT("ActorComponent"));
	if (ActorComponentClass->IsChildOf(baseClass))
	{
		UActorComponent* NewComp = NewObject<UActorComponent>(owner, ActorComponentClass);
		if (!NewComp)
		{
			return NULL;
		}
		//~~~~~~~~~~~~~

		NewComp->RegisterComponent();        //You must ConstructObject with a valid Outer that has world, see above     

		return NewComp;
	}
	else
		return NULL;
}
