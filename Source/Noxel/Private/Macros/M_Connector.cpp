//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_Connector.h"

#include "Noxel/NoxelCombatLibrary.h"
#include "Noxel/CraftDataHandler.h"
#include "Noxel/NoxelNetworkingAgent.h"

#include "Components/WidgetInteractionComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Connectors/ConnectorBase.h"

#include "Kismet/KismetMathLibrary.h"

AM_Connector::AM_Connector() 
{
	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialFinder(TEXT("Material'/Game/Materials/Connector.Connector'"));
	if(MaterialFinder.Succeeded())
	{
		ConnectorMaterial = MaterialFinder.Object;
	}
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

UConnectorBase * AM_Connector::GetConnectorClicked()
{
	FHitResult Hit;
	FVector TraceStart, TraceEnd;
	getTrace(TraceStart, TraceEnd);
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;
	//if (LineTraceComponent(Hit, TraceStart, TraceEnd, Params)) //trace against just this component
	{
		//int32 SectionIndex, FaceIndex;
		//GetSectionIdAndFaceIdFromCollisionFaceIndex(Hit.FaceIndex, SectionIndex, FaceIndex);
		/*if (ConnectorSectionMap.Contains(SectionIndex))
		{
			return *ConnectorSectionMap.Find(SectionIndex);
		}*/
	}
	return nullptr;
}

// Called every frame
void AM_Connector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	ConnectorSectionMap.Reset();
	TArray<AActor*> Actors = GetCraft()->GetComponents();
	TArray<UConnectorBase*> Connectors;
	for (AActor* Actor : Actors)
	{
		Connectors.Append(UNoxelCombatLibrary::GetConnectorsFromActor(Actor));
	}
	FVector CameraPos, CameraDir;
	getRay(CameraPos, CameraDir);
	for (UConnectorBase* Connector : Connectors)
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
	}

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
	if (Alternate && SelectedConnector)
	{
		TArray<UConnectorBase*> Connectors;
		ConnectorSectionMap.GenerateValueArray(Connectors);
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
			//GetNoxelNetworkingAgent()->DisconnectConnector(SelectedConnector, SelectedConnector2);
			//UE_LOG(Noxel, Log, TEXT("Were connected"));
		}
		else if(UConnectorBase::CanBothConnect(SelectedConnector, SelectedConnector2))
		{
			FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
			queue->AddConnectorConnectOrder({SelectedConnector}, {SelectedConnector2});
			GetNoxelNetworkingAgent()->SendCommandQueue(queue);
			//DrawDebugLine(GetWorld(), SelectedConnector->GetComponentLocation(), SelectedConnector2->GetComponentLocation(), FColor::Blue, true);
			//UE_LOG(Noxel, Log, TEXT("[AM_Connector::leftClickReleased_Implementation] Connection made"));
			//GetNoxelNetworkingAgent()->ConnectConnector(SelectedConnector, SelectedConnector2);
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
		ConnectorSectionMap.GenerateValueArray(Connectors);
		for (UConnectorBase* ConnectorA : Connectors)
		{
			if (ConnectorA->bIsMale)
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