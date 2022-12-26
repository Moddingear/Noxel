//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FunctionLibrary.generated.h"

/*
 *
 * 
 */

UENUM(BlueprintType)
enum class ENoxelColor : uint8
{
	NodeInactive 	UMETA(DisplayName = "Inactive node"),
	NodeFocus1 		UMETA(DisplayName = "Focused node 1"),
	NodeFocus2		UMETA(DisplayName = "Focused node 2"),
	NodeSelected1	UMETA(DisplayName = "Selected node 1"),
	NodeSelected2	UMETA(DisplayName = "Selected node 2"),
	LineColor		UMETA(DisplayName = "Line color"),
	PlaneColor		UMETA(DisplayName = "Plane color"),
	MoveColor1		UMETA(DisplayName = "Move color"),
	MoveColor2		UMETA(DisplayName = "Move color (arrows)")
};

USTRUCT()
struct FNoxelSavedColor
{
	GENERATED_BODY()

	UPROPERTY()
		FString Name;
	UPROPERTY()
		FString HexColor;

	FNoxelSavedColor()
		:Name(TEXT("NO-NAME")),
		HexColor(TEXT("00000000"))
	{}

	FNoxelSavedColor(const FString InName, const FString InHexColor)
		:Name(InName),
		HexColor(InHexColor)
	{}

	FNoxelSavedColor(const FString InName, const FColor InHexColor)
		:Name(InName),
		HexColor(InHexColor.ToHex())
	{}
};

USTRUCT()
struct FNoxelSavedColorArray
{
	GENERATED_BODY()

		UPROPERTY()
		TArray<FNoxelSavedColor> Colors;

	FNoxelSavedColorArray()
		:Colors()
	{}

	FNoxelSavedColorArray(const TArray<FNoxelSavedColor>& InColors)
		:Colors(InColors)
	{}
};

UCLASS()
class NOXEL_API UFunctionLibrary: public UBlueprintFunctionLibrary
{

private:
	static const FString SaveDirectory;
	static const FString ColorsFile;
	static const TMap<ENoxelColor, FColor> DefaultColors;

	
	static TMap<ENoxelColor, FColor> JsonColorCache;

public:
	GENERATED_BODY()

	UFUNCTION(BlueprintPure, Category = "Project")
		static FString GetProjectVersion();

	UFUNCTION(BlueprintPure, Category = "Project")
		static bool isWithEditor();

	UFUNCTION(BlueprintPure, Category = "Math|Date Time")
	static void GetUTCFromUnixTimeStamp(int32 UnixTimeStamp, FDateTime& UTCTime)
	{
		//Please note it should really be int64 but that is not supported by BP yet
		UTCTime = FDateTime::FromUnixTimestamp(UnixTimeStamp);
	}

	UFUNCTION(BlueprintPure, Category = "Math|Date Time")
	static int32 GetUnixTimeStamp(const FDateTime& UTCTime)
	{
		//Please note it should really be int64 but that is not supported by BP yet
		return UTCTime.ToUnixTimestamp();
	}

	template<typename TEnum>
	static FORCEINLINE FString GetEnumValueAsString(const FString& Name, TEnum Value)
	{
		const UEnum* enumPtr = FindObject<UEnum>(ANY_PACKAGE, *Name, true);
		if (!enumPtr)
		{
			return FString("Invalid");
		}
		return enumPtr->GetNameByValue((int64)Value).ToString();
	}

	UFUNCTION(BlueprintCallable)
		static FColor getColorFromJson(ENoxelColor color);

	UFUNCTION(BlueprintCallable, Category = "Component|AddComponent")
		static UActorComponent* AddActorComponent(AActor* owner, UClass* ActorComponentClass);


	//https://wiki.unrealengine.com/Dynamic_Load_Object
	UFUNCTION(BlueprintPure)
	static FORCEINLINE FName GetObjPath(const UObject* Obj) { 
		if(!Obj)
		{
			return NAME_None;
		}
		FStringAssetReference ThePath = FStringAssetReference(Obj);
		if(!ThePath.IsValid()) return NAME_None;
		//The Class FString Name For This Object 
		FString Str = Obj->GetClass()->GetDescription();
		Str += "'"; Str += ThePath.ToString(); Str += "'";
		return FName(*Str); 
	}
	//Player Editor'/Game/Levels/UEDPIE_0_Editor.Editor:PersistentLevel.PlayerEditor_C_0'
	//Blueprint'/Game/_Blueprints/PlayerEditor/PlayerEditor.PlayerEditor'

	UFUNCTION(BlueprintPure)
	static FORCEINLINE FName GetClassPath(const UClass* Class) { 
		if(!Class)
		{
			return NAME_None;
		}
		FStringAssetReference ThePath = FStringAssetReference(Class);
		if(!ThePath.IsValid()) return NAME_None;
		//The Class FString Name For This object 
		FString Str = Class->GetClass()->GetDescription();
		Str += "'"; Str += ThePath.ToString(); Str += "'";
		return FName(*Str); 
	}
	//Blueprint Generated Class'/Game/_Blueprints/PlayerEditor/PlayerEditor.PlayerEditor_C'
	

	template <typename ObjClass> 
	static FORCEINLINE ObjClass* LoadObjFromPath(const FName& Path) { 
		if(Path == NAME_None) return NULL; //~
    	return Cast<ObjClass>(StaticLoadObject( ObjClass::StaticClass(), NULL,*Path.ToString()));
	}

	UFUNCTION(BlueprintCallable)
	static FORCEINLINE UClass* LoadClassFromPath(const FName& Path){
		return LoadObjFromPath<UClass>(Path);
	}

	/*UFUNCTION(BlueprintCallable)
	static FORCEINLINE UNoxelDataAsset* GetNoxelDataAsset(){
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(TEXT("Blueprint'/Game/_Blueprints/NObjects/NoxelDataAsset.NoxelDataAsset'"));
		return Cast<UNoxelDataAsset>(AssetData.GetAsset())
		//return FindObject<UNoxelDataAsset>(TestsOuter, *TestName);
		/*return Cast<UNoxelDataAsset>(
			TSoftObjectPtr<UNoxelDataAsset>(
				FSoftObjectPath(
					TEXT("Blueprint'/Game/_Blueprints/NObjects/NoxelDataAsset.NoxelDataAsset'")
				)
			).LoadSynchronous()
		);
		//return LoadObjFromPath<UNoxelDataAsset>(TEXT("Blueprint'/Game/_Blueprints/NObjects/NoxelDataAsset.NoxelDataAsset'"));
	}*/
};
