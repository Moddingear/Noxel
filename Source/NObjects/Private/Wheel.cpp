//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Wheel.h"
#include "NObjects.h"

AWheel::AWheel()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshConstructor(TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/WheelMount.WheelMount'"));
	staticMesh->SetStaticMesh(MeshConstructor.Object);

	WheelMesh = CreateDefaultSubobject<UStaticMeshComponent>("Wheel mesh");
	WheelMesh->SetupAttachment(staticMesh, TEXT("WheelSocket"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WheelMeshConstructor(TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/Wheel.Wheel'"));
	WheelMesh->SetStaticMesh(WheelMeshConstructor.Object);
	
	ForceIn->SetupAttachment(staticMesh, TEXT("ForceConnector"));

	nodesContainer->SetNodeSize(50.f);
	SetupNodeContainerBySocket(staticMesh, "Node", nodesContainer);

}

void AWheel::BeginPlay()
{
	Super::BeginPlay();
}

void AWheel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Enabled && ForceIn->GetLastOrder().Num() >= 1)
	{
		FTransform LocationWorld = GetActorTransform();
		float force = FMath::Clamp(ForceIn->GetLastOrder()[0], -1.f, 1.f);
		FVector forceVector = GetActorForwardVector() * force * MaxForce;
		FVector forceLoc = GetActorLocation();
		staticMesh->AddForceAtLocation(forceVector, forceLoc);
		FVector ArrowLoc = GetActorLocation() + GetActorUpVector() * 100;
		FVector ArrowEnd = ArrowLoc + GetActorForwardVector() * 1000 * force;
		DrawDebugDirectionalArrow(GetWorld(), ArrowLoc, ArrowEnd, 100, FColor::Red, false, 0, 0, 10);
		//UE_LOG(NObjects, Log, TEXT("[AEDF::Tick] Lift : %f"), lift * MaxLift);
	}
}

TArray<FTorsor> AWheel::GetMaxTorsor()
{
	FTorsor Force = FTorsor(FVector(MaxForce, 0, 0), FVector::ZeroVector, -1.f, 1.f);
	return TArray<FTorsor>({ Force });
}

void AWheel::OnNObjectEnable_Implementation(UCraftDataHandler* Craft)
{
	Super::OnNObjectEnable_Implementation(Craft);
	ForceIn->OnGetReceivedDelegate.AddDynamic(this, &AWheel::OnGetReceived);
	ForceIn->OnSetReceivedDelegate.AddDynamic(this, &AWheel::OnSetReceived);
}

void AWheel::OnNObjectDisable_Implementation()
{
	Super::OnNObjectDisable_Implementation();
}

FJsonObjectWrapper AWheel::OnReadMetadata_Implementation(const TArray<AActor*>& Components)
{
	return Super::OnReadMetadata_Implementation(Components);
}

bool AWheel::OnWriteMetadata_Implementation(const FJsonObjectWrapper & Metadata, const TArray<AActor*>& Components)
{
	return Super::OnWriteMetadata_Implementation(Metadata, Components);
}

void AWheel::OnGetReceived()
{
	TArray<FTorsor> Torsors = GetMaxTorsor();
	ForceIn->SetTorsors(Torsors);
}

void AWheel::OnSetReceived()
{
	//UE_LOG(NObjects, Log, TEXT("[AEDF::OnSetReceived] Set received"));
}
