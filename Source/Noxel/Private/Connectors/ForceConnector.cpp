//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Connectors/ForceConnector.h"
#include "Noxel.h"
#include "NObjects/BruteForceSolver.h"

AActor * FTorsor::GetOwner()
{
	return Source->GetOwner();
}

FForceSource FTorsor::ToForceSource(FTransform WorldLocation)
{
	FTorsor movedTorsor = GetTorsorAt(WorldLocation);
	FForceSource ForceSource;
	ForceSource.ForceAndTorque = FTRVector(movedTorsor.Force, movedTorsor.Torque);
	ForceSource.RangeMax = RangeMax;
	ForceSource.RangeMin = RangeMin;
	return ForceSource;
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

void UForceConnector::SendOrder(TArray<float>& Order, UForceConnector* Target)
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
		if (!bIsSender && Connected.Num() > 0) //If receiver and already connected
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
		Torsors.Append(GetMaxTorsor(Cast<UForceConnector>(Connected[i])));
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
		if (Torsors[i].Source != CurrentSource) //Source to send data to changed
		{
			if (CurrentSource)
			{
				SendOrder(CurrentStack, CurrentSource); //Send data that was spooled to source
			}
			CurrentStack.Empty(); //Reset
			CurrentSource = Torsors[i].Source;
		}
		CurrentStack.Add(Scalars[i]); //Spool data
	}
	if (CurrentSource) //Send remaining data
	{
		SendOrder(CurrentStack, CurrentSource);
	}
}

void UForceConnector::SetTorsors(TArray<FTorsor>& InMaxTorsors)
{
	MaxTorsors = InMaxTorsors;
	for (int i = 0; i < MaxTorsors.Num(); ++i)
	{
		MaxTorsors[i].Source = this;
		MaxTorsors[i].Index = i;
	}
}

TArray<float> UForceConnector::GetLastOrder()
{
	return LastOrder;
}
