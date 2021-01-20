//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "ControlSchemes/DroneControlScheme.h"
#include "GameFramework/PhysicsVolume.h"
#include "Kismet/KismetMathLibrary.h"
#include "NObjects.h"


void UDroneControlScheme::GetUsableTorsors()
{
	TorqueTorsors.Empty(inputForces.Num() / 2);
	ForceTorsors.Empty(inputForces.Num() / 2);
	if (!getOwnerActor())
	{
		return;
	}
	AActor* owner = getOwnerActor();
	FTransform transform = owner->GetActorTransform();
	for (FTorsor torsor : inputForces)
	{
		if (!torsor.IsNull())
		{
			if (torsor.IsForceOnly())
			{
				if ((torsor.GetForceRelativeTo(transform).GetUnsafeNormal() | FVector(0, 0, 1)) > 0.95f)
				{
					ForceTorsors.Add(torsor);
				}
			}
			else if (torsor.IsTorqueOnly())
			{
				if ((torsor.GetTorqueRelativeTo(transform).GetUnsafeNormal() | FVector(0, 0, 1)) > 0.95f)
				{
					TorqueTorsors.Add(torsor);
				}
			}
		}
	}

}

TArray<float> UDroneControlScheme::Solve()
{
	UPrimitiveComponent* RootComponent = (UPrimitiveComponent*)getOwnerActor()->GetRootComponent();
	const UWorld* world = RootComponent->GetWorld();

	FVector UpVector = FVector(0, 0, 1);
	if (Rotation.X != 0.0f || Rotation.Y != 0.0f)
	{
		UpVector = FVector(Rotation.X, Rotation.Y, FVector2D(Rotation.X, Rotation.Y).Size() / UKismetMathLibrary::DegTan(AttackAngle));
	}
	FVector ForwardVector = FVector(1,0,0);
	ForwardVector = UKismetMathLibrary::RotateAngleAxis(ForwardVector, Rotation.Z, FVector(0, 0, 1));
	FRotator TargetRotator = UKismetMathLibrary::MakeRotFromZX(UpVector.GetSafeNormal(), ForwardVector);
	

	GetUsableTorsors();
	int32 NumForces = ForceTorsors.Num();
	UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] Found %d usable forces"), NumForces);
	
	FlushPersistentDebugLines(world);
	float Gravity = RootComponent->GetPhysicsVolume()->GetGravityZ();
	float Weight = Mass * Gravity;
	DrawDebugSphere(world, COM, 50, 16, FColor::White, true);
	FTransform GravityTransform = FTransform(UKismetMathLibrary::MakeRotFromZX(FVector(0, 0, 1), RootComponent->GetForwardVector()), COM); //Make rotator only taking in account Z rotation

	FVector PitchAxis = (GravityTransform.InverseTransformVector(RootComponent->GetUpVector()) ^ UpVector).GetSafeNormal();

	TArray<FVector> ForcesRelativeLocations;
	for (FTorsor torsor : ForceTorsors)
	{
		ForcesRelativeLocations.Add(torsor.GetTorsorLocationRelativeTo(GravityTransform));
		//UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] ForcesRelativeLocations[last] = %s"), *ForcesRelativeLocations.Last().ToString());
	}

	//Step 1 : Gravity
	TArray<float> ScalarForces = VoronoiAngleFill(ForcesRelativeLocations);
	TArray<bool> Saturated;
	for (int i = 0; i < NumForces; i++)
	{
		//UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] Voronoi[%d] = %f"), i, ScalarGravity[i]);
		float size2d = ForcesRelativeLocations[i].Size2D();
		ScalarForces[i] /= size2d;

	}
	NormalizeArray(ScalarForces);
	for (int i = 0; i < NumForces; i++)
	{
		FVector Vec2d = FVector(FVector2D(ForcesRelativeLocations[i]),0);
		
		//UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] DistNormalized[%d] = %f"), i, ScalarGravity[i]);
		ScalarForces[i] *= -Weight / ForceTorsors[i].Force.Size();
		//UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] DistScaled[%d] = %f"), i, ScalarGravity[i]);
		DrawDebugSphere(RootComponent->GetWorld(), GravityTransform.TransformPosition(Vec2d), 20, 16, FColor::Red, true);
		UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] Location[i] = %s"), *GravityTransform.TransformPosition(Vec2d).ToString());

	}
	CheckSaturation(ForceTorsors, ScalarForces, Saturated);


	FVector GravityMoment = FVector::ZeroVector;
	for (int i = 0; i < NumForces; i++)
	{
		GravityMoment += ForcesRelativeLocations[i] ^ (ForceTorsors[i].GetForceRelativeTo(GravityTransform) * ScalarForces[i]);
	}
	FVector GravityMomentWorld = FVector::ZeroVector;
	FVector SumForces = FVector::ZeroVector;
	FVector SumLocation = FVector::ZeroVector;
	for (int i = 0; i < NumForces; i++)
	{
		FVector Force = ForceTorsors[i].GetForceInWorld() * ScalarForces[i];
		FVector ForceMoment = (Force) ^ (ForceTorsors[i].GetTorsorLocationInWorld() - COM);
		UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] Moment %i : %s"), i, *ForceMoment.ToString());
		GravityMomentWorld += ForceMoment;
		SumForces += Force;
		SumLocation += ForceTorsors[i].GetTorsorLocationInWorld() * Force.Size();
	}
	FVector ForcesLocation = SumLocation / SumForces.Size();
	FVector DeltaLocation = COM - ForcesLocation;
	GravityMomentWorld = SumForces ^ DeltaLocation;
	DrawDebugSphere(world, ForcesLocation, 45, 16, FColor::Red, true);

	((UStaticMeshComponent*)RootComponent)->AddForceAtLocation(FVector(0,0,-Weight), COM);
	UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] Gravity Moment : %s"), *GravityMomentWorld.ToString());



	//Step 2 : calculate pitch needed
	FVector PitchMoment = GravityMoment /*+ PitchAxis * Mass*/; //Pitch moment should be updated to use angular momentum instead
	PitchMoment = UKismetMathLibrary::ProjectVectorOnToPlane(PitchMoment, FVector(0, 0, 1));
	FVector PitchNormal = PitchMoment.GetSafeNormal();
	FVector PitchTangent = PitchNormal ^ FVector(0, 0, 1); //Forces on side should push more

	TArray<int32> SideIndices, OppositeSideIndices;
	TArray<FVector> SideLocation, OppositeSideLocation;
	for (int32 i = 0; i < NumForces; i++)
	{
		if ((ForcesRelativeLocations[i] | PitchTangent) > 0)
		{
			SideIndices.Add(i);
			SideLocation.Add(ForcesRelativeLocations[i]);
		}
		else
		{
			OppositeSideIndices.Add(i);
			OppositeSideLocation.Add(ForcesRelativeLocations[i]);
		}
	}
	TArray<float> SideMomentScalars = GetSumOnDirection(PitchTangent, FVector(0, 0, 1), SideLocation);
	TArray<float> OppositeSideMomentScalars = GetSumOnDirection(-PitchTangent, FVector(0, 0, 1), OppositeSideLocation);

	if (SideMomentScalars.Num() > 0 && OppositeSideMomentScalars.Num() > 0)
	{
		FVector SideMoment = FVector::ZeroVector;
		for (int32 i = 0; i < SideIndices.Num(); i++)
		{
			int32 TorsorIndex = SideIndices[i];
			SideMoment += (ForceTorsors[TorsorIndex].GetForceRelativeTo(GravityTransform) ^ SideLocation[i]) * SideMomentScalars[i];
		}

		FVector OppositeSideMoment = FVector::ZeroVector;
		for (int32 i = 0; i < OppositeSideIndices.Num(); i++)
		{
			int32 TorsorIndex = OppositeSideIndices[i];
			OppositeSideMoment += (ForceTorsors[TorsorIndex].GetForceRelativeTo(GravityTransform) ^ OppositeSideLocation[i]) * OppositeSideMomentScalars[i];
		}

		float SideMomentMultiplier = PitchMoment.Size() / 1.0f / SideMoment.Size();
		float OppositeSideMomentMultiplier = -PitchMoment.Size() / 1.0f / OppositeSideMoment.Size();

		UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] Pitching moment : %s | Side moments : %f; %f | Multipliers : %f; %f"), *PitchMoment.ToString(), SideMoment.Size(), OppositeSideMoment.Size(), SideMomentMultiplier, OppositeSideMomentMultiplier);
		for (int32 i = 0; i < NumForces; i++)
		{
			if (SideIndices.Contains(i))
			{
				int32 newIndex = SideIndices.Find(i);
				float forceNeeded = SideMomentScalars[newIndex] * SideMomentMultiplier;
				//ScalarForces[i] += forceNeeded / ForceTorsors[i].GetForceInWorld().Size();
			}
			if (OppositeSideIndices.Contains(i))
			{
				int32 newIndex = OppositeSideIndices.Find(i);
				float forceNeeded = OppositeSideMomentScalars[newIndex] * OppositeSideMomentMultiplier;
				//ScalarForces[i] += forceNeeded / ForceTorsors[i].GetForceInWorld().Size();
			}
		}
	}
	else
	{
		UE_LOG(NObjects, Log, TEXT("[UDroneControlScheme::Solve] %i elements on side, %i on opposite"), SideIndices.Num(), OppositeSideIndices.Num());
	}


	//Rebuild order
	TArray<float> AllForces;
	AllForces.SetNum(inputForces.Num());
	for (int32 i = 0; i < inputForces.Num(); i++)
	{
		int32 ForceIndex = ForceTorsors.Find(inputForces[i]);
		if (ForceIndex != INDEX_NONE)
		{
			//AllForces[i] = ScalarForces[ForceIndex];
		}
	}
	return AllForces;
}