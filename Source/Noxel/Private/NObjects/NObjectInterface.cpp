//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "NObjects/NObjectInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Noxel/NodesContainer.h"
#include "Noxel.h"

void INObjectInterface::OnNObjectEnable_Implementation(UCraftDataHandler* Craft)
{
}

void INObjectInterface::OnNObjectDisable_Implementation()
{
}

bool INObjectInterface::OnNObjectAttach_Implementation(ANoxelPart * Part)
{
	return false;
}

FJsonObjectWrapper INObjectInterface::OnReadMetadata_Implementation(const TArray<AActor*>& Components)
{
	return FJsonObjectWrapper();
}

bool INObjectInterface::OnWriteMetadata_Implementation(const FJsonObjectWrapper& Metadata, const TArray<AActor*>& Components)
{
	return false;
}

void INObjectInterface::SetReplicatedAttachmentData_Implementation(FNoxelReplicatedAttachmentData data)
{
	
}

bool INObjectInterface::IsAttachedAtFinalLocation_Implementation()
{
	return false;
}

void INObjectInterface::SetupNodeContainerBySocket(UStaticMeshComponent * Mesh, FString SocketRegex, UNodesContainer * Target)
{
	TArray<FName> Sockets = Mesh->GetAllSocketNames();
	//UE_LOG(Noxel, Log, TEXT("[INObjectInterface::SetupNodeContainerBySocket] Starting"));
	TArray<FVector> Nodes;
	int32 NodesFound = 0;
	for (FName Socket : Sockets)
	{
		if (Socket.ToString().Contains(SocketRegex))
		{
			//UE_LOG(Noxel, Log, TEXT("[INObjectInterface::SetupNodeContainerBySocket] Found %s"), *Socket.ToString());
			const UStaticMeshSocket* FullSocket = Mesh->GetSocketByName(Socket);
			FVector newLocation = Target->GetComponentTransform().InverseTransformPosition(Mesh->GetComponentTransform().TransformPosition(FullSocket->RelativeLocation));
			Nodes.Add(newLocation);
			NodesFound++;
		}
	}
	Target->SetNodesDefault(Nodes, false);
	UE_LOG(Noxel, Log, TEXT("[%s::SetupNodeContainerBySocket] Found %d nodes"), *Mesh->GetOwner()->GetClass()->GetFName().ToString(), NodesFound);
}

bool INObjectInterface::ComputeCOMFromComponents(TArray<AActor*> &Actors, FVector &COM, float &Mass, FVector& InertiaTensor)
{
	FVector WeightedCenter = FVector::ZeroVector;
	float TotalMass = 0;
	InertiaTensor = FVector::ZeroVector;
	TArray<UPrimitiveComponent*> Roots;
	for (int i = 0; i < Actors.Num(); i++)
	{
		AActor* curr = Actors[i];
		if (!curr->HasValidRootComponent())
		{
			continue;
		}
		if (curr->GetRootComponent()->IsA<UPrimitiveComponent>())
		{
			UPrimitiveComponent* root = Cast<UPrimitiveComponent>(curr->GetRootComponent());
			if (!root->IsSimulatingPhysics())
			{
				UE_LOG(Noxel, VeryVerbose, TEXT("[INObjectInterface::ComputeCOMFromComponents] Component %s is not simulating physics, skipping"), *root->GetPathName());
				continue;
			}
			if (root->IsWelded())
			{
				UE_LOG(Noxel, VeryVerbose, TEXT("[INObjectInterface::ComputeCOMFromComponents] Component %s is welded to another object, skipping"), *root->GetPathName());
				continue;
			}
			Roots.Add(root);
			FTransform rootTransform = root->GetComponentTransform();
			float mass = root->GetMass();
			FVector com = root->GetCenterOfMass();
			WeightedCenter += com * mass;
			TotalMass += mass;
		}
	}
	COM = WeightedCenter / TotalMass;
	Mass = TotalMass;

	for (UPrimitiveComponent* root : Roots)
	{
		FTransform rootTransform = root->GetComponentTransform();
		float mass = root->GetMass();
		FVector com = root->GetCenterOfMass();
		FVector AngularInertia = root->GetInertiaTensor();
		FVector InertiaRebased = rootTransform.TransformVector(AngularInertia);
		FVector InertiaFinal = InertiaRebased + (com - COM) * mass;
		InertiaTensor += InertiaFinal;
	}
	
	return true;
}

bool INObjectInterface::IsAttachmentValid() const
{
	const AActor* self = CastChecked<AActor>(this);
	auto attrep = self->GetRootComponent()->GetAttachmentRootActor();
	return attrep != self && IsValid(attrep) && attrep != self->GetRootComponent()->GetOwner();
}

void INObjectInterface::CheckNetworkAttachment(FString CallContext) const
{
	const AActor* self = CastChecked<AActor>(this);
	FString role = self->GetWorld()->IsServer() ? "server" : "client";
	if (IsAttachmentValid())
	{
		UE_LOG(NoxelDataNetwork, Log, TEXT("[%s] Object %s on %s, attached to %s at %s"), *CallContext,
			*self->GetPathName(), *role, *self->GetRootComponent()->GetAttachParent()->GetPathName(), *self->GetRootComponent()->GetComponentLocation().ToString());
	}
	else
	{
		UE_LOG(NoxelDataNetwork, Log, TEXT("[%s] Object %s on %s at location %s, not attached"), *CallContext,
			*self->GetPathName(), *role, *self->GetActorLocation().ToString());
	}
}
