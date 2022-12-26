//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "NObjects/NoxelPart.h"
#include "Noxel.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"
#include "..\..\Public\NObjects\NoxelPart.h"


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
		UE_LOG(Noxel, Log, TEXT("Component %s/%s has been hit by %s/%s : Impulse %s"), *GetName(), *noxelContainer->GetName(), *OtherActor->GetName(), *OtherComp->GetName(), *NormalImpulse.ToString());
	}
	else
	{
		UE_LOG(Noxel, Log, TEXT("Component %s/%s has been hit by %s : Impulse %s"), *GetName(), *noxelContainer->GetName(), *OtherComp->GetName(), *NormalImpulse.ToString());
	}
}

