//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Server.h"
#include "CoreMinimal.h"
#include "ControlScheme.h"
#include "ControlSchemes/DroneControlScheme.h"
#include "ControlSchemes/CarControlScheme.h"
#include "Noxel/CraftDataHandler.h"
#include "NObjects.h"
#include "Connectors/GunConnector.h"
#include "Connectors/ForceConnector.h"
#include "Net/UnrealNetwork.h"
#include "NObjects/BruteForceSolver.h"

AServer::AServer()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshConstructor(TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/Server.Server'"));
	staticMesh->SetStaticMesh(MeshConstructor.Object);
	
	ControlSchemeClass = UCarControlScheme::StaticClass();

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
	Solver = NewObject<UBruteForceSolver>();
}

void AServer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//UE_LOG(NObjects, Log, TEXT("[AServer::Tick] Enabled : %s"), *UKismetStringLibrary::Conv_BoolToString(Enabled));
	if (Enabled && ControlScheme && ReplicatedCraft)
	{
		float Mass; FVector COM, InertiaTensor;
		ComputeCOMFromComponents(ReplicatedCraft->Components, COM, Mass, InertiaTensor);
		//Convert inertia tensor to local space:
		InertiaTensor = GetTransform().InverseTransformVectorNoScale(InertiaTensor);

		const FTRVector Inertia(FVector(Mass), InertiaTensor);
		FTRVector ForcesScaler = ControlScheme->GetUsedForces() / Inertia; //Vector that only retains the forces to be optimized and their scales
		ForcesScaler.Translation /= 1000.f;

		const FTRVector WantedForces = ControlScheme->ApplyInputMatrix(
			FTRVector(TranslationInputs, FVector::ZeroVector), //TODO get inputs from the player
			GetTransform(),
			Camera->GetComponentTransform(),
			FTRVector(staticMesh->GetPhysicsLinearVelocity(), staticMesh->GetPhysicsAngularVelocityInRadians()),
			FTRVector(0,0,GetWorld()->GetGravityZ(),0,0,0), Inertia);

		const FTRVector WantedOutput = WantedForces * ForcesScaler;
		
		TArray<FTorsor> Torsors = ForcesOut->GetAllTorsors();
		const int NumTorsors = Torsors.Num();

		TArray<FForceSource> TorsorsConverted;
		TorsorsConverted.Reserve(NumTorsors);

		FTransform COMTransform = FTransform(GetActorRotation(), COM);
		
		for (int i = 0; i < NumTorsors; ++i)
		{
			FForceSource Source = Torsors[i].ToForceSource(COMTransform);
			//UE_LOG(NObjects, Log, TEXT("Source %d : %s"), i, *Source.ToString());
			Source.ForceAndTorque = Source.ForceAndTorque * ForcesScaler;
			TorsorsConverted.Add(Source);
		}
		for (int i = 0; i < TorsorsConverted.Num(); ++i)
		{
			//UE_LOG(NObjects, Log, TEXT("Source %d : %s"), i, *TorsorsConverted[i].ToString());
		}
		if (Solver->GetNumRunners() > 0)
		{
			FOutputColumn col = Solver->GetOutput(0);
			//UE_LOG(NObjects, Log, TEXT("Runner done (after %d iterations)! Matrix :\r\n%s"), Solver->GetRunnerIteration(0), *col.ToString());
			FTRVector outVec = UBruteForceSolver::GetOutputVector(LastTorsors, col, false);
			//UE_LOG(NObjects, Log, TEXT("[AServer::Tick] Test Runner, result %s after %d iterations, will ask for %s"), *outVec.ToString(), Solver->GetRunnerIteration(0), *WantedOutput.ToString());
			ForcesOut->SendAllOrders(Torsors, col.InputCoefficients);
			FTRVector SumAbs = FTRVector::ZeroVector;
			for (FForceSource Torsor : LastTorsors)
			{
				FTRVector rangemax = Torsor.ForceAndTorque * Torsor.RangeMax;
				FTRVector rangemin = Torsor.ForceAndTorque * Torsor.RangeMin;
				FTRVector abs = FTRVector::MaxComponents(rangemax.GetAbs(), rangemin.GetAbs());
				SumAbs += abs;
			}
			FTRVector diff = WantedOutput - outVec;
			//FTRVector diff = WantedOutput;
			
			//UE_LOG(NObjects, Log, TEXT("[AServer::Tick] Distance to target : %f (%s), SumAbs magnitude normalized : %f (%s)"), FTRVector::Distance(outVec, WantedOutput), *diff.ToString(), SumAbs.GetSize(), *SumAbs.ToString());

			/*FTRVector diffunscaled = diff * Inertia;
			staticMesh->AddForce(GetTransform().TransformVector(diffunscaled.Translation));
			staticMesh->AddTorqueInRadians(GetTransform().TransformVector(diffunscaled.Rotation));*/
			//staticMesh->AddForce(FVector(0,0, -GetWorld()->GetGravityZ()), NAME_None, true); //remove gravity
			Solver->ClearRunners();
		}
		Solver->StartSolveInputs(TorsorsConverted, WantedOutput, 10000);
		LastTorsors = TorsorsConverted;
		
		/*FString ResultsString;
		for (float r : Results)
		{
			ResultsString += FString::SanitizeFloat(r, 7) + FString("; ");
		}
		UE_LOG(NObjects, Log, TEXT("Mass is %f, found %d forces, results : %s"), mass, Torsors.Num(), *ResultsString);*/
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
