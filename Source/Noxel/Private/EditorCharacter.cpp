//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "EditorCharacter.h"
#include "Runtime/UMG/Public/UMG.h"

#include "NoxelPlayerController.h"
#include "Noxel/NoxelNetworkingAgent.h"
#include "EditorGameState.h"

#include "NoxelHangarBase.h"

#include "Components/WidgetInteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Macros/NoxelMacroBase.h"
#include "Macros/M_Nodes.h"

#include "Noxel.h"


// Sets default values
AEditorCharacter::AEditorCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	LookSensitivity = 1.0f;

	NetworkingAgent = CreateDefaultSubobject<UNoxelNetworkingAgent>(TEXT("Networking Agent"));

	UCapsuleComponent* RootCapsule = GetCapsuleComponent();
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootCapsule);
	WidgetInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("Widget Interaction"));
	WidgetInteraction->SetupAttachment(Camera);

	RootCapsule->SetCapsuleRadius(20.f, false);
	RootCapsule->SetCapsuleHalfHeight(20.f, false);
	Camera->bUsePawnControlRotation = true;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->DefaultLandMovementMode = EMovementMode::MOVE_Flying;
	Movement->DefaultWaterMovementMode = EMovementMode::MOVE_Flying;
	Movement->MaxFlySpeed = 200.f;
	Movement->BrakingDecelerationFlying = Movement->MaxAcceleration * 8;
	WidgetInteraction->InteractionDistance = 20000.f;

	static ConstructorHelpers::FClassFinder<UUserWidget> hudWidgetObj(TEXT("/Game/NoxelEditor/PlayerEditor/EditorUI"));
	if (hudWidgetObj.Succeeded())
	{
		wUserUI = hudWidgetObj.Class;
	}

}

// Called when the game starts or when spawned
void AEditorCharacter::BeginPlay()
{
	Super::BeginPlay();
	//Reduce the size of the capsule to avoid the player getting stuck between other players at spawn
	UCapsuleComponent* RootCapsule = GetCapsuleComponent();
	RootCapsule->SetCapsuleRadius(10.f, false);
	RootCapsule->SetCapsuleHalfHeight(10.f, true);

	if (IsLocallyControlled())
	{
		Possessed();
	}
}

UNoxelNetworkingAgent * AEditorCharacter::GetNetworkingAgent()
{
	return NetworkingAgent;
}

ANoxelHangarBase * AEditorCharacter::GetHangar()
{
	return Hangar;
}

UCraftDataHandler * AEditorCharacter::GetCraft()
{
	ANoxelHangarBase* tempHangar = GetHangar();
	if (tempHangar)
	{
		return tempHangar->GetCraftDataHandler();
	}
	return nullptr;
}

ANoxelPart * AEditorCharacter::GetCurrentPart()
{
	return CurrentPart;
}

void AEditorCharacter::SetCurrentPart(ANoxelPart * InCurrentPart)
{
	CurrentPart = InCurrentPart;
}

ANoxelMacroBase * AEditorCharacter::GetCurrentMacro()
{
	return CurrentMacro;
}

UWidgetInteractionComponent * AEditorCharacter::GetInteractionWidget()
{
	return WidgetInteraction;
}

// Called every frame
void AEditorCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*if (GetWorld()->GetTimeSeconds() > 1 && GetWorld()->IsServer())
	{
		ReplicatedQueueOrder = &FEditorQueueOrderAddNode();
	}*/
}

// Called to bind functionality to input
void AEditorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	//Action mappings ----------------------------------------------------------------
	InputComponent->BindAction("Main mouse button", IE_Pressed, this, &AEditorCharacter::LeftClickPressed);
	InputComponent->BindAction("Main mouse button", IE_Released, this, &AEditorCharacter::LeftClickReleased);

	InputComponent->BindAction("Secondary mouse button", IE_Pressed, this, &AEditorCharacter::RightClickPressed);
	InputComponent->BindAction("Secondary mouse button", IE_Released, this, &AEditorCharacter::RightClickReleased);

	InputComponent->BindAction("Ternary mouse button", IE_Pressed, this, &AEditorCharacter::MiddleClickPressed);
	InputComponent->BindAction("Ternary mouse button", IE_Released, this, &AEditorCharacter::MiddleClickReleased);

	InputComponent->BindAction("Alternate Mode", IE_Pressed, this, &AEditorCharacter::AlternateModePressed);
	InputComponent->BindAction("Alternate Mode", IE_Released, this, &AEditorCharacter::AlternateModeReleased);

	//Axis mappings ----------------------------------------------------------------

	InputComponent->BindAxis("Forwards/Backwards", this, &AEditorCharacter::MoveX);
	InputComponent->BindAxis("Left/Right", this, &AEditorCharacter::MoveY);
	InputComponent->BindAxis("Up/Down", this, &AEditorCharacter::MoveZ);
	InputComponent->BindAxis("Mouse X Axis", this, &AEditorCharacter::LookYaw);
	InputComponent->BindAxis("Mouse Y Axis", this, &AEditorCharacter::LookPitch);
}

void AEditorCharacter::ReceivePossessed_Implementation(AController * NewController)
{
	Possessed(); //Pass possession onto the client
}

void AEditorCharacter::Possessed_Implementation()
{
	UE_LOG(Noxel, Log, TEXT("Possessed"));
	if (wUserUI)
	{
		UserUI = CreateWidget<UUserWidget>((APlayerController*)GetController(), wUserUI);
		UserUI->AddToViewport();
	}

	if (NetworkingAgent->Craft)
	{
		Hangar = (ANoxelHangarBase*)NetworkingAgent->Craft->GetOwner();
		TArray<ANoxelPart*> Parts = GetCraft()->GetParts();
		GetCraft()->OnCraftLoadedEvent.AddDynamic(this, &AEditorCharacter::CraftLoaded);
		UE_LOG(Noxel, Log, TEXT("[AEditorCharacter::BeginPlay] OnCraftLoaded bound"));
		if (Parts.Num() > 0)
		{
			CurrentPart = Parts[0];
			SetMacro(AM_Nodes::StaticClass());
		}
		else
		{
			UE_LOG(Noxel, Warning, TEXT("[AEditorCharacter::BeginPlay] No part found in craft"));
		}
	}
}

void AEditorCharacter::CraftLoaded()
{
	UE_LOG(Noxel, Log, TEXT("[AEditorCharacter::CraftLoaded]"));
	TArray<ANoxelPart*> Parts = GetCraft()->GetParts();
	if (Parts.Num() > 0)
	{
		CurrentPart = Parts[0];
		SetMacro(AM_Nodes::StaticClass());
	}
	else
	{
		UE_LOG(Noxel, Log, TEXT("[AEditorCharacter::CraftLoaded] No part found"));
	}
}

void AEditorCharacter::SetMacro(TSubclassOf<class ANoxelMacroBase> Macro)
{
	if (IsLocallyControlled())
	{
		if (CurrentMacro)
		{
			CurrentMacro->Destroy();
		}
		CurrentMacro = GetWorld()->SpawnActorDeferred<ANoxelMacroBase>(Macro.Get(), GetHangar()->GetActorTransform(), this, this);
		if (!CurrentMacro)
		{
			UE_LOG(Noxel, Warning, TEXT("[AEditorCharacter::SetMacro] New macro is invalid, aborting")); 
			return;
		}
		CurrentMacro->SetOwningActor(this);
		UGameplayStatics::FinishSpawningActor(CurrentMacro, GetHangar()->GetActorTransform());
		CurrentMacro->AddTickPrerequisiteActor(this);
	}
}

void AEditorCharacter::MoveX(float input)
{
	//right^z = forward
	AddMovementInput(Camera->GetRightVector() ^ FVector(0.f, 0.f, 1.f), input, false);
}

void AEditorCharacter::MoveY(float input)
{
	AddMovementInput(Camera->GetRightVector(), input, false);
}

void AEditorCharacter::MoveZ(float input)
{
	AddMovementInput({ 0.f,0.f,1.f }, input, false);
}

void AEditorCharacter::SetSensitivity(float NewValue)
{
	LookSensitivity = NewValue;
	SaveConfig();
}

void AEditorCharacter::LookPitch(float input)
{
	AddControllerPitchInput(input * LookSensitivity);
}

void AEditorCharacter::LookYaw(float input)
{
	AddControllerYawInput(input * LookSensitivity);
}

void AEditorCharacter::LeftClickPressed()
{
	if (WidgetInteraction->IsOverInteractableWidget())
	{
		WidgetInteraction->PressPointerKey(FKey("LeftMouseButton"));
		//UE_LOG(Noxel, Log, TEXT("Pointer Press"));
		bPointerPressed = true;
	}
	else if (CurrentMacro)
	{
		CurrentMacro->leftClickPressed();
	}
}

void AEditorCharacter::LeftClickReleased()
{
	if (bPointerPressed)
	{
		WidgetInteraction->ReleasePointerKey(FKey("LeftMouseButton"));
		//UE_LOG(Noxel, Log, TEXT("Pointer Release"));
		bPointerPressed = false;
	}
	else if (CurrentMacro)
	{
		CurrentMacro->leftClickReleased();
	}
}

void AEditorCharacter::MiddleClickPressed()
{
	if (CurrentMacro)
	{
		CurrentMacro->middleClickPressed();
	}
}

void AEditorCharacter::MiddleClickReleased()
{
	if (CurrentMacro)
	{
		CurrentMacro->middleClickReleased();
	}
}

void AEditorCharacter::RightClickPressed()
{
	if (CurrentMacro)
	{
		CurrentMacro->rightClickPressed();
	}
}

void AEditorCharacter::RightClickReleased()
{
	if (CurrentMacro)
	{
		CurrentMacro->rightClickReleased();
	}
}

void AEditorCharacter::AlternateModePressed()
{
	if (CurrentMacro)
	{
		CurrentMacro->alternateModePressed();
	}
}

void AEditorCharacter::AlternateModeReleased()
{
	if (CurrentMacro)
	{
		CurrentMacro->alternateModeReleased();
	}
}
