//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Wheel.h"
#include "NObjects.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

AWheel::AWheel()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshConstructor(TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/WheelMount.WheelMount'"));
	staticMesh->SetStaticMesh(MeshConstructor.Object);

	FVector AttachOffset = FVector(0,0, -SuspensionLength/2);
	
	WheelMesh = CreateDefaultSubobject<UStaticMeshComponent>("Wheel mesh");
	WheelMesh->SetupAttachment(staticMesh, TEXT("WheelSocket"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WheelMeshConstructor(TEXT("StaticMesh'/Game/NoxelEditor/NObjects/Meshes/Wheel.Wheel'"));
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
}

void AWheel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Enabled && ForceIn->GetLastOrder().Num() >= 1)
	{
		FTransform LocationWorld = GetActorTransform();
		FVector ActorLoc = LocationWorld.GetLocation();
		FQuat ActorRot = LocationWorld.GetRotation();
		FVector LocalSpeed = ActorRot.UnrotateVector(staticMesh->GetPhysicsLinearVelocityAtPoint(ActorLoc));

		//forces applied on car/ground
		float GroundDistance;
		bool GroundContact = IsContactingGround(GroundDistance);
		float SuspensionForce = GetSuspensionForce(GroundDistance, DeltaTime);
		
		float force = FMath::Clamp(ForceIn->GetLastOrder()[0], -1.f, 1.f) * GroundContact;
		
		float LateralSpeed = LocalSpeed.Y;
		float LateralFriction = -SuspensionForce * LateralSpeed * LateralFrictionCoefficient/100.f * GroundContact;
		LateralFriction = FMath::Clamp(LateralFriction, -MaxLateralFriction, MaxLateralFriction);
		
		FVector forceVector = ActorRot.RotateVector(FVector(force * MaxForce, LateralFriction, SuspensionForce));
		
		FVector forceLoc = ActorLoc - LocationWorld.GetUnitAxis(EAxis::Z) * (GroundDistance + WheelRadius);
		staticMesh->AddForceAtLocation(forceVector, forceLoc); //TODO: Equal but opposite force on ground

		//wheel animation
		float ForwardSpeed = LocalSpeed.X;
		float AddedRotation = ForwardSpeed/WheelRadius*DeltaTime;
		FVector WheelLocation = staticMesh->GetSocketLocation(TEXT("WheelSocket")) - GetActorUpVector() * GroundDistance;
		WheelMesh->SetWorldLocation(WheelLocation);
		WheelMesh->AddLocalRotation(FRotator::MakeFromEuler(FVector(0, -FMath::RadiansToDegrees(AddedRotation), 0)));
		
		//force vectors
		FVector ArrowLoc = forceLoc + GetActorUpVector() * 10;
		DrawDebugDirectionalArrow(GetWorld(), ArrowLoc,
			ArrowLoc + GetActorForwardVector() * 1000 * force, 100, FColor::Red, false, 0, 0, 10);
		DrawDebugDirectionalArrow(GetWorld(), ArrowLoc,
			ArrowLoc + GetActorRightVector() * LateralFriction / MaxLateralFriction *1000.f,
			100, FColor::Green, false, 0, 0, 10);
		DrawDebugDirectionalArrow(GetWorld(), ArrowLoc,
			ArrowLoc + GetActorUpVector() * SuspensionForce / MaxSuspensionWeight/9.8f,
			100, FColor::Blue, false, 0, 0, 10);
		//UE_LOG(NObjects, Log, TEXT("[AEDF::Tick] Lift : %f"), lift * MaxLift);
		
	}
}

TArray<FTorsor> AWheel::GetMaxTorsor()
{
	float DistanceToGround;
	bool IsOnGround = IsContactingGround(DistanceToGround);
	FTorsor Force = FTorsor(FVector(MaxForce * IsOnGround, 0, 0), FVector::ZeroVector, -1.f, 1.f);
	return TArray<FTorsor>({ Force });
}

bool AWheel::IsContactingGround(float& DistanceToGround)
{
	FHitResult OutHit;
	FVector startTrace = staticMesh->GetSocketLocation(TEXT("WheelSocket")), endTrace = startTrace + GetActorUpVector() * -(WheelRadius + SuspensionLength);
	
	bool hit = GetWorld()->LineTraceSingleByChannel(OutHit, startTrace, endTrace, ECollisionChannel::ECC_WorldStatic);//TODO : better trace
	if (hit)
	{
		DistanceToGround = FMath::Clamp(OutHit.Distance - WheelRadius, 0.f, SuspensionLength);
	}
	else
	{
		DistanceToGround = SuspensionLength;
	}
	return hit;
}

float AWheel::GetSuspensionForce(float NewDistanceToGround, float dt)
{
	//Positive force : Push stuff outwards
	float extension = FMath::Clamp(NewDistanceToGround/SuspensionLength, 0.f, 1.f);
	float spring = (1-extension) * MaxSuspensionWeight*9800.f;
	float damper = -(extension - PreviousExtension)/dt * Damping * MaxSuspensionWeight*9800.f;
	PreviousExtension = extension;
	return spring + damper;
}

void AWheel::OnNObjectEnable_Implementation(UCraftDataHandler* Craft)
{
	Super::OnNObjectEnable_Implementation(Craft);
	ForceIn->OnGetReceivedDelegate.AddDynamic(this, &AWheel::OnGetReceived);
	ForceIn->OnSetReceivedDelegate.AddDynamic(this, &AWheel::OnSetReceived);

	if (IsValid(WheelMesh))
	{
		WheelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
