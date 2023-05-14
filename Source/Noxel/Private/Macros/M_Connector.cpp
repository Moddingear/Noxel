//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_Connector.h"

#include "Components/SplineMeshComponent.h"
#include "Noxel/NoxelCombatLibrary.h"
#include "Noxel/CraftDataHandler.h"
#include "Noxel/NoxelNetworkingAgent.h"

#include "Components/WidgetInteractionComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Connectors/ConnectorBase.h"

#include "Kismet/KismetMathLibrary.h"

AM_Connector::AM_Connector() 
{
	AlternationMethod = EAlternateType::Hold;
	MoveWithFollowComponent = false;
}

// Called when the game starts
void AM_Connector::BeginPlay()
{
	Super::BeginPlay();
	//UE_LOG(NoxelMacro, Warning, TEXT("Macro added"));
	GetCraft()->SetNodesContainersVisibility(false);
	GetCraft()->SetNoxelContainersVisibility(false);
}

void AM_Connector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetCraft()->SetNodesContainersVisibility(true);
	GetCraft()->SetNoxelContainersVisibility(true);
	Super::EndPlay(EndPlayReason);
}

TArray<UConnectorBase*> AM_Connector::GetAllConnectors()
{
	TArray<AActor*> Actors = GetCraft()->GetComponents();
	TArray<UConnectorBase*> Connectors;
	for (AActor* Actor : Actors)
	{
		Connectors.Append(UNoxelCombatLibrary::GetConnectorsFromActor(Actor));
	}
	return Connectors;
}

void AM_Connector::UpdateDisplayedConnectors()
{
	//Step 0 : Get all connectors
	TArray<UConnectorBase*> Connectors = GetAllConnectors();
	//Step 1 : Display the connectors
	//display new
	TArray<UConnectorBase*> DisplayedConnectors2 = DisplayedConnectors;
	for (int i = 0; i < Connectors.Num(); ++i)
	{
		UConnectorBase* Connector = Connectors[i];
		if (DisplayedConnectors2.Remove(Connector) == 0)
		{
			FTransform ConnectorTransform = Connector->GetComponentTransform();
			//FQuat rotated = ConnectorTransform.GetRotation() * FRotator(0,0,45).Quaternion();
			//ConnectorTransform.SetRotation(rotated);
			UStaticMeshComponent* ConnectorMesh = NewObject<UStaticMeshComponent>(this);
			ConnectorMesh->RegisterComponent();
			ConnectorMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			ConnectorMesh->SetWorldTransform(ConnectorTransform);
			ConnectorMesh->SetStaticMesh(Connector->ConnectorMesh);
			ConnectorMesh->SetCollisionProfileName(TEXT("Connector"));
			DisplayedConnectors.Add(Connector);
			ConnectorsMeshes.Add(ConnectorMesh);
			UE_LOG(NoxelMacro, Log, TEXT("[AM_Connector::UpdateDisplayedConnectors] Found connector in %s at rel %s, abs %s"),
				*Connector->GetOwner()->GetName(), *Connector->GetRelativeTransform().ToString(), *ConnectorTransform.ToString());
		}
	}
	//cleanup unneeded
	for (int i = DisplayedConnectors2.Num() - 1; i >= 0; --i)
	{
		const int IndexInArray = DisplayedConnectors.Find(DisplayedConnectors2[i]);
		if(IndexInArray != INDEX_NONE)
		{
			UStaticMeshComponent* MeshToRemove = ConnectorsMeshes[IndexInArray];
			MeshToRemove->DestroyComponent();
			ConnectorsMeshes.RemoveAt(IndexInArray);
			DisplayedConnectors.RemoveAt(IndexInArray);
		}
	}
}

void AM_Connector::ShowOnlyConnectableConnectors()
{
	for (int i = 0; i < DisplayedConnectors.Num(); ++i)
	{
		UConnectorBase* ThisConnector = DisplayedConnectors[i];
		UStaticMeshComponent* ThisMesh = ConnectorsMeshes[i];
		bool ShouldDisplay = true;
		if (IsValid(SelectedConnector))
		{
			if (UConnectorBase::CanBothConnect(SelectedConnector, ThisConnector)
				|| SelectedConnector->Connected.Contains(ThisConnector)
				|| SelectedConnector == ThisConnector)
			{
				ShouldDisplay = true;
			}
			else
			{
				ShouldDisplay = false;
			}
		}
		else
		{
			ShouldDisplay = true;
		}
		if (ShouldDisplay)
		{
			ThisMesh->SetVisibility(true);
			ThisMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		else
		{
			ThisMesh->SetVisibility(false);
			ThisMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AM_Connector::DestroyAllWires()
{
	for (int i = WiresMeshes.Num() - 1; i >= 0; --i)
	{
		WiresMeshes[i]->DestroyComponent();
	}
	WiresMeshes.Reset();
}

USplineMeshComponent* AM_Connector::MakeWire(FTransform Sender, FTransform Receiver, UStaticMesh* WireMesh)
{
	USplineMeshComponent* WireComponent = NewObject<USplineMeshComponent>(this);
	WireComponent->SetMobility(EComponentMobility::Movable);
	WireComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	WireComponent->SetStaticMesh(WireMesh);
	float Distance = FVector::Distance(Sender.GetLocation(), Receiver.GetLocation());
	float TangentScale = Distance;
	WireComponent->SetStartAndEnd(Sender.GetLocation(), Sender.GetRotation().GetForwardVector() * TangentScale,
		Receiver.GetLocation(), -Receiver.GetRotation().GetForwardVector() * TangentScale);
	WireComponent->SetStartRoll(Sender.Rotator().Roll);
	WireComponent->SetEndRoll(-Receiver.Rotator().Roll);
	WireComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WireComponent->RegisterComponent();
	return WireComponent;
}

void AM_Connector::MakeAllWires()
{
	for (int SenderIdx = 0; SenderIdx < DisplayedConnectors.Num(); ++SenderIdx)
	{
		UConnectorBase* Sender = DisplayedConnectors[SenderIdx];
		UStaticMeshComponent* SenderMesh = ConnectorsMeshes[SenderIdx];
		if (Sender->bIsSender && SenderMesh->IsVisible())
		{
			for (int ReceiverIdx = 0; ReceiverIdx < Sender->Connected.Num(); ++ReceiverIdx)
			{
				UConnectorBase* Receiver = Sender->Connected[ReceiverIdx];
				int ReceiverIndexInArray = DisplayedConnectors.Find(Receiver);
				if (ConnectorsMeshes.IsValidIndex(ReceiverIndexInArray))
				{
					UStaticMeshComponent* ReceiverMesh = ConnectorsMeshes[ReceiverIndexInArray];
					if (ReceiverMesh->IsVisible())
					{
						WiresMeshes.Add(MakeWire(SenderMesh->GetComponentTransform(),
							ReceiverMesh->GetComponentTransform(), Sender->WireMesh));
					}
				}
			}
		}
	}
}

UConnectorBase * AM_Connector::GetConnectorClicked()
{
	FHitResult Hit;
	FVector TraceStart, TraceEnd;
	GetTraceFromFollow(TraceStart, TraceEnd);
	FCollisionQueryParams Params;
	Params.bTraceComplex = false;
	if(ActorLineTraceSingle(Hit, TraceStart, TraceEnd, ECollisionChannel::ECC_WorldDynamic, Params))
	{
		int IndexInArray = ConnectorsMeshes.Find(Cast<UStaticMeshComponent>(Hit.GetComponent()));
		if (DisplayedConnectors.IsValidIndex(IndexInArray))
		{
			return DisplayedConnectors[IndexInArray];
		}
	}
	return nullptr;
}

// Called every frame
void AM_Connector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateDisplayedConnectors();
	ShowOnlyConnectableConnectors();
	DestroyAllWires(); //TODO : Avoid destroying everything, prefer updating location of objects instead
	MakeAllWires();
	FVector CameraPos, CameraDir;
	GetRayFromFollow(CameraPos, CameraDir);

	if (SelectedConnector)
	{
		const int AIndex = DisplayedConnectors.Find(SelectedConnector);
		if (ConnectorsMeshes.IsValidIndex(AIndex))
		{
			UStaticMeshComponent* A = ConnectorsMeshes[AIndex];
			FTransform ATransform = A->GetComponentTransform();
			UConnectorBase* Clicked = GetConnectorClicked();
			FTransform BTransform;
			FVector Location, Direction;
			GetRayFromFollow(Location, Direction);
			BTransform.SetLocation(Location+Direction*200.f);
			FVector AB = BTransform.GetLocation() - ATransform.GetLocation();
			FRotator BRot = UKismetMathLibrary::MakeRotFromZX(FVector::UpVector, -AB.GetSafeNormal());
			BTransform.SetRotation(BRot.Quaternion());
            if (IsValid(Clicked) && Clicked != SelectedConnector)
            {
            	const int BIndex = DisplayedConnectors.Find(Clicked);
            	if (ConnectorsMeshes.IsValidIndex(BIndex))
            	{
            		UStaticMeshComponent* B = ConnectorsMeshes[BIndex];
            		BTransform = B->GetComponentTransform();
            	}
            }
			
			USplineMeshComponent* WireMesh;
			if (SelectedConnector->bIsSender)
			{
				WireMesh = MakeWire(ATransform, BTransform, SelectedConnector->WireMesh);
			}
			else
			{
				WireMesh = MakeWire(BTransform, ATransform, SelectedConnector->WireMesh);
			}
			WiresMeshes.Add(WireMesh);
		}
		
	}
	
	/*for (UConnectorBase* Connector : Connectors)
	{
		if(SelectedConnector)
		{
			if(!UConnectorBase::CanBothConnect(Connector, SelectedConnector) && Connector != SelectedConnector)
			{
				continue;
			}
		}
		FTransform ConnectorTransform = Connector->GetComponentTransform();
		FVector PlayerToConnector = ConnectorTransform.GetLocation() - CameraPos;
		FVector DirToConnector = PlayerToConnector.GetUnsafeNormal();
		float DistOnSphere = FMath::GetMappedRangeValueClamped(FVector2D(DistMin,DistMax), FVector2D(SphereSizeMin,SphereSizeMax), PlayerToConnector.Size());
		FVector ProjectionToSphere = CameraPos + DirToConnector * DistOnSphere;
		FRotator Dir = UKismetMathLibrary::MakeRotFromZY(-DirToConnector, GetActorUpVector());
		FVector Scale = FVector::OneVector*FMath::Pow(DistOnSphere, 1/3);
		FTransform LocationWorld = FTransform(Dir, ProjectionToSphere, Scale);
		UMaterialInstanceDynamic* DynMaterial = nullptr;
		if(ConnectorMaterial)
		{
			DynMaterial = UMaterialInstanceDynamic::Create(ConnectorMaterial, this);
			if(Connector->ConnectorTexture)
			{
				DynMaterial->SetTextureParameterValue("Texture", Connector->ConnectorTexture);
			}
			else
			{
				UE_LOG(NoxelMacro, Warning, TEXT("[AM_Connector::TickActor] Connector texture is invalid"));
			}
			DynMaterial->SetScalarParameterValue("bIsMale", Connector->bIsMale * 1.0f);
		}
		else
		{
			UE_LOG(NoxelMacro, Warning, TEXT("[AM_Connector::TickActor] Default material is invalid"));
		}
		int32 sectionIdx = DrawPlaneMaterial(LocationWorld, DynMaterial, true);
		//UE_LOG(Noxel, Log, TEXT("[AM_Connector::TickActor] SectionIndex : %d"), sectionIdx);
		ConnectorSectionMap.Add(sectionIdx, Connector);

		if (Connector->bIsMale)
		{
			for (UConnectorBase* OtherConnector : Connector->Connected)
			{
				DrawLine(Connector->GetComponentLocation(), OtherConnector->GetComponentLocation());
			}
		}
	}*/

	if (Alternate)
	{
		LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "ConnectorAutoSingle", "Auto-Connect to all");
		RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "ConnectorAutoAll", "Auto-Connect everything");
	}
	else 
	{
		if (SelectedConnector)
		{
			LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "ConnectorSelectB", "(Release) Select a second connector");
		}
		else
		{
			LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "ConnectorSelectA", "(Hold) Select a connector");
		}
		RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "ConnectorDestroyAll", "Destroy all connections");
	}
	
}

void AM_Connector::leftClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Left click"));
	SelectedConnector = GetConnectorClicked();
	if (Alternate && IsValid(SelectedConnector))
	{
		TArray<UConnectorBase*> Connectors = GetAllConnectors();
		TArray<UConnectorBase*> A, B;
		for (int32 ConnectorIdx = 0; ConnectorIdx < Connectors.Num(); ConnectorIdx++)
		{
			if (UConnectorBase::CanBothConnect(SelectedConnector, Connectors[ConnectorIdx]))
			{
				A.Add(SelectedConnector);
				B.Add(Connectors[ConnectorIdx]);
			}
		}
		FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
        queue->AddConnectorConnectOrder(A, B);
        GetNoxelNetworkingAgent()->SendCommandQueue(queue);
		SelectedConnector = nullptr;
	}
}

void AM_Connector::leftClickReleased_Implementation()
{
	if(SelectedConnector)
	{
		UConnectorBase* SelectedConnector2 = GetConnectorClicked();
		if (UConnectorBase::AreConnected(SelectedConnector, SelectedConnector2))
		{
			FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
			queue->AddConnectorDisconnectOrder({SelectedConnector}, {SelectedConnector2});
			GetNoxelNetworkingAgent()->SendCommandQueue(queue);
			UE_LOG(Noxel, Log, TEXT("Disconnected %s and %s"),
				*SelectedConnector->GetConnectorName(), *SelectedConnector2->GetConnectorName());
		}
		else if(UConnectorBase::CanBothConnect(SelectedConnector, SelectedConnector2))
		{
			FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
			queue->AddConnectorConnectOrder({SelectedConnector}, {SelectedConnector2});
			GetNoxelNetworkingAgent()->SendCommandQueue(queue);
			UE_LOG(Noxel, Log, TEXT("Connected %s and %s"),
				*SelectedConnector->GetConnectorName(), *SelectedConnector2->GetConnectorName());
		}
		SelectedConnector = nullptr;
	}
}

void AM_Connector::middleClickPressed_Implementation()
{
}

void AM_Connector::middleClickReleased_Implementation()
{
}

void AM_Connector::rightClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Right click"));
	if (Alternate)
	{
		TArray<UConnectorBase*> Connectors, A, B;
		Connectors = GetAllConnectors();
		for (UConnectorBase* ConnectorA : Connectors)
		{
			if (ConnectorA->bIsSender)
			{
				for (UConnectorBase* ConnectorB : Connectors)
				{
					if (UConnectorBase::CanBothConnect(ConnectorA, ConnectorB))
					{
						A.Add(ConnectorA);
						B.Add(ConnectorB);
					}
				}
			}
		}
		FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
		queue->AddConnectorConnectOrder(A, B);
		GetNoxelNetworkingAgent()->SendCommandQueue(queue);
	}
	else
	{
		UConnectorBase* Selected = GetConnectorClicked();
		if (IsValid(Selected))
		{
			TArray<UConnectorBase*> OldConnections = Selected->Connected;
			TArray<UConnectorBase*> A; A.Init(Selected, OldConnections.Num());
			FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
			queue->AddConnectorDisconnectOrder(A, OldConnections);
			GetNoxelNetworkingAgent()->SendCommandQueue(queue);
		}
	}
}

void AM_Connector::rightClickReleased_Implementation()
{
}