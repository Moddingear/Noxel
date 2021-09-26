//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Server.h"
#include "CoreMinimal.h"
#include "ControlScheme.h"
#include "ControlSchemes/DroneControlScheme.h"
#include "Noxel/CraftDataHandler.h"
#include "NObjects.h"
#include "Connectors/GunConnector.h"
#include "Connectors/ForceConnector.h"
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
		float mass; FVector COM;
		ComputeCOMFromComponents(ReplicatedCraft->Components, COM, mass);
		
		FTRVector WantedForces = ControlScheme->ApplyInputMatrix(
			FTRVector::ZeroVector, //TODO
			GetTransform(),
			FTRVector(staticMesh->GetPhysicsLinearVelocity(), staticMesh->GetPhysicsAngularVelocityInRadians()),
			FTRVector(0,0,GetWorld()->GetGravityZ(),0,0,0),
			FTRVector(FVector::OneVector * mass, FVector::OneVector));
		
		TArray<FTorsor> Torsors = ForcesOut->GetAllTorsors();
		int NumTorsors = Torsors.Num();
		TArray<FTorsorBias> BiasTemp = ForceBiases;
		TArray<FTorsorBias> BiasOrdered; BiasOrdered.Init(FTorsorBias(), NumTorsors);
		TArray<float> Results; Results.Init(0, NumTorsors);
		
		FTRVector SumUnscaled = FTRVector::ZeroVector;
		
		for (int i = 0; i < NumTorsors; ++i)
		{
			FTorsor& CurrentTorsor = Torsors[i];
			FTorsorBias Bias;
			for (int j = 0; j < BiasTemp.Num(); ++j)
			{
				if (BiasTemp[j].Source == CurrentTorsor.Source && BiasTemp[j].Index == CurrentTorsor.Index)
				{
					Bias = BiasTemp[j];
					BiasTemp.RemoveAtSwap(j);
					break;
				}
			}
			if (Bias.Source != nullptr)
			{
				BiasOrdered[i] = Bias;
				float InputValue = (Bias.Bias*WantedForces).Sum();
			}
		}
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

FJsonObjectWrapper AServer::OnReadMetadata_Implementation(const TArray<AActor*>& Components)
{
	return Super::OnReadMetadata_Implementation(Components);
}

bool AServer::OnWriteMetadata_Implementation(const FJsonObjectWrapper & Metadata, const TArray<AActor*>& Components)
{
	return Super::OnWriteMetadata_Implementation(Metadata, Components);
}

void AServer::OnRep_CraftDataHandler()
{
}
