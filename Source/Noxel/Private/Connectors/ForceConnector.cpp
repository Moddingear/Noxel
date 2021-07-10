//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Connectors/ForceConnector.h"
#include "Noxel.h"


AActor * FTorsor::GetOwner()
{
	return Source->GetOwner();
}

UForceConnector::UForceConnector()
{
	ConnectorID = FString("Force");
	ConnectorName = NSLOCTEXT(CONNECTOR_NAMESPACE, "Force", "Serial Force Connector");
}

void UForceConnector::BeginPlay()
{
	Super::BeginPlay();
}

void UForceConnector::SendOrder(TArray<float> Order, UForceConnector* Target)
{
	if (Target && CanSend())
	{
		Target->LastOrder = Order;
		Target->TriggerSetReceived();
	}
}

TArray<FTorsor> UForceConnector::GetMaxTorsor(UForceConnector * Target)
{
	if (Target && CanSend())
	{
		Target->TriggerGetReceived();
		return Target->MaxTorsors;
	}
	return TArray<FTorsor>();
}

bool UForceConnector::CanConnect(UConnectorBase * other)
{
	if (UConnectorBase::CanConnect(other))
	{
		if (!bIsSender && Connected.Num() > 0) //If female and already connected
		{
			return false;
		}
		return true;
	}
	return false;
}

TArray<FTorsor> UForceConnector::GetAllTorsors()
{
	TArray<FTorsor> Torsors;
	Torsors.Reserve(Connected.Num());
	for (int32 i = 0; i < Connected.Num(); i++)
	{
		Torsors.Append(GetMaxTorsor((UForceConnector*)Connected[i]));
	}
	return Torsors;
}

void UForceConnector::SendAllOrders(TArray<FTorsor>& Torsors, TArray<float>& Scalars)
{
	Scalars.SetNum(Torsors.Num());
	UForceConnector* CurrentSource = NULL;
	TArray<float> CurrentStack;
	for (int32 i = 0; i < Torsors.Num(); i++)
	{
		if (Torsors[i].Source != CurrentSource)
		{
			if (CurrentSource)
			{
				SendOrder(CurrentStack, CurrentSource);
			}
			CurrentStack.Empty();
			CurrentSource = Torsors[i].Source;
		}
		CurrentStack.Add(Scalars[i]);
	}
	if (CurrentSource)
	{
		SendOrder(CurrentStack, CurrentSource);
	}
}
