//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Connectors/ConnectorBase.h"

#include "Noxel.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UConnectorBase::UConnectorBase()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	//PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	bIsSender = false;

	ConnectorID = FString("None");
	ConnectorName = FText();
	static ConstructorHelpers::FObjectFinder<UStaticMesh>ConnectorMeshConstructor(TEXT("StaticMesh'/Game/NoxelEditor/Connectors/Connector_JST_1x4.Connector_JST_1x4'"));
	ConnectorMesh = ConnectorMeshConstructor.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh>WireMeshConstructor(TEXT("StaticMesh'/Game/NoxelEditor/Connectors/Wire_4.Wire_4'"));
	WireMesh = WireMeshConstructor.Object;
}

void UConnectorBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UConnectorBase, Connected, COND_InitialOnly);
}

// Called when the game starts
void UConnectorBase::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->IsServer())
	{
		OnConnectedUpdate();
	}
}


// Called every frame
void UConnectorBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UConnectorBase::CanConnect(UConnectorBase * other)
{
	if (!other || other == this)
	{
		//UE_LOG(Noxel, Log, TEXT("[UConnectorBase::CanConnect] Can't connect : invalid"));
		return false;
	}
	if (GetClass() == other->GetClass() //Strict equality needed : connector types are each a class
		&& ( (other->CanSend() && CanReceive()) || (CanSend() && other->CanReceive()) ))
	{
		return true;
	}
	//UE_LOG(Noxel, Log, TEXT("[UConnectorBase::CanConnect] Can't connect : different"));
	return false;
}

bool UConnectorBase::CanSend()
{
	return bIsSender;
}

bool UConnectorBase::CanReceive()
{
	return !bIsSender;
}

bool UConnectorBase::Connect(UConnectorBase * other)
{
	if (CanBothConnect(this, other) && !AreConnected(this, other))
	{
		Connected.Add(other);
		other->Connected.Add(this);
		if (bIsSender)
		{
			other->GetOwner()->AddTickPrerequisiteActor(GetOwner());
		}
		if (other->bIsSender)
		{
			GetOwner()->AddTickPrerequisiteActor(other->GetOwner());
		}
		return true;
	}
	return false;
}

bool UConnectorBase::Disconnect(UConnectorBase * other)
{
	if (AreConnected(this, other))
	{
		Connected.Remove(other);
		other->Connected.Remove(this);
		if (bIsSender)
		{
			other->GetOwner()->RemoveTickPrerequisiteActor(GetOwner());
		}
		return true;
	}
	return false;
}

bool UConnectorBase::CanBothConnect(UConnectorBase * A, UConnectorBase * B)
{
	return A->CanConnect(B) && B->CanConnect(A);
}

bool UConnectorBase::AreConnected(UConnectorBase * A, UConnectorBase * B)
{
	return A->Connected.Contains(B) && B->Connected.Contains(A);
}

void UConnectorBase::OnConnectedUpdate()
{
	if (bIsSender)
	{
		AActor* Owner = GetOwner();
		for (int32 i = 0; i < Connected.Num(); i++) //Only connect if everything is valid
		{
			if (!IsValid(Connected[i]))
			{
				checkf(!GetWorld()->IsServer(), TEXT("[UConnectorBase::OnConnectedUpdate] Invalid connector reference on server side !"));
				return;
			}
		}
		for (int32 i = 0; i < Connected.Num(); i++)
		{
			AActor* OtherOwner = Connected[i]->GetOwner();
			OtherOwner->AddTickPrerequisiteActor(Owner);
		}
		if (!GetWorld()->IsServer())
		{
			UE_LOG(NoxelDataNetwork, Log, TEXT("[UConnectorBase::OnConnectedUpdate] fired"));
		}
	}
}

FString UConnectorBase::GetConnectorName() const
{
	return GetOwner()->GetName() + TEXT(":") + GetName();
}

