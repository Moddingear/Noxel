//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Connectors/GunConnector.h"
#include "Noxel.h"

UGunConnector::UGunConnector()
{
	ConnectorID = FString("Gun");
	ConnectorName = NSLOCTEXT(CONNECTOR_NAMESPACE, "Gun", "Parallel Gun Connector");
}

void UGunConnector::BeginPlay()
{
	Super::BeginPlay();
}

void UGunConnector::Fire()
{
	if (CanSend())
	{
		for (UConnectorBase* connector : Connected)
		{
			((UGunConnector*)connector)->TriggerSetReceived();
		}
	}
}

void UGunConnector::GetGunInformation(UGunConnector * Target, float & OutReloadTime, bool & bOutReadyToFire, float & OutReloadDuration)
{
	if (Target && CanSend())
	{
		Target->TriggerGetReceived();
		OutReloadTime = Target->ReloadTime;
		bOutReadyToFire = Target->bReadyToFire;
		OutReloadDuration = Target->ReloadDuration;
	}
}
