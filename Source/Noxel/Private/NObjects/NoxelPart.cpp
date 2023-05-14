//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "NObjects/NoxelPart.h"
#include "Noxel.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"

#include "Net/UnrealNetwork.h"
#include "NObjects/NObjectInterface.h"
#include "Noxel/CraftDataHandler.h"


// Sets default values
ANoxelPart::ANoxelPart()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	bReplicates = true;
	SetReplicatingMovement(true);
	PrimaryActorTick.bCanEverTick = true;
	noxelContainer = CreateDefaultSubobject<UNoxelContainer>(TEXT("Noxel Container"));
	noxelContainer->SetCollisionProfileName(TEXT("NObject"));
	RootComponent = noxelContainer;
	
	//noxelContainer->SetNotifyRigidBodyCollision(true);
	//noxelContainer->OnComponentHit.AddDynamic(this, &ANoxelPart::OnNoxelHit);
	
	nodesContainer = CreateDefaultSubobject<UNodesContainer>(TEXT("Nodes Container"));
	nodesContainer->SetupAttachment(RootComponent);
	nodesContainer->SetNodesDefault(TArray<FVector>(), true);

	
	FRepMovement &mov = GetReplicatedMovement_Mutable();
	mov.LocationQuantizationLevel = EVectorQuantization::RoundOneDecimal;
	mov.RotationQuantizationLevel = ERotatorQuantization::ShortComponents;
	mov.VelocityQuantizationLevel = EVectorQuantization::RoundTwoDecimals;

}

void ANoxelPart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANoxelPart, NoxelSave);
}

// Called when the game starts or when spawned
void ANoxelPart::BeginPlay()
{
	Super::BeginPlay();
	OnRep_NoxelSave();
	//UE_LOG(Noxel, Log, TEXT("[(%s)ANoxelPart::BeginPlay] Server : %s; Part location : %s"), *GetName(), GetWorld()->IsServer() ? TEXT("True") : TEXT("False"), *GetActorLocation().ToString());
	
}

UNoxelContainer * ANoxelPart::GetNoxelContainer()
{
	return noxelContainer;
}

UNodesContainer * ANoxelPart::GetNodesContainer()
{
	return nodesContainer;
}

// Called every frame
void ANoxelPart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!hasLoadedFromNetworkSave)
	{
		OnRep_NoxelSave();
	}
	if (IsValid(NoxelSave.Noxel) && GFrameCounter % 60 == 0)
	{
		//UE_LOG(Noxel, Log, TEXT("[ANoxelPart::Tick]%d Part %s has mass %f, angular %s"), GetWorld()->IsServer(), *GetName(), noxelContainer->GetMass(), *noxelContainer->GetInertiaTensor().ToString());
	}
}

void ANoxelPart::OnNoxelHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor)
	{
		UE_LOG(Noxel, Log, TEXT("[ANoxelPart::OnNoxelHit]%d Component %s/%s has been hit by %s/%s : Impulse %s"), GetWorld()->IsServer(), *GetName(), *noxelContainer->GetName(), *OtherActor->GetName(), *OtherComp->GetName(), *NormalImpulse.ToString());
	}
	else
	{
		UE_LOG(Noxel, Log, TEXT("[ANoxelPart::OnNoxelHit]%d Component %s/%s has been hit by %s : Impulse %s"), GetWorld()->IsServer(), *GetName(), *noxelContainer->GetName(), *OtherComp->GetName(), *NormalImpulse.ToString());
	}
}

void ANoxelPart::OnRep_AttachmentReplication()
{
	
}

/*void ANoxelPart::GatherCurrentMovement()
{
	
}*/

void ANoxelPart::OnRep_NoxelSave()
{
	if (!IsValid(NoxelSave.Noxel) || GetWorld()->IsServer() || NoxelSave.Noxel != noxelContainer) //if invalid or server
	{
		hasLoadedFromNetworkSave = true;
	}
	else
	{
		auto connectednodes = NoxelSave.Noxel->GetConnectedNodesContainers();
		bool HasInvalid = connectednodes.Num() == 0;
		for (int i = 0; i < connectednodes.Num(); ++i)
		{
			if (!IsValid(connectednodes[i]))
			{
				HasInvalid = true;
				break;
			}
		}
		if (HasInvalid)
		{
			hasLoadedFromNetworkSave = false;
		}
		else
		{
			NoxelSave.NodesConnected = connectednodes;
			hasLoadedFromNetworkSave = true;
			noxelContainer->SetSimulatePhysics(false);
			UE_LOG(NoxelDataNetwork, Log, TEXT("[%s::OnRep_NoxelSave] Loading noxel for battle"), *GetName())
			UE_LOG(NoxelDataNetwork, Verbose, TEXT("[ANoxelPart::OnRep_NoxelSave] Relative transform for part is \n%s"), *noxelContainer->GetComponentTransform().ToHumanReadableString())
			for (int i = 0; i < connectednodes.Num(); ++i)//Everything is valid : Position the NObjects, load the noxel, then force collision cook and attach components
			{
				AActor* owner = connectednodes[i]->GetOwner();
				if (owner == this)
				{
					continue;
				}
				UPrimitiveComponent* rootPrimitive = CastChecked<UPrimitiveComponent>(owner->GetRootComponent());
				if (rootPrimitive->GetAttachmentRootActor() != owner)
				{
					UE_LOG(NoxelDataNetwork, Verbose, TEXT("[ANoxelPart::OnRep_NoxelSave] Component %s/%s was attached (to %s/%s) before noxel load."), *owner->GetName(), *rootPrimitive->GetName(), *rootPrimitive->GetAttachmentRootActor()->GetName(), *rootPrimitive->GetAttachParent()->GetName())
				}
				//rootPrimitive->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				
				FTransform worldtransform = NoxelSave.RelativeTransforms[i] * noxelContainer->GetComponentTransform();
				owner->SetActorTransform(worldtransform);
				UE_LOG(NoxelDataNetwork, Verbose, TEXT("[ANoxelPart::OnRep_NoxelSave] World transform for %d is \n%s"), i, *worldtransform.ToHumanReadableString());
			
			}
			UCraftDataHandler::loadNoxelNetwork(NoxelSave);
			noxelContainer->GetRuntimeMesh()->ForceCollisionUpdate();
			for (int i = 0; i < connectednodes.Num(); ++i)
			{
				AActor* owner = connectednodes[i]->GetOwner();
				UPrimitiveComponent* rootPrimitive = CastChecked<UPrimitiveComponent>(owner->GetRootComponent());
				if (owner != this) //skip attaching self to self
				{
					FAttachmentTransformRules rules(EAttachmentRule::KeepWorld, false);
                    owner->AttachToComponent(noxelContainer, rules);
					UE_LOG(NoxelDataNetwork, Verbose, TEXT("[ANoxelPart::OnRep_NoxelSave] After attach, world transform for %d is \n%s"), i, *owner->GetActorTransform().ToHumanReadableString());
				}
				
				//owner->SetActorRelativeTransform(NoxelSave.RelativeTransforms[i]);
				//rootPrimitive->UpdateComponentToWorld();
				//owner->SetActorEnableCollision(false);
			}
			noxelContainer->SetSimulatePhysics(true);
			
		}
	}
}
