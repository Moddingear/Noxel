//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "NObjects/NoxelPart.h"
#include "Noxel.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"
#include "..\..\Public\NObjects\NoxelPart.h"

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
	UE_LOG(Noxel, Log, TEXT("[(%s)ANoxelPart::BeginPlay] Server : %s; Part location : %s"), *GetName(), GetWorld()->IsServer() ? TEXT("True") : TEXT("False"), *GetActorLocation().ToString());
	
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
}

void ANoxelPart::OnNoxelHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor)
	{
		UE_LOG(Noxel, Log, TEXT("[ANoxelPart::OnNoxelHit] Component %s/%s has been hit by %s/%s : Impulse %s"), *GetName(), *noxelContainer->GetName(), *OtherActor->GetName(), *OtherComp->GetName(), *NormalImpulse.ToString());
	}
	else
	{
		UE_LOG(Noxel, Log, TEXT("[ANoxelPart::OnNoxelHit] Component %s/%s has been hit by %s : Impulse %s"), *GetName(), *noxelContainer->GetName(), *OtherComp->GetName(), *NormalImpulse.ToString());
	}
}

void ANoxelPart::OnRep_NoxelSave()
{
	if (IsValid(NoxelSave.Noxel))
	{
		auto connectednodes = NoxelSave.NodesConnected;
		bool HasInvalid = connectednodes.Num() == 0;
		for (int i = 0; i < connectednodes.Num(); ++i)
		{
			if (!IsValid(connectednodes[i]))
			{
				HasInvalid = true;
			}
		}
		if (!HasInvalid)
		{
			for (int i = 0; i < connectednodes.Num(); ++i)//Everything is valid : Position the NObjects, load the noxel, then force collision cook and attach components
			{
				AActor* owner = connectednodes[i]->GetOwner();
				FTransform worldtransform(GetActorTransform().ToMatrixWithScale() * NoxelSave.RelativeTransforms[i].ToMatrixWithScale());
				owner->SetActorTransform(worldtransform);
			}
			UCraftDataHandler::loadNoxelNetwork(NoxelSave);
			noxelContainer->GetRuntimeMesh()->ForceCollisionUpdate();
			check(noxelContainer)
			for (int i = 0; i < connectednodes.Num(); ++i)
			{
				AActor* owner = connectednodes[i]->GetOwner();
				if (owner == this)
				{
					continue;
				}
				FAttachmentTransformRules rules(EAttachmentRule::KeepWorld, true);
				owner->AttachToComponent(noxelContainer, rules);
				owner->SetActorRelativeTransform(NoxelSave.RelativeTransforms[i]);
				//owner->SetActorEnableCollision(false);
			}
		}
	}
}
