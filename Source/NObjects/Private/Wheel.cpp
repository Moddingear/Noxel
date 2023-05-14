//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Wheel.h"
#include "NObjects.h"
#include "Noxel.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

AWheel::AWheel()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshConstructor(
		TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/WheelMount.WheelMount'"));
	staticMesh->SetStaticMesh(MeshConstructor.Object);
	staticMesh->Rename(TEXT("Wheel Mount"));

	WheelMesh = CreateDefaultSubobject<UStaticMeshComponent>("Wheel mesh");
	WheelMesh->SetupAttachment(staticMesh, TEXT("WheelSocket"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WheelMeshConstructor(
		TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/Wheel.Wheel'"));
	WheelMesh->SetStaticMesh(WheelMeshConstructor.Object);
	WheelRadius = 93.f;

	/*PhysicsJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>("Wheel Joint");
	PhysicsJoint->SetupAttachment(staticMesh, TEXT("WheelSocket"));
	PhysicsJoint->AddLocalOffset(AttachOffset);
	//since physics does rotation then translation, invert components to have the correct order
	PhysicsJoint->OverrideComponent2 = staticMesh;
	PhysicsJoint->OverrideComponent1 = WheelMesh;

	PhysicsJoint->SetLinearXLimit(LCM_Locked, 0);
	PhysicsJoint->SetLinearYLimit(LCM_Locked, 0);
	
	PhysicsJoint->SetAngularSwing1Limit(ACM_Locked, 360);
	PhysicsJoint->SetAngularTwistLimit(ACM_Locked, 360);
	
	PhysicsJoint->SetLinearZLimit(LCM_Limited, SuspensionLength);
	PhysicsJoint->SetAngularSwing2Limit(ACM_Free, 360);*/


	ForceIn->SetupAttachment(staticMesh, TEXT("ForceConnector"));

	nodesContainer->SetNodeSize(50.f);
	SetupNodeContainerBySocket(staticMesh, "Node", nodesContainer);
}

void AWheel::BeginPlay()
{
	Super::BeginPlay();
	PreviousWheelSpeed = 0;
}

void AWheel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Enabled || !staticMesh->IsAnySimulatingPhysics())
	{
		return;
	}
	
	FTransform LocationWorld = GetActorTransform();
	FVector ActorLoc = LocationWorld.GetLocation();
	FQuat ActorRot = LocationWorld.GetRotation();
	FVector LocalSpeed = ActorRot.UnrotateVector(staticMesh->GetPhysicsLinearVelocityAtPoint(ActorLoc));

	//forces applied on car/ground
	WheelFrictionData fd = GetFrictionData(), pfd = GetPreviousFrictionData();

	FVector WheelNormal = (fd.GroundCast.Location - fd.GroundCast.ImpactPoint).GetSafeNormal();
	FVector LateralVector = (FVector(0,0,1) ^ ActorRot.GetForwardVector()).GetSafeNormal();
	float LateralReaction = WheelNormal | LateralVector;
	float NormalReaction = WheelNormal.Z;
	if (FMath::Abs(LateralReaction/NormalReaction) < LateralFrictionCoefficient)
	{
		WheelNormal = WheelNormal - LateralReaction * LateralVector;
		WheelNormal.Normalize();
	}
	else
	{
		WheelNormal = WheelNormal - copysign(LateralFrictionCoefficient*NormalReaction, LateralReaction/NormalReaction) * LateralVector;
		WheelNormal.Normalize();
	}

	float LateralSpeed = LocalSpeed.Y;
	float LateralFriction = -fd.SuspensionForce * LateralSpeed * LateralFrictionCoefficient / 100.f * fd.IsOnGround;
	LateralFriction = FMath::Clamp(LateralFriction, -MaxLateralFriction, MaxLateralFriction);

	FVector SuspensionForce = WheelNormal * fd.SuspensionForce;
	FVector LateralForce = LateralVector * LateralFriction;
	FVector forceVector = SuspensionForce + LateralForce;

	FVector forceLoc = fd.GroundCast.ImpactPoint;
	//ActorLoc - LocationWorld.GetUnitAxis(EAxis::Z) * (GroundDistance + WheelRadius);

	staticMesh->AddForceAtLocation(forceVector, forceLoc); //TODO: Equal but opposite force on ground

	//wheel animation
	float ForwardSpeed = fd.IsOnGround
		                     ? LocalSpeed.X
		                     : PreviousWheelSpeed
		                     - FMath::Sign(PreviousWheelSpeed) * FMath::Min(
			                     DeltaTime * 100 + DeltaTime * abs(PreviousWheelSpeed), abs(PreviousWheelSpeed));
	PreviousWheelSpeed = ForwardSpeed;
	float AddedRotation = ForwardSpeed / WheelRadius * DeltaTime;
	FVector WheelLocation = staticMesh->GetSocketLocation(TEXT("WheelSocket")) - GetActorUpVector() * fd.
		DistanceToGround;
	WheelMesh->SetWorldLocation(WheelLocation);
	WheelMesh->AddLocalRotation(FRotator::MakeFromEuler(FVector(0, -FMath::RadiansToDegrees(AddedRotation), 0)));

	//force vectors
	FVector ArrowLoc = forceLoc + GetActorUpVector() * 10;

	DrawDebugDirectionalArrow(GetWorld(), ArrowLoc,
	                          ArrowLoc + LateralForce / MaxLateralFriction * 1000.f,
	                          100, FColor::Green, false, 0, 0, 10);
	DrawDebugDirectionalArrow(GetWorld(), ArrowLoc,
	                          ArrowLoc + SuspensionForce / MaxSuspensionWeight / 9.8f,
	                          100, FColor::Blue, false, 0, 0, 10);
	//UE_LOG(NObjects, Log, TEXT("[AEDF::Tick] Lift : %f"), lift * MaxLift);
}

TArray<FTorsor> AWheel::GetMaxTorsor()
{
	WheelFrictionData fd = GetFrictionData();
	float forceapplied = FMath::Max(fd.SuspensionForce * ForwardFrictionCoefficient * fd.IsOnGround, MaxForce);
	FVector forcedir = FVector(0, 1, 0) ^ fd.GroundCast.Normal;
	FTorsor Force = FTorsor(forcedir * forceapplied, FVector::ZeroVector, -1.f, 1.f);
	return TArray<FTorsor>({Force});
}

bool AWheel::IsContactingGround(FHitResult& OutHit)
{
	TArray<FHitResult> OutHits;
	FVector startTrace = staticMesh->GetSocketLocation(TEXT("WheelSocket")), endTrace = startTrace + GetActorUpVector()
		        * -SuspensionLength;

	//bool hit = GetWorld()->LineTraceSingleByChannel(OutHit, startTrace, endTrace, ECollisionChannel::ECC_WorldStatic);//TODO : better trace
	bool hit = UKismetSystemLibrary::SphereTraceMultiByProfile(this,
	                                                           startTrace, endTrace, WheelRadius, TEXT("PhysicsActor"),
	                                                           true, {GetAttachParentActor()},
	                                                           EDrawDebugTrace::ForOneFrame, OutHits, true);
	//UE_LOG(NObjects, Log, TEXT("[AWheel::IsContactingGround] %s:%d starting trace at %s"), *GetName(), GetWorld()->IsServer(), *startTrace.ToString());
	int ChosenIndex = INDEX_NONE;
	for (int i = 0; i < OutHits.Num(); ++i)
	{
		if (OutHit.GetActor() == this)
		{
			continue;
		}
		if (IsValid(GetAttachParentActor()))
		{
			if (OutHit.GetActor() == GetAttachParentActor())
			{
				continue;
			}
		}
		if (ChosenIndex == INDEX_NONE || OutHits[i].Distance < OutHits[ChosenIndex].Distance)
		{
			ChosenIndex = i;
		}
	}
	if (ChosenIndex != INDEX_NONE)
	{
		OutHit = OutHits[ChosenIndex];
	}
	else
	{
		OutHit.bBlockingHit = false;
		OutHit.Distance = SuspensionLength;
		OutHit.ImpactPoint = endTrace - GetActorUpVector() * WheelRadius;
		OutHit.ImpactNormal = GetActorUpVector();
		OutHit.Location = endTrace;
	}

	if (IsValid(OutHit.GetActor()) && IsValid(OutHit.GetComponent()))
	{
		//UE_LOG(NObjects, Log, TEXT("[AWheel::IsContactingGround]%d Chosen collision is from %s/%s"), GetWorld()->IsServer(), *OutHit.GetActor()->GetName(), *OutHit.GetComponent()->GetName())
	}
	return ChosenIndex != INDEX_NONE;
}

float AWheel::GetSuspensionForce(float NewDistanceToGround, float dt)
{
	//Positive force : Push stuff outwards
	const float extension = FMath::Clamp(NewDistanceToGround / SuspensionLength, 0.f, 1.f);
	const float spring = (1 - extension) * MaxSuspensionWeight * 9800.f;
	const float diffextension = FMath::Clamp(PreviousExtension - extension, 0.f, dt / Damping);
	const float damper = diffextension / dt * Damping * MaxSuspensionWeight * 9800.f;
	PreviousExtension = extension;
	return spring + damper;
}

AWheel::WheelFrictionData AWheel::GetFrictionData()
{
	if (GFrameCounter == ThisFrameFriction.frameNumber)
	{
		return ThisFrameFriction;
	}
	else
	{
		PreviousFrameFriction = ThisFrameFriction;
		ThisFrameFriction.frameNumber = GFrameCounter;
		ThisFrameFriction.IsOnGround = IsContactingGround(ThisFrameFriction.GroundCast);
		ThisFrameFriction.DistanceToGround = ThisFrameFriction.GroundCast.Distance;
		const float dt = GetWorld()->GetDeltaSeconds();
		ThisFrameFriction.SuspensionForce = GetSuspensionForce(ThisFrameFriction.DistanceToGround, dt);
		return ThisFrameFriction;
	}
}

AWheel::WheelFrictionData AWheel::GetPreviousFrictionData()
{
	if (PreviousFrameFriction.frameNumber + 1 != GFrameCounter)
	{
		GetFrictionData();
		if (PreviousFrameFriction.frameNumber + 1 != GFrameCounter)
		{
			PreviousFrameFriction = ThisFrameFriction;
		}
	}
	return PreviousFrameFriction;
}

void AWheel::OnNObjectEnable_Implementation(UCraftDataHandler* Craft)
{
	Super::OnNObjectEnable_Implementation(Craft);
	UE_LOG(NObjects, Log, TEXT("[AWheel::OnNObjectEnable_Implementation] Called on %s"),
	       GetWorld()->IsServer() ? TEXT("server") : TEXT("client"))
	ForceIn->OnGetReceivedDelegate.AddDynamic(this, &AWheel::OnGetReceived);
	ForceIn->OnSetReceivedDelegate.AddDynamic(this, &AWheel::OnSetReceived);

	if (IsValid(WheelMesh))
	{
		//WheelMesh->SetVisibility(false);
		WheelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		UE_LOG(NObjects, Warning, TEXT("[AWheel::OnNObjectEnable_Implementation] wheel mesh is invalid for %s"),
		       GetWorld()->IsServer() ? TEXT("server") : TEXT("client"))
	}
}

void AWheel::OnNObjectDisable_Implementation()
{
	Super::OnNObjectDisable_Implementation();
}

FJsonObjectWrapper AWheel::OnReadMetadata_Implementation(const TArray<AActor*>& Components)
{
	return Super::OnReadMetadata_Implementation(Components);
}

bool AWheel::OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components)
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
	WheelFrictionData fd = GetFrictionData(), pfd = GetPreviousFrictionData();
	FVector forceLoc = fd.GroundCast.ImpactPoint;
	FVector ForwardForceVector = GetActorRightVector() ^ (pfd.GroundCast.Location - pfd.GroundCast.ImpactPoint).
		GetSafeNormal();
	float force = FMath::Clamp(ForceIn->GetLastOrder()[0], -1.f, 1.f) * pfd.IsOnGround;
	float ForwardFriction = pfd.SuspensionForce * force * ForwardFrictionCoefficient;
	ForwardFriction = FMath::Clamp(ForwardFriction, -MaxForce, MaxForce);
	FVector forceVector = ForwardForceVector * ForwardFriction;
	staticMesh->AddForceAtLocation(forceVector, forceLoc);
	FVector ArrowLoc = forceLoc + GetActorUpVector() * 10;
	DrawDebugDirectionalArrow(GetWorld(), ArrowLoc,
	                          ArrowLoc + forceVector / MaxForce * 1000.f, 100,
	                          FColor::Red, false, 0, 0, 10);
	//UE_LOG(NObjects, Log, TEXT("[AEDF::OnSetReceived] Set received"));
}
