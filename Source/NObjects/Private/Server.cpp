//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Server.h"
#include "CoreMinimal.h"
#include "ControlScheme.h"
#include "ControlSchemes/DroneControlScheme.h"
#include "Noxel/CraftDataHandler.h"
#include "NObjects.h"
#include "Net/UnrealNetwork.h"

AServer::AServer()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshConstructor(TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/Server.Server'"));
	staticMesh->SetStaticMesh(MeshConstructor.Object);
	
	ControlSchemeClass = UDroneControlScheme::StaticClass();

	ForcesOut = CreateDefaultSubobject<UForceConnector>("Forces");
	ForcesOut->SetupAttachment(staticMesh, TEXT("ForceConnector"));
	ForcesOut->bIsSender = true;
	GunsOut = CreateDefaultSubobject<UGunConnector>("Guns");
	GunsOut->SetupAttachment(staticMesh, TEXT("GunConnector"));
	GunsOut->bIsSender = true;

	SetupNodeContainerBySocket(staticMesh, "Node", nodesContainer);
}

void AServer::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AServer, ReplicatedCraft, COND_OwnerOnly);
}

void AServer::BeginPlay()
{
	Super::BeginPlay();
}

void AServer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//UE_LOG(NObjects, Log, TEXT("[AServer::Tick] Enabled : %s"), *UKismetStringLibrary::Conv_BoolToString(Enabled));
	if (Enabled && ControlScheme && ReplicatedCraft)
	{
		TArray<FTorsor> Torsors = ForcesOut->GetAllTorsors();
		ControlScheme->SetForces(Torsors);
		float mass; FVector COM;
		ComputeCOMFromComponents(ReplicatedCraft->Components, COM, mass);
		ControlScheme->SetCOMAndMass(COM, mass);
		TArray<float> Results = ControlScheme->Solve();
		ForcesOut->SendAllOrders(Torsors, Results);
		FString ResultsString;
		for (float r : Results)
		{
			ResultsString += FString::SanitizeFloat(r, 7) + FString("; ");
		}
		UE_LOG(NObjects, Log, TEXT("Mass is %f, found %d forces, results : %s"), mass, Torsors.Num(), *ResultsString);
	}
}

void AServer::OnNObjectEnable_Implementation(UCraftDataHandler* Craft)
{
	UE_LOG(NObjects, Log, TEXT("[AServer::OnNObjectEnable] Control Scheme Enabled"));
	Super::OnNObjectEnable_Implementation(Craft);
	ControlScheme = NewObject<UControlScheme>(this, ControlSchemeClass);
	UE_LOG(NObjects, Log, TEXT("Control scheme is a %s, class is a %s"), *ControlScheme->GetClass()->GetName(), *ControlSchemeClass->GetName());
	ReplicatedCraft = Craft;
}

void AServer::OnNObjectDisable_Implementation()
{
	ControlScheme->MarkPendingKill();
	Super::OnNObjectDisable_Implementation();
}

FString AServer::OnReadMetadata_Implementation()
{
	return Super::OnReadMetadata_Implementation();
}

bool AServer::OnWriteMetadata_Implementation(const FString & Metadata)
{
	return Super::OnWriteMetadata_Implementation(Metadata);
}

void AServer::OnRep_CraftDataHandler()
{
}
