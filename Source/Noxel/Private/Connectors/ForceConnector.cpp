//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Connectors/ForceConnector.h"
#include "Noxel.h"
#include "NObjects/BruteForceSolver.h"

bool FTorsor::IsNearlyZero() const
{
	return Torque.IsNearlyZero() && Force.IsNearlyZero();
}

bool FTorsor::IsForceOnly() const
{
	return Torque.IsNearlyZero();
}

bool FTorsor::IsTorqueOnly() const
{
	return Force.IsNearlyZero();
}

AActor * FTorsor::GetOwner() const
{
	return Source->GetOwner();
}

FTransform FTorsor::GetOwnerTransform()
{
	return GetOwner()->GetActorTransform();
}

FVector FTorsor::GetOwnerLocation()
{
	return GetOwnerTransform().GetLocation();
}

FVector FTorsor::GetTorsorLocationInWorld()
{
	return GetOwnerTransform().TransformPosition(RelativeLocation);
}

FVector FTorsor::GetForceInWorld()
{
	return GetOwnerTransform().TransformVectorNoScale(Force);
}

FVector FTorsor::GetTorqueInWorld()
{
	return GetOwnerTransform().TransformVectorNoScale(Torque);
}

FVector FTorsor::GetTorsorLocationRelativeTo(FTransform WorldTransform)
{
	return WorldTransform.InverseTransformPosition(GetTorsorLocationInWorld());
}

FVector FTorsor::GetForceRelativeTo(FTransform WorldTransform)
{
	return WorldTransform.InverseTransformVectorNoScale(GetForceInWorld());
}

FVector FTorsor::GetTorqueRelativeTo(FTransform WorldTransform)
{
	return WorldTransform.InverseTransformVectorNoScale(GetTorqueInWorld());
}

bool FTorsor::IsSaturatedWith(float Input) const
{
	return Input > RangeMax || Input < RangeMin;
}

float FTorsor::ClampToSaturation(float Input) const
{
	return FMath::Clamp(Input, RangeMin, RangeMax);
}

FTorsor FTorsor::GetTorsorAt(FTransform WorldTransform)
{
	FTransform SourceTransform = GetOwnerTransform() * FTransform(RelativeLocation); //world to source
	FTransform cumul = SourceTransform.Inverse() * WorldTransform; //Source to new transform
	FVector transl = WorldTransform.InverseTransformVectorNoScale(cumul.GetTranslation());
	//UE_LOG(LogTemp, Log, TEXT("cumul = %s"), *cumul.ToString());
	FVector newforce = cumul.TransformVectorNoScale(Force);
	FVector torquerebased = cumul.TransformVectorNoScale(Torque);
	FVector newtorque = torquerebased - (transl ^ newforce);
	return FTorsor(newforce, newtorque, RangeMin, RangeMax);
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
