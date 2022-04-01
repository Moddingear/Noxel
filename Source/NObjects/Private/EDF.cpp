//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "EDF.h"
#include "NObjects.h"

AEDF::AEDF()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshConstructor(TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/EDF/EDF_Body.EDF_Body'"));
	staticMesh->SetStaticMesh(MeshConstructor.Object);

	BladesTop = CreateDefaultSubobject<UStaticMeshComponent>("BladesTop");
	BladesTop->SetupAttachment(staticMesh, TEXT("BladesSocket"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BladesMeshTop(TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/EDF/EDF_BladesTop.EDF_BladesTop'"));
	BladesTop->SetStaticMesh(BladesMeshTop.Object);

	BladesBottom = CreateDefaultSubobject<UStaticMeshComponent>("BladesBottom");
	BladesBottom->SetupAttachment(staticMesh, TEXT("BladesSocket"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BladesMeshBottom(TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/EDF/EDF_BladesBottom.EDF_BladesBottom'"));
	BladesBottom->SetStaticMesh(BladesMeshBottom.Object);
	
	ForceIn->SetupAttachment(staticMesh, TEXT("ForceConnector"));

	nodesContainer->SetNodeSize(50.f);
	SetupNodeContainerBySocket(staticMesh, "Node", nodesContainer);

}

void AEDF::BeginPlay()
{
	Super::BeginPlay();
}

void AEDF::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Enabled && ForceIn->GetLastOrder().Num() >= 2)
	{
		FTransform LocationWorld = GetActorTransform();
		float lift = FMath::Clamp(ForceIn->GetLastOrder()[0], 0.f, 1.f), torque = FMath::Clamp(ForceIn->GetLastOrder()[1], -1.f, 1.f);
		float addfortop = FMath::Sign(TopPropellerRotationSpeed) * BladesInverted ? 1.f : -1.f;
		float RotationSpeedTop = TopPropellerRotationSpeed * (lift + torque * addfortop);
		float RotationSpeedBottom = - TopPropellerRotationSpeed * (lift - torque * addfortop);
		BladesTop->AddRelativeRotation(FRotator::MakeFromEuler(FVector(0.f, 0.f, RotationSpeedTop * DeltaTime)));
		BladesBottom->AddRelativeRotation(FRotator::MakeFromEuler(FVector(0.f, 0.f, RotationSpeedBottom * DeltaTime)));
		staticMesh->AddForceAtLocationLocal(FVector(0, 0, 1) * lift * MaxLift, FVector::ZeroVector);
		//UE_LOG(NObjects, Log, TEXT("[AEDF::Tick] Lift : %f"), lift * MaxLift);
	}
	else {
		float TopPropRotationDirection = FMath::Sign(TopPropellerRotationSpeed);
		BladesTop->AddRelativeRotation(FRotator::MakeFromEuler(FVector(0.f, 0.f, TopPropRotationDirection * DeltaTime)));
		BladesBottom->AddRelativeRotation(FRotator::MakeFromEuler(FVector(0.f, 0.f, -TopPropRotationDirection * DeltaTime)));
	}
	
}

bool AEDF::ShouldInvertBlades()
{
	if (ForceIn->Connected.Num() == 0)
	{
		return false;
	}
	AActor* ParentOwner = ForceIn->Connected[0]->GetOwner();
	//Get location relative to parent
	FVector RelativeLocation = ParentOwner->GetActorTransform().InverseTransformPosition(GetActorLocation());
	return (RelativeLocation.X > 0 ? 1 : -1) * (RelativeLocation.Y > 0 ? 1 : -1) < 0;
}

void AEDF::CheckBlades()
{
	BladesInverted = ShouldInvertBlades();
	BladesTop->SetRelativeScale3D(FVector(1, BladesInverted ? -1 : 1, 1));
	BladesBottom->SetRelativeScale3D(FVector(1, BladesInverted ? -1 : 1, 1));
}

TArray<FTorsor> AEDF::GetMaxTorsor()
{
	FTorsor Force = FTorsor(FVector(0, 0, MaxLift), FVector::ZeroVector, 0.f, 1.f);
	FTorsor Torque = FTorsor(FVector::ZeroVector, FVector(0, 0, MaxTorque), -1.f, 1.f);
	return TArray<FTorsor>({ Force, Torque });
}

void AEDF::OnNObjectEnable_Implementation(UCraftDataHandler* Craft)
{
	Super::OnNObjectEnable_Implementation(Craft);
	CheckBlades();
	ForceIn->OnGetReceivedDelegate.AddDynamic(this, &AEDF::OnGetReceived);
	ForceIn->OnSetReceivedDelegate.AddDynamic(this, &AEDF::OnSetReceived);
}

void AEDF::OnNObjectDisable_Implementation()
{
	Super::OnNObjectDisable_Implementation();
}

FJsonObjectWrapper AEDF::OnReadMetadata_Implementation(const TArray<AActor*>& Components)
{
	return Super::OnReadMetadata_Implementation(Components);
}

bool AEDF::OnWriteMetadata_Implementation(const FJsonObjectWrapper & Metadata, const TArray<AActor*>& Components)
{
	return Super::OnWriteMetadata_Implementation(Metadata, Components);
}

void AEDF::OnGetReceived()
{
	TArray<FTorsor> Torsors = GetMaxTorsor();
	ForceIn->SetTorsors(Torsors);
}

void AEDF::OnSetReceived()
{
	//UE_LOG(NObjects, Log, TEXT("[AEDF::OnSetReceived] Set received"));
}
