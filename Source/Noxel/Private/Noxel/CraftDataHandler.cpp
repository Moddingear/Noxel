//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Noxel/CraftDataHandler.h"
#include "Noxel.h"

#include "Net/UnrealNetwork.h"

#include "Noxel/NoxelDataComponent.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"
#include "Connectors/ConnectorBase.h"

#include "NObjects/NoxelPart.h"
#include "NObjects/NObjectInterface.h"
#include "NoxelDataAsset.h"
#include "RuntimeMesh.h"

#include "JsonObjectConverter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Noxel/NoxelLibrary.h"

//Initialization --------------------------------------------------------------------------------------------------------------------------------
UCraftDataHandler::UCraftDataHandler()
{
	SetIsReplicatedByDefault(true);
	SpawnContext = ECraftSpawnContext::None;
	Scale = 10.0f;
	static ConstructorHelpers::FObjectFinder<UDataTable> DataConstructor(OBJECTLIBRARY_PATH);
	if(DataConstructor.Succeeded()){
		DataTable = DataConstructor.Object;
	}
}

void UCraftDataHandler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Here we list the variables we want to replicate + a condition if wanted
	DOREPLIFETIME(UCraftDataHandler, Name);
	DOREPLIFETIME(UCraftDataHandler, Scale);
	DOREPLIFETIME(UCraftDataHandler, Components);
}

void UCraftDataHandler::BeginPlay(){
	Super::BeginPlay();
	//UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::BeginPlay] %s : %d components"), *GetFullName(), Components.Num());
}

void UCraftDataHandler::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	//kill all remaining components
	destroyCraft();

	Super::EndPlay(EndPlayReason);
}

bool UCraftDataHandler::AreAllComponentsValid()
{
	for (AActor* Actor : Components)
	{
		if (!IsValid(Actor))
		{
			return false;
		}
	}
	return true;
}

TArray<AActor*> UCraftDataHandler::GetComponents()
{
	return Components;
}

TArray<ANoxelPart*> UCraftDataHandler::GetParts()
{
	TArray<ANoxelPart*> Parts;
	for (AActor* Component : Components)
	{
		if (IsValid(Component) && Component->IsA<ANoxelPart>())
		{
			Parts.Add(Cast<ANoxelPart>(Component));
		}
	}
	return Parts;
}

bool UCraftDataHandler::HasAnyDataComponentConnected(AActor* Component)
{
	TArray<UNoxelDataComponent*> DataComps;
	Component->GetComponents<UNoxelDataComponent>(DataComps);
	for (UNoxelDataComponent* DataComp : DataComps)
	{
		if (DataComp->IsConnected())
		{
			return true;
		}
	}
	return false;
}

bool UCraftDataHandler::HasAnyConnectorConnected(AActor* Component)
{
	TArray<UConnectorBase*> connectors;
	Component->GetComponents<UConnectorBase>(connectors);
	for (UConnectorBase* Connector : connectors)
	{
		if (Connector->Connected.Num() >0) //TODO : Add function for this
		{
			return true;
		}
	}
	return false;
}

AActor* UCraftDataHandler::AddComponent(TSubclassOf<AActor> Class, FTransform Location, FActorSpawnParameters SpawnParameters, bool bSetSpawnContext, bool bFinishSpawning)
{
	if (!Class)
	{
		UE_LOG(Noxel, Error, TEXT("[UCraftDataHandler::AddComponent] Class is invalid"));
		return nullptr;
	}
	//UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::AddComponent] Spawning actor"));
	AActor* Spawned = GetWorld()->SpawnActorDeferred<AActor>(Class, Location, SpawnParameters.Owner, GetOwner()->GetInstigator(), SpawnParameters.SpawnCollisionHandlingOverride);
#ifdef WITH_EDITOR
	if (!Spawned->GetIsReplicated() || !Spawned->IsReplicatingMovement())
	{
		UE_LOG(Noxel, Error, TEXT("[UCraftDataHandler::AddComponent] Class %s is not set to replicate correctly"), *Class->GetName());
	}
#endif // WITH_EDITOR

	if (bSetSpawnContext)
	{
		TArray<UNoxelDataComponent*> containers;
		Spawned->GetComponents<UNoxelDataComponent>(containers);
		for (UNoxelDataComponent* container : containers)
		{
			container->SetSpawnContext(SpawnContext);
			if (container->IsA<UNodesContainer>())
			{
				//UNodesContainer* NodeContainer = Cast<UNodesContainer>(container);
				/*if (!nc->NodeSizeAbsolute)
				{
					nc->SetNodeSize(Scale);
				}*/
			}
		}
		//UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::AddComponent] Setting Spawn context : %d containers found"), containers.Num());
	}
	if (bFinishSpawning)
	{
		//UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::AddComponent] Actor finished spawning"));
		Spawned->FinishSpawning(Location);
	}

	//Spawned->AttachToActor(GetOwner(), FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
	Components.Emplace(Spawned);
	//UE_LOG(Noxel, Warning, TEXT("[UCraftDataHandler::AddComponent] Added one actor to the list : Length %d; Class %s"), Components.Num(), *Class->GetName());
	return Spawned;
}

bool UCraftDataHandler::RemoveComponent(AActor * Component)
{
	if (GetWorld()->IsServer()) 
	{
		if (Components.Contains(Component))
		{
			Components.Remove(Component);
			Component->Destroy();
			return true;
		}
	}
	return false;
}

AActor* UCraftDataHandler::AddComponentFromComponentID(FString ComponentID, FTransform Location)
{
	TSubclassOf<AActor> CompClass = UNoxelDataAsset::getClassFromComponentID(DataTable, ComponentID);
	return AddComponent(CompClass, Location, FActorSpawnParameters());
}

bool UCraftDataHandler::MoveComponent(AActor* Component, FTransform Location)
{
	if (!IsValid(Component))
	{
		return false;
	}
	if (HasAnyDataComponentConnected(Component))
	{
		return false;
	}
	if (GetWorld()->IsServer())
	{
		if (Components.Contains(Component))
		{
			Component->SetActorTransform(Location);
			return true;
		}
	}
	return false;
}

bool UCraftDataHandler::RemoveComponentIfUnconnected(AActor* Component)
{
	if (!IsValid(Component))
	{
		return false;
	}
	if (HasAnyDataComponentConnected(Component))
	{
		return false;
	}
	if (HasAnyConnectorConnected(Component))
	{
		return false;
	}
	return RemoveComponent(Component);
}

void UCraftDataHandler::SetNodesContainersVisibility(bool NewVisibility)
{
	for (int i = 0; i < Components.Num(); ++i)
	{
		if (!IsValid(Components[i]))
		{
			continue;
		}
		TArray<UNodesContainer*> containers;
		Components[i]->GetComponents<UNodesContainer>(containers);
		for (UNodesContainer* Container : containers)
		{
			Container->SetVisibility(NewVisibility, false);
			Container->SetCollisionEnabled(NewVisibility ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		}
	}
}

void UCraftDataHandler::SetNoxelContainersVisibility(bool NewVisibility)
{
	for (int i = 0; i < Components.Num(); ++i)
	{
		if (!IsValid(Components[i]))
		{
			continue;
		}
		TArray<UNoxelContainer*> containers;
		Components[i]->GetComponents<UNoxelContainer>(containers);
		for (UNoxelContainer* Container : containers)
		{
			Container->SetVisibility(NewVisibility, false);
			Container->SetCollisionEnabled(NewVisibility ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}
}

//Saving and loading --------------------------------------------------------------------------------------------------------------------------------
void UCraftDataHandler::destroyCraft()
{
	if(GetWorld()->IsServer())
	{
		for(int i = Components.Num()-1; i >= 0; i--)
		{
			AActor* comp = Components[i];
			Components.RemoveAt(i);
			if (IsValid(comp))
			{
				comp->Destroy();
			}
		}
	}
}

FCraftSave UCraftDataHandler::saveCraft()
{
	//UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::saveCraft] Starting save"));
	//UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::saveCraft] %s : %d components"), *GetFullName(), Components.Num());
	TArray<FComponentSave> SavedComponents;
	TMap <FNodeID, FNodeSavedRedirector> RedirectorMap;
	for (int i = 0; i < Components.Num(); i++) //Save all the nodes first to first construct the redirector map
	{
		if(!Components[i]){
			UE_LOG(NoxelData, Warning, TEXT("[UCraftDataHandler::saveCraft] Skipped an invalid component"));
			continue;
		}
		SavedComponents.Add(FComponentSave(Components[i]));
		SavedComponents[i].ComponentID = UNoxelDataAsset::getComponentIDFromClass(DataTable, Components[i]->GetClass());
		TArray<UNodesContainer*> containers;
		Components[i]->GetComponents<UNodesContainer>(containers);
		for (int j = 0; j < containers.Num(); j++)
		{
			FNodesContainerSave cn;
			saveNodesContainer(containers[j], i, j, cn, RedirectorMap);
			cn.ComponentName = containers[j]->GetName();
			SavedComponents[i].SavedNodes.Add(cn);
		}
	}
	
	for (int i = 0; i < Components.Num(); i++)
	{
		if(!Components[i]){
			continue;
		}

		//Savings noxels
		TArray<UNoxelContainer*> containers;
		Components[i]->GetComponents<UNoxelContainer>(containers);
		for (int j = 0; j < containers.Num(); j++)
		{
			FNoxelContainerSave cn;
			saveNoxelContainer(containers[j], RedirectorMap, cn);
			cn.ComponentName = containers[j]->GetName();
			SavedComponents[i].SavedNoxels.Add(cn);
		}

		//Saving connectors
		TArray<UConnectorBase*> connectors;
		Components[i]->GetComponents<UConnectorBase>(connectors);
		for (UConnectorBase* connector : connectors)
		{
			if (connector->bIsSender)
			{
				FConnectorSavedRedirector cn;
				saveConnector(connector, Components, cn);
				SavedComponents[i].SavedConnectors.Add(cn);
			}
		}

		if (Components[i]->IsA<UNObjectInterface>())
		{
			INObjectInterface* ComponentInterface = Cast<INObjectInterface>(Components[i]);
			SavedComponents[i].SavedMetadata = ComponentInterface->Execute_OnReadMetadata(Components[i], Components);
		}
	}

	FCraftSave Craft = FCraftSave(Name, Scale);
	Craft.Components = SavedComponents;
	return Craft;
}

void UCraftDataHandler::loadCraft(FCraftSave Craft, FTransform transform)
{
	//UE_LOG(NoxelData, Warning, TEXT("[CraftDataHandler] Loading craft, spawn context : %s"), *UFunctionLibrary::GetEnumValueAsString<ECraftSpawnContext>(FString("ECraftSpawnContext"), SpawnContext));
	if(!GetWorld()->IsServer()){
		UE_LOG(NoxelData, Warning, TEXT("[UCraftDataHandler::loadCraft] Loading aborted, client isn't server"));
		return;
	}
	destroyCraft();

	Name = Craft.CraftName;
	Scale = Craft.CraftScale;

	TMap<FNodeSavedRedirector, FNodeID> RedirectorMap;

	TArray<AActor*> TempComp;
	TArray<ANoxelPart*> Parts;
	//Spawning components and setting nodes
	for(int i = 0; i < Craft.Components.Num(); i++)
	{
		FComponentSave Comp = Craft.Components[i];
		TSubclassOf<AActor> CompClass = UNoxelDataAsset::getClassFromComponentID(DataTable, Comp.ComponentID);
		if (!CompClass) {
			UE_LOG(NoxelData, Error, TEXT("[UCraftDataHandler::loadCraft] Invalid class from component ID %s"), *Comp.ComponentID);
			continue;
		}
		FTransform savedTransform = Comp.ComponentLocation;
		savedTransform.SetScale3D(FVector::OneVector);
		FTransform finalTransform = UKismetMathLibrary::ComposeTransforms(transform, savedTransform);

		AActor* SpawnActor = AddComponent(CompClass, finalTransform, FActorSpawnParameters(), true, true); //Spawn part
		TempComp.Add(SpawnActor);
		if (SpawnActor->IsA<ANoxelPart>())
		{
			Parts.Add(Cast<ANoxelPart>(SpawnActor));
		}
		TArray<UNodesContainer*> containers;
		SpawnActor->GetComponents<UNodesContainer>(containers); 	//Get all nodes container

		for(int j = 0; j < containers.Num(); j++)
		{
			for(int k = 0; k < Comp.SavedNodes.Num(); k++)
			{
				if (Comp.SavedNodes[k].ComponentName == containers[j]->GetName()) {				//Load if name matches the saved
					FNodesContainerSave load = Comp.SavedNodes[k];
					loadNodesContainer(containers[j], i, k, load, RedirectorMap);	
					break;
				}
			}
		}
	}

	
	for (int i = 0; i < Craft.Components.Num(); i++)
	{
		FComponentSave comp = Craft.Components[i];
		if (!TempComp.IsValidIndex(i)) {
			continue;
		}
		if (!TempComp[i]) {
			continue;
		}

		//Setting noxels
		TArray<UNoxelContainer*> containers;
		TempComp[i]->GetComponents<UNoxelContainer>(containers); 								//Get all noxel container
		for (int j = 0; j < containers.Num(); j++)
		{
			for (int k = 0; k < comp.SavedNoxels.Num(); k++)
			{
				if (comp.SavedNoxels[k].ComponentName == containers[j]->GetName()) {			//Load if name matches the saved
					loadNoxelContainer(containers[j], RedirectorMap, comp.SavedNoxels[k]);
					break;
				}
			}
		}

		//Setting connectors
		TArray<UConnectorBase*> connectors;
		TempComp[i]->GetComponents<UConnectorBase>(connectors);
		for (FConnectorSavedRedirector save : comp.SavedConnectors)
		{
			for (UConnectorBase* connector : connectors)
			{
				if (save.ConnectorName == connector->GetName())
				{
					loadConnector(connector, TempComp, save);
					break;
				}
			}
		}

		if (TempComp[i]->GetClass()->ImplementsInterface(UNObjectInterface::StaticClass()))
		{
			INObjectInterface::Execute_OnWriteMetadata(TempComp[i], comp.SavedMetadata, TempComp);
		}
	}

	for (ANoxelPart* Part : Parts)
	{
		for (UNodesContainer* connected : Part->GetNoxelContainer()->GetConnectedNodesContainers())
		{
			AActor* NObject = connected->GetOwner();
			if (NObject != Part)
			{
				NObject->bNetUseOwnerRelevancy = true;
				NObject->SetOwner(Part);//TODO: should be the controller (server) instead
			}
		}
	}

	//UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::loadCraft] Broadcasting OnCraftLoadedEvent"));
	if (OnCraftLoadedEvent.IsBound())
	{
		OnCraftLoadedEvent.Broadcast();
	}
	else 
	{
		UE_LOG(Noxel, Warning, TEXT("[UCraftDataHandler::loadCraft] OnCraftLoadedEvent not bound"));
	}
	//UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::loadCraft] %s : %d components"), *GetFullName(), Components.Num());
}

void UCraftDataHandler::enableCraft()
{
	UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::enableCraft]"));
	TArray<ANoxelPart*> Parts;
	for (AActor* NObject : Components) //Enable components
	{
		if (NObject->GetClass()->ImplementsInterface(UNObjectInterface::StaticClass()))
		{
			UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::enableCraft] Calling Enable on %s"), *NObject->GetName());
			INObjectInterface::Execute_OnNObjectEnable(NObject, this);
		}
		if (NObject->IsA<ANoxelPart>())
		{
			Parts.Add(Cast<ANoxelPart>(NObject));
		}
	}

	for (ANoxelPart* Part : Parts) //Attach components together
	{
		TArray<AActor*> AlreadyConnected = {Part};
		UNoxelContainer* PartNoxel = Part->GetNoxelContainer();
		FTransform NoxelTransform = PartNoxel->GetComponentTransform();
		//PartNoxel->SetSimulatePhysics(true); //has to be simulating in order to be weldable
		for (UNodesContainer* NodeContainer : Part->GetNoxelContainer()->GetConnectedNodesContainers())
		{
			AActor* NObject = NodeContainer->GetOwner();
			FTransform ComTransform = NObject->GetTransform();
			FTransform DeltaTransform(NoxelTransform.ToMatrixWithScale().Inverse() * ComTransform.ToMatrixWithScale());
			UPrimitiveComponent* root = Cast<UPrimitiveComponent>(NObject->GetRootComponent());
			if (AlreadyConnected.Contains(NObject))
			{
				continue;
			}
			if (NObject->GetClass()->ImplementsInterface(UNObjectInterface::StaticClass()))
			{
				if (INObjectInterface::Execute_OnNObjectAttach(NObject, Part))
				{
					AlreadyConnected.Add(NObject);
					continue;
				}
			}
			//Update the UBodySetup to something valid to allow welding 
			PartNoxel->GetRuntimeMesh()->ForceCollisionUpdate();
			//NObject->AttachToActor(Part, FAttachmentTransformRules::KeepWorldTransform);
			FAttachmentTransformRules rules(EAttachmentRule::SnapToTarget, false);
			NObject->AttachToComponent(PartNoxel, rules);
            NObject->SetActorRelativeTransform(DeltaTransform);
			
			FString attachedTo = TEXT("nothing");
			if (root->GetAttachmentRoot())
			{
				attachedTo = root->GetAttachmentRoot()->GetName();
			}
			UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::enableCraft] Attaching %s to %s : IsWelded = %d, IsSimulating = %d, attached to %s"), *root->GetPathName(), *Part->GetName(), root->IsWelded(), root->IsSimulatingPhysics(), *attachedTo);
			AlreadyConnected.Add(NObject);
		}
		Part->NoxelSave = saveNoxelNetwork(PartNoxel);
	}
	for (AActor* NObject : Components)
	{
		if (NObject->GetAttachParentActor() == nullptr)
		{
			if (UPrimitiveComponent* rootPrimitive = Cast<UPrimitiveComponent>(NObject->GetRootComponent()))
			{
				rootPrimitive->SetSimulatePhysics(true);
			}
			/*else
			{
				UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::enableCraft] %s root is not a primitive"), *NObject->GetName());
			}*/
		}
		/*else 
		{
			UE_LOG(Noxel, Log, TEXT("[UCraftDataHandler::enableCraft] %s is attached"), *NObject->GetName());
		}*/
	}
}

FNoxelNetwork UCraftDataHandler::saveNoxelNetwork(UNoxelContainer * noxel)
{
	FNoxelNetwork save;
	save.Noxel = noxel; //Store ref to noxel
	save.NodesConnected = noxel->GetConnectedNodesContainers(); //Store ref to nodes connected to the noxel
	save.NodesSave.SetNum(save.NodesConnected.Num());
	save.RelativeTransforms.SetNum(save.NodesConnected.Num());
	TMap <FNodeID, FNodeSavedRedirector> redirmap;
	for (int i = 0; i < save.NodesConnected.Num(); i++)
	{
		AActor* owner = save.NodesConnected[i]->GetOwner();
		saveNodesContainer(save.NodesConnected[i], 0, i, save.NodesSave[i], redirmap); //Save nodes
		save.RelativeTransforms[i] = FTransform(noxel->GetOwner()->GetActorTransform().ToMatrixWithScale().Inverse() * owner->GetActorTransform().ToMatrixWithScale());
	}
	saveNoxelContainer(noxel, redirmap, save.NoxelSave); //Save noxel
	return save;
}

bool UCraftDataHandler::loadNoxelNetwork(FNoxelNetwork save)
{
	TMap<FNodeSavedRedirector, FNodeID> redirmap;
	for (UNodesContainer* Container : save.NodesConnected)
	{
		if (Container == nullptr)
		{
			return false;
		}
	}
	for (int i = 0; i < save.NodesConnected.Num(); i++)
	{
		loadNodesContainer(save.NodesConnected[i], 0, i, save.NodesSave[i], redirmap); //Load nodes while building the redirmap like it was deconstructed
	}
	loadNoxelContainer(save.Noxel, redirmap, save.NoxelSave); //Load noxel
	return true;
}

FNodesNetwork UCraftDataHandler::saveNodesNetwork(UNodesContainer * nodes)
{
	FNodesNetwork save;
	save.Nodes = nodes;
	TMap<FNodeID, FNodeSavedRedirector> RedirectorMap;
	saveNodesContainer(nodes, 0, 0, save.NodesSave, RedirectorMap);
	return save;
}

void UCraftDataHandler::loadNodesNetwork(FNodesNetwork save)
{
	TMap<FNodeSavedRedirector, FNodeID> RedirectorMap;
	loadNodesContainer(save.Nodes, 0, 0, save.NodesSave, RedirectorMap);
}

FString UCraftDataHandler::getCraftSaveLocation()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + TEXT("Crafts/");
}

TArray<FString> UCraftDataHandler::getSavedCrafts()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.CreateDirectoryTree(*getCraftSaveLocation())) 
	{
		FJsonSerializableArray files;
		PlatformFile.FindFiles(files, *getCraftSaveLocation(), TEXT(".json"));
		return files;
	}
	return TArray<FString>();
}

bool UCraftDataHandler::getCraftSave(FString path, FCraftSave & Save)
{
	FString text;
	if (FFileHelper::LoadFileToString(text, *path)) {
		return FJsonObjectConverter::JsonObjectStringToUStruct<FCraftSave>(text, &Save, 0, 0);
	}
	return false;
}

void UCraftDataHandler::setCraftSave(FString path, FCraftSave Save)
{
	FString text;
	FJsonObjectConverter::UStructToJsonObjectString<FCraftSave>(Save, text);
	FFileHelper::SaveStringToFile(text, *path);
}

FCraftSave UCraftDataHandler::GetDefaultCraftSave()
{
	FCraftSave save;
	save.CraftName = TEXT("");
	FComponentSave comp;
	comp.ComponentLocation = FTransform::Identity;
	comp.ComponentID = TEXT("Part");
	save.Components.Add(comp);
	return save;
}

// Serialise to JSON text for debug ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UCraftDataHandler::nodeSaveToText(FNodesContainerSave SavedData)
{
	FString string;
	FJsonObjectConverter::UStructToJsonObjectString<FNodesContainerSave>(SavedData, string);
	UE_LOG(NoxelData, Warning, TEXT("%s"), *string);
}

void UCraftDataHandler::noxelSaveToText(FNoxelContainerSave SavedData)
{
	FString string;
	FJsonObjectConverter::UStructToJsonObjectString<FNoxelContainerSave>(SavedData, string);
	UE_LOG(NoxelData, Warning, TEXT("%s"), *string);
}

void UCraftDataHandler::noxelNetworkToText(FNoxelNetwork SavedData)
{
	FString string;
	FJsonObjectConverter::UStructToJsonObjectString<FNoxelNetwork>(SavedData, string);
	UE_LOG(NoxelData, Warning, TEXT("%s"), *string);
}

void UCraftDataHandler::craftSaveToText(FCraftSave save)
{
	FString string;
	FJsonObjectConverter::UStructToJsonObjectString<FCraftSave>(save, string);
	UE_LOG(NoxelData, Warning, TEXT("%s"), *string);
}

void UCraftDataHandler::OnRep_Components()
{
	OnComponentsReplicatedEvent.Broadcast();
}

// Loading and saving of specific nodes / noxels --------------------------------------------------------------------------------------------------------------------------------

//Nodes ----------------------------------------------------------------
void UCraftDataHandler::saveNodesContainer(UNodesContainer * NodesContainer, int32 parentIndex, int32 nodesContainerIndex, FNodesContainerSave & SavedData, TMap<FNodeID, FNodeSavedRedirector>& RedirectorMap)
{
	SavedData = FNodesContainerSave(NodesContainer->GetName(), NodesContainer->GetNodeSize());

	TArray<FNodeID> Nodes = NodesContainer->GenerateNodesKeyArray();
	for (int i = 0; i < Nodes.Num(); i++)
	{
		if (NodesContainer->IsPlayerEditable())
		{
			SavedData.Nodes.Add(Nodes[i].Location);
		}
		FNodeSavedRedirector Redir = FNodeSavedRedirector(parentIndex, nodesContainerIndex, i);
		RedirectorMap.Add(Nodes[i], Redir);
	}
}

bool UCraftDataHandler::loadNodesContainer(UNodesContainer * NodesContainer, int32 parentIndex, int32 nodesContainerIndex, FNodesContainerSave SavedData, TMap<FNodeSavedRedirector, FNodeID>& RedirectorMap)
{
	if (!NodesContainer)
	{
		return false;
	}
	if (NodesContainer->IsPlayerEditable()) {
		//Remove all present nodes
		TArray<FNodeID> OldNodes = NodesContainer->GenerateNodesKeyArray();
		for (int i = 0; i > OldNodes.Num(); i++)
		{
			NodesContainer->RemoveNode(OldNodes[i]);
		}
		//UE_LOG(NoxelData, Log, TEXT("Loading with node size %f"), SavedData.NodeSize);
		NodesContainer->SetNodeSize(SavedData.NodeSize);
		//Add nodes from save and build redirector map
		for (int i = 0; i < SavedData.Nodes.Num(); i++)
		{
			FNodeID NewNode = FNodeID(NodesContainer, SavedData.Nodes[i]);
			NodesContainer->AddNode(NewNode);
			RedirectorMap.Add(FNodeSavedRedirector(parentIndex, nodesContainerIndex, i), NewNode);
		}
	}
	else {
		//Build redirector map by finding the nodes
		/*for (int i = 0; i < SavedData.Nodes.Num(); i++)
		{
			FNodeID NewNode;
			
			if (NodesContainer->FindNode(SavedData.Nodes[i], NewNode)) {
				RedirectorMap.Add(FNodeSavedRedirector(parentIndex, nodesContainerIndex, i), NewNode);
			}
		}*/
		//assume stability in order of nodes across clients for non editable objects
		TArray<FNodeID> Nodes = NodesContainer->GenerateNodesKeyArray();
		for (int i = 0; i < Nodes.Num(); i++)
		{
			FNodeSavedRedirector Redir = FNodeSavedRedirector(parentIndex, nodesContainerIndex, i);
			RedirectorMap.Add(Redir, Nodes[i]);
		}
	}
	return true;
}

//Noxel ----------------------------------------------------------------
void UCraftDataHandler::saveNoxelContainer(UNoxelContainer * NoxelContainer, TMap<FNodeID, FNodeSavedRedirector>& RedirectorMap, FNoxelContainerSave & SavedData)
{
	SavedData = FNoxelContainerSave(NoxelContainer->GetName());

	TArray<FPanelData> Panels = NoxelContainer->GetPanels();
	for (int i = 0; i < Panels.Num(); i++)
	{
		FPanelData data = Panels[i];
		FPanelSavedData panel = FPanelSavedData(data);
		for (FNodeID node : data.Nodes)
		{
			if (RedirectorMap.Contains(node)) {
				panel.Nodes.Add(*RedirectorMap.Find(node));
			}
			else {
				UE_LOG(NoxelData, Error, TEXT("Redirector map is missing a node"));
			}
		}
		SavedData.Panels.Add(panel);
	}
}

bool UCraftDataHandler::loadNoxelContainer(UNoxelContainer * NoxelContainer, TMap<FNodeSavedRedirector, FNodeID>& RedirectorMap, FNoxelContainerSave SavedData)
{
	if (!NoxelContainer)
	{
		return false;
	}
	//Delete all old panels
	TArray<FPanelData> OldPanels = NoxelContainer->GetPanels();
	for (int i = 0; i < OldPanels.Num(); i++)
	{
		NoxelContainer->RemovePanel(OldPanels[i].PanelIndex);
	}

	for (int i = 0; i < SavedData.Panels.Num(); i++)
	{
		//Rebuild the panel data
		FPanelSavedData SData = SavedData.Panels[i];
		TArray<FNodeID> nodes;
		for (int j = 0; j < SData.Nodes.Num(); j++)
		{
			if (RedirectorMap.Contains(SData.Nodes[j])) {
				nodes.Add(*RedirectorMap.Find(SData.Nodes[j]));
			}
		}
		const FPanelData data = FPanelData(nodes, SData.ThicknessNormal, SData.ThicknessAntiNormal, SData.Virtual);

		NoxelContainer->AddPanel(data);
	}
	NoxelContainer->UpdateMesh();
	return true;
}

//Connectors -----------------------------------------------------------
void UCraftDataHandler::saveConnector(UConnectorBase * Connector, TArray<AActor*>& Components, FConnectorSavedRedirector & SavedData)
{
	if (!Connector->bIsSender)
	{
		return;
	}
	SavedData.ConnectorName = Connector->GetName();
	SavedData.parentIndex.Empty(Connector->Connected.Num());
	SavedData.OtherConnectorName.Empty(Connector->Connected.Num());
	for (UConnectorBase* OtherConnector : Connector->Connected)
	{
		SavedData.parentIndex.Add(Components.Find(OtherConnector->GetOwner()));
		SavedData.OtherConnectorName.Add(OtherConnector->GetName());
	}
}

void UCraftDataHandler::loadConnector(UConnectorBase * Connector, TArray<AActor*>& Components, FConnectorSavedRedirector SavedData)
{
	for (int32 SaveIdx = 0; SaveIdx < SavedData.parentIndex.Num(); SaveIdx++)
	{
		AActor* Parent = Components[SavedData.parentIndex[SaveIdx]];
		FString ConnectorName = SavedData.OtherConnectorName[SaveIdx];
		TArray<UConnectorBase*> connectors;
		Parent->GetComponents<UConnectorBase>(connectors);
		for (UConnectorBase* OtherConnector : connectors)	
		{
			if (ConnectorName == OtherConnector->GetName())
			{
				Connector->Connect(OtherConnector);
				break;
			}
		}
	}
}

