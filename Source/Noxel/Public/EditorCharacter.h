//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "EditorCharacter.generated.h"

class UNoxelNetworkingAgent;
class ANoxelHangarBase;
class UCraftDataHandler;
class ANoxelPart;
class UCameraComponent;
class ANoxelMacroBase;
class UWidgetInteractionComponent;
class UUserWidget;

UCLASS(BlueprintType, Blueprintable, config = Game, meta = (BlueprintSpawnable))
class NOXEL_API AEditorCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEditorCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:	
	
	UPROPERTY(BlueprintReadOnly, BlueprintGetter = GetNetworkingAgent, meta = (AllowPrivateAccess = "true"))
	UNoxelNetworkingAgent* NetworkingAgent;

	UPROPERTY(BlueprintReadOnly, BlueprintGetter = GetHangar, meta = (AllowPrivateAccess = "true"))
	ANoxelHangarBase* Hangar;
	UPROPERTY(BlueprintReadWrite, BlueprintGetter = GetCurrentPart, BlueprintSetter = SetCurrentPart, meta = (AllowPrivateAccess = "true"))
	ANoxelPart* CurrentPart;
	UPROPERTY(BlueprintReadOnly, BlueprintGetter = GetCurrentMacro, meta = (AllowPrivateAccess = "true"))
	ANoxelMacroBase* CurrentMacro;
	UPROPERTY(BlueprintReadOnly, BlueprintGetter = GetInteractionWidget, meta = (AllowPrivateAccess = "true"))
	UWidgetInteractionComponent* WidgetInteraction;

protected:
	UPROPERTY(BlueprintReadOnly)
	UCameraComponent* Camera;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<class UUserWidget> wUserUI;
	UPROPERTY(BlueprintReadOnly)
	UUserWidget* UserUI;

	UPROPERTY(BlueprintReadOnly, GlobalConfig)
	float LookSensitivity;

private:
	bool bPointerPressed = false;

public:
	UFUNCTION(BlueprintPure)
	UNoxelNetworkingAgent* GetNetworkingAgent();
	UFUNCTION(BlueprintPure)
	ANoxelHangarBase* GetHangar();
	UFUNCTION(BlueprintPure)
	UCraftDataHandler* GetCraft();
	UFUNCTION(BlueprintPure)
	ANoxelPart* GetCurrentPart();
	UFUNCTION(BlueprintCallable)
	void SetCurrentPart(ANoxelPart* InCurrentPart);
	UFUNCTION(BlueprintPure)
	ANoxelMacroBase* GetCurrentMacro();
	UFUNCTION(BlueprintPure)
	UWidgetInteractionComponent* GetInteractionWidget();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void ReceivePossessed_Implementation(AController* NewController);

	UFUNCTION(Client, Reliable)
	void Possessed();
	virtual void Possessed_Implementation();

	UFUNCTION()
	void CraftLoaded();

	UFUNCTION(BlueprintCallable)
	void SetMacro(TSubclassOf<class ANoxelMacroBase> Macro);

	void MoveX(float input);

	void MoveY(float input);

	void MoveZ(float input);

	UFUNCTION(BlueprintCallable)
	void SetSensitivity(float NewValue);

	void LookPitch(float input);

	void LookYaw(float input);

	void LeftClickPressed();
	void LeftClickReleased();

	void MiddleClickPressed();
	void MiddleClickReleased();

	void RightClickPressed();
	void RightClickReleased();

	void AlternateModePressed();
	void AlternateModeReleased();

};
