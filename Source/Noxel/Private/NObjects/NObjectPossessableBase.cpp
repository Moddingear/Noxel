//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "NObjects/NObjectPossessableBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ANObjectPossessableBase::ANObjectPossessableBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicatingMovement(true);
	staticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	staticMesh->SetCollisionProfileName(TEXT("NObject"));
	RootComponent = Cast<USceneComponent>(staticMesh);
	staticMesh->bEditableWhenInherited = true;
	staticMesh->SetIsReplicated(true);

	nodesContainer = CreateDefaultSubobject<UNodesContainer>(TEXT("Nodes Container"));
	nodesContainer->SetupAttachment(RootComponent);
	nodesContainer->bEditableWhenInherited = true;

	CameraRotationPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Rotation point"));
	CameraRotationPoint->SetupAttachment(RootComponent);
	CameraRotationPoint->SetRelativeLocation(FVector(0, 0, 100));
	CameraRotationPoint->bEditableWhenInherited = true;
	CameraRotationPoint->bUseAttachParentBound = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraRotationPoint);
	Camera->bEditableWhenInherited = true;
	OrbitDistance = 2000.f;
	Camera->bUsePawnControlRotation = false;
	Camera->bUseAttachParentBound = true;

	TranslationInputs = FVector::ZeroVector;
	RotationInputs = FVector::ZeroVector;
}

void ANObjectPossessableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANObjectPossessableBase, Enabled);
	DOREPLIFETIME(ANObjectPossessableBase, ParentCraft);
}

// Called when the game starts or when spawned
void ANObjectPossessableBase::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ANObjectPossessableBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	pitch = FMath::Clamp(pitch - pitchInput, -90.f, 90.f);
	yaw = yaw + yawInput;

	FRotator rotation = FRotator(0.f, yaw, 0.f).Add(pitch, 0.f, 0.f);
	FTransform forward = FTransform(rotation);
	Camera->SetWorldRotation(rotation);
	Camera->SetWorldLocation(CameraRotationPoint->GetComponentLocation() - forward.GetUnitAxis(EAxis::X) * OrbitDistance);
}

void ANObjectPossessableBase::OnNObjectEnable_Implementation(UCraftDataHandler* Craft)
{
	Enabled = true;
	ParentCraft = Craft;
}

void ANObjectPossessableBase::OnNObjectDisable_Implementation()
{
	Enabled = false;
}

bool ANObjectPossessableBase::OnNObjectAttach_Implementation(ANoxelPart* Part)
{
	return false;
}

FJsonObjectWrapper ANObjectPossessableBase::OnReadMetadata_Implementation(const TArray<AActor*>& Components)
{
	return FJsonObjectWrapper();
}

bool ANObjectPossessableBase::OnWriteMetadata_Implementation(const FJsonObjectWrapper & Metadata, const TArray<AActor*>& Components)
{
	return false;
}

void ANObjectPossessableBase::SetupPlayerInputComponent(UInputComponent * PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	//Action mappings ----------------------------------------------------------------
	/*InputComponent->BindAction("Main mouse button", IE_Pressed, this, &AEditorCharacter::LeftClickPressed);
	InputComponent->BindAction("Main mouse button", IE_Released, this, &AEditorCharacter::LeftClickReleased);

	InputComponent->BindAction("Secondary mouse button", IE_Pressed, this, &AEditorCharacter::RightClickPressed);
	InputComponent->BindAction("Secondary mouse button", IE_Released, this, &AEditorCharacter::RightClickReleased);

	InputComponent->BindAction("Ternary mouse button", IE_Pressed, this, &AEditorCharacter::MiddleClickPressed);
	InputComponent->BindAction("Ternary mouse button", IE_Released, this, &AEditorCharacter::MiddleClickReleased);

	InputComponent->BindAction("Alternate Mode", IE_Pressed, this, &AEditorCharacter::AlternateModePressed);
	InputComponent->BindAction("Alternate Mode", IE_Released, this, &AEditorCharacter::AlternateModeReleased);*/

	//Axis mappings ----------------------------------------------------------------

	InputComponent->BindAxis("Forwards/Backwards", this, &ANObjectPossessableBase::MoveX);
	InputComponent->BindAxis("Left/Right", this, &ANObjectPossessableBase::MoveY);
	InputComponent->BindAxis("Up/Down", this, &ANObjectPossessableBase::MoveZ);
	InputComponent->BindAxis("Mouse X Axis", this, &ANObjectPossessableBase::LookYaw);
	InputComponent->BindAxis("Mouse Y Axis", this, &ANObjectPossessableBase::LookPitch);
}

void ANObjectPossessableBase::MoveX(float input)
{
	TranslationInputs.X = input;
}

void ANObjectPossessableBase::MoveY(float input)
{
	TranslationInputs.Y = input;
}

void ANObjectPossessableBase::MoveZ(float input)
{
	TranslationInputs.Z = input;
}

void ANObjectPossessableBase::LookPitch(float input)
{
	pitchInput = input;
}

void ANObjectPossessableBase::LookYaw(float input)
{
	yawInput = input;
}

