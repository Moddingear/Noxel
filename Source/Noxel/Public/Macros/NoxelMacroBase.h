//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "FunctionLibrary.h"
#include "Noxel/NoxelLibrary.h"
#include "Noxel/NoxelCombatLibrary.h"
#include "Noxel/NoxelDataStructs.h"
#include "EditorCommandQueue.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoxelMacroBase.generated.h"

#define ray_length 10000.0f

#define MACROS_NAMESPACE "NoxelMacros" 

DECLARE_LOG_CATEGORY_EXTERN(NoxelMacro, Log, All);

UENUM(BlueprintType)
enum class EAlternateType : uint8
{
	None,
	Hold,
	Switch
};

class AEditorCharacter;
class UNoxelNetworkingAgent;
class ANoxelPart;
class ANoxelHangarBase;
class UNodesContainer;
class UNoxelContainer;
class UVoxelComponent;

UCLASS(ClassGroup = "Noxel Macros", Blueprintable, meta=(BlueprintSpawnableComponent) )
class NOXEL_API ANoxelMacroBase : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ANoxelMacroBase();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(BlueprintReadOnly, BlueprintGetter = GetOwningActor, meta = (AllowPrivateAccess = "true"))
	AEditorCharacter* owningActor;

	TArray<FNodeID> ColoredNodes;
	//UPROPERTY(BlueprintReadOnly)
	//class UTransformGizmo* TransformGizmo;

public:	
	UPROPERTY(BlueprintReadOnly)
	EAlternateType AlternationMethod = EAlternateType::None;

	//Used to know if alt mode is active
	UPROPERTY(BlueprintReadOnly)
	bool Alternate = false;

	//Hint display as the left click on the side of the hangar
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText LeftClickHint;

	//Hint display as the right click on the side of the hangar
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText RightClickHint;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText MacroDescription;

	//Used for BP to retrieve hints
	UFUNCTION(BlueprintPure)
	void getHints(FText& LeftClick, FText& RightClick, FText& Description, EAlternateType& Alternation, bool& IsAlternate) 
	{
		LeftClick = LeftClickHint;
		RightClick = RightClickHint;
		Description = MacroDescription;
		Alternation = AlternationMethod;
		IsAlternate = Alternate;
	}; 

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure)
	AEditorCharacter* GetOwningActor() const;
	UFUNCTION(BlueprintCallable)
	void SetOwningActor(AEditorCharacter* InOwningActor);

	UFUNCTION(BlueprintPure)
	UNoxelNetworkingAgent* GetNoxelNetworkingAgent() const;

	UFUNCTION(BlueprintPure)
	ANoxelHangarBase* GetHangar() const;

	UFUNCTION(BlueprintPure)
	UCraftDataHandler* GetCraft() const;

	UFUNCTION(BlueprintPure)
	ANoxelPart* GetCurrentPart() const;

	
	////////////////////////////////////////////////////////////////
	//Get the ray emanating form the macro
	UFUNCTION(BlueprintPure)
	void getRay(FVector& Location, FVector& Direction);
	//Get the trace from the macro
	UFUNCTION(BlueprintPure)
	void getTrace(FVector& start, FVector& end);

	////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintCallable)
	FVector getNodePlacement(float PlacementDistance, UNodesContainer* Container);
	
	////////////////////////////////////////////////////////////////

	void setNodeColor(FNodeID id, ENoxelColor color);

	UFUNCTION(BlueprintCallable)
	void resetNodesColor();

	////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintPure)
	UVoxelComponent* GetVoxel();

	UFUNCTION(BlueprintPure)
	UNodesContainer* getSelectedNodesContainer();

	UFUNCTION(BlueprintPure)
	UNoxelContainer* getSelectedNoxelContainer();

	UFUNCTION(BlueprintCallable)
	void switchMacro(TSubclassOf<class ANoxelMacroBase> macro);

	////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintCallable)
	bool tracePart(FVector start, FVector end, ANoxelPart* & Part);

	bool traceNodes(FVector start, FVector end, FNodeID& id);

	UFUNCTION(BlueprintCallable)
	bool tracePanels(FVector start, FVector end, FPanelID& id);

	////////////////////////////////////////////////////////////////

	/*UFUNCTION(BlueprintCallable)
		void CreateTransformGizmo(FTransform position);

	UFUNCTION(BlueprintCallable)
		void DestroyTransformGizmo();*/

	////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintCallable)
	void DrawNode(FVector WorldPosition, float size = 10.0f);

	//UFUNCTION(BlueprintCallable)
	void DrawPanel(FPanelData PanelData);

	UFUNCTION(BlueprintCallable)
	void DrawLine(FVector Start, FVector End, float thickness = 10.0f);

	UFUNCTION(BlueprintCallable)
	void DrawPlane(FVector Position, FVector Normal);
	
	UFUNCTION(BlueprintCallable)
	int32 DrawPlaneMaterial(FTransform LocationWorld, UMaterialInterface* Material, bool bCreateComplexCollision = false);

	UFUNCTION(BlueprintCallable)
	void ClearDraws();

//private:
	//void MakeWireframe(FMeshData& mesh);


	////////////////////////////////////////////////////////////////

//public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void leftClickPressed();
	virtual void leftClickPressed_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void middleClickPressed();
	virtual void middleClickPressed_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void rightClickPressed();
	virtual void rightClickPressed_Implementation();


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void leftClickReleased();
	virtual void leftClickReleased_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void middleClickReleased();
	virtual void middleClickReleased_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void rightClickReleased();
	virtual void rightClickReleased_Implementation();

	////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void alternateModePressed();
	virtual void alternateModePressed_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void alternateModeReleased();
	virtual void alternateModeReleased_Implementation();

public:
	friend class AEditorCharacter;
};
