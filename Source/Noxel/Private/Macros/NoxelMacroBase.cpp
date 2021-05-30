//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/NoxelMacroBase.h"

#include "CoreMinimal.h"


#include "EditorCharacter.h"
#include "NoxelHangarBase.h"

#include "Noxel/NoxelDataStructs.h"
#include "Noxel/NodesContainer.h"
#include "Noxel/NoxelContainer.h"

#include "NObjects/NoxelPart.h"

#include "Voxel/VoxelComponent.h"

#include "Noxel.h"

DEFINE_LOG_CATEGORY(NoxelMacro);

// Sets default values for this component's properties
ANoxelMacroBase::ANoxelMacroBase()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> NodeMeshConstructor(TEXT("StaticMesh'/Game/Meshes/Nodes/Octahedron.Octahedron'"));
	//if (NodeMeshConstructor.Succeeded()) { NodeMeshStatic = NodeMeshConstructor.Object; }
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>NodeMaterialConstructor(TEXT("Material'/Game/Materials/Noxel/PlasticWireframe.PlasticWireframe'"));
	//if (NodeMaterialConstructor.Succeeded()) { NodeMeshMaterial = NodeMaterialConstructor.Object; }
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>PanelValidMaterialConstructor(TEXT("Material'/Game/Materials/Noxel/PanelPreview.PanelPreview'"));
	//if (PanelValidMaterialConstructor.Succeeded()) { PanelValidMaterial = PanelValidMaterialConstructor.Object; }
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>PanelInvalidMaterialConstructor(TEXT("MaterialInstanceConstant'/Game/Materials/Noxel/PanelObstructed.PanelObstructed'"));
	//if (PanelInvalidMaterialConstructor.Succeeded()) { PanelInvalidMaterial = PanelInvalidMaterialConstructor.Object; }

}


// Called when the game starts
void ANoxelMacroBase::BeginPlay()
{
	Super::BeginPlay();

	//Get the owner and it's networking agent
	owningActor = (AEditorCharacter*)GetOwner();
	if (owningActor == nullptr) 
	{ 
		UE_LOG(NoxelMacro, Error, TEXT("[ANoxelMacroBase::BeginPlay] Macro couldn't find owner")); 
	}
	else 
	{ 
		//if (!owningActor->IsLocallyControlled())//Kill the macro to save resources on server if not locally controlled
		//{
		//	return;
		//}
		//networkingAgent = ((ANoxelPlayerController*)UGameplayStatics::GetPlayerController(GetWorld(), 0))->NetworkingAgent;
	}

	//Tick after actor so that the location of the camera is final and not one tick late
	AddTickPrerequisiteActor(owningActor);

	/*if (NodeMeshStatic) {
		NodeMesh = FMeshData();
		URuntimeMeshLibrary::GetStaticMeshSection<FVertexNoxel, int32>(NodeMeshStatic, 0, 0, NodeMesh.Vertices, NodeMesh.Triangles);
		FColor color = UFunctionLibrary::getColorFromJson(ENoxelColor::NodeInactive).ToFColor(false);
		for (int32 i = 0; i < NodeMesh.Vertices.Num(); i++)
		{
			NodeMesh.Vertices[i].Position *= 1.0f / NODEMESHSCALE;
			NodeMesh.Vertices[i].Color = color;
		}
	}*/

	/*if (networkingAgent)
	{
		UE_LOG(NoxelMacro, Log, TEXT("NetworkingAgent is valid"));
		if (networkingAgent->Craft)
		{
			UE_LOG(NoxelMacro, Log, TEXT("NetworkingAgent's craft is valid"));
		}
	}
	if (owningActor)
	{
		UE_LOG(NoxelMacro, Log, TEXT("OwningActor is valid"));
		if (owningActor->CurrentPart)
		{
			UE_LOG(NoxelMacro, Log, TEXT("OwningActor's current part is valid"));
		}
		if (owningActor->Craft)
		{
			UE_LOG(NoxelMacro, Log, TEXT("OwningActor's craft is valid"));
		}
	}*/
}

void ANoxelMacroBase::EndPlay(const EEndPlayReason::Type EndPlayReason){
	resetNodesColor();
	//DestroyTransformGizmo();
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ANoxelMacroBase::Tick(float DeltaTime)
{
	ClearDraws();
	Super::Tick(DeltaTime);
}

AEditorCharacter * ANoxelMacroBase::GetOwningActor() const
{
	return owningActor;
}

void ANoxelMacroBase::SetOwningActor(AEditorCharacter * InOwningActor)
{
	owningActor = InOwningActor;
}

UNoxelNetworkingAgent * ANoxelMacroBase::GetNoxelNetworkingAgent() const
{
	AEditorCharacter* tempOwning = GetOwningActor();
	if (tempOwning)
	{
		return tempOwning->GetNetworkingAgent();
	}
	return nullptr;
}

ANoxelHangarBase * ANoxelMacroBase::GetHangar() const
{
	AEditorCharacter* tempOwning = GetOwningActor();
	if (tempOwning)
	{
		return tempOwning->GetHangar();
	}
	return nullptr;
}

UCraftDataHandler * ANoxelMacroBase::GetCraft() const
{
	ANoxelHangarBase* tempHangar = GetHangar();
	if (tempHangar)
	{
		return tempHangar->GetCraftDataHandler();
	}
	return nullptr;
}

ANoxelPart * ANoxelMacroBase::GetCurrentPart() const
{
	AEditorCharacter* tempOwning = GetOwningActor();
	if (tempOwning)
	{
		return tempOwning->GetCurrentPart();
	}
	return nullptr;
}

//General trace utilities --------------------------------------------------------------------------------------------------------------------------------

void ANoxelMacroBase::getRay(FVector & Location, FVector & Direction)
{
	FTransform comptransf = GetActorTransform();
	Location = comptransf.GetLocation();
	Direction = comptransf.GetRotation().GetForwardVector();
}

void ANoxelMacroBase::getTrace(FVector & start, FVector & end)
{
	FVector dir;
	getRay(start, dir);
	end = start + dir * ray_length;
}

// Node placement utilities ----------------------------------------------------------------

FVector ANoxelMacroBase::getNodePlacement(float PlacementDistance, UNodesContainer* Container)
{
	if(!Container){
		Container = getSelectedNodesContainer();
	}
	FVector location, direction;
	getRay(location, direction);
	FIntVector cube_hit, hit_normal;
	if (GetVoxel() && GetVoxel()->trace(location, location + direction * PlacementDistance*2, cube_hit, hit_normal)) {
		return FNodeID::FromWorld(Container, GetVoxel()->voxelToWorld(cube_hit));
	}
	return FNodeID::FromWorld(Container, location + direction * PlacementDistance);
}

// Nodes colors --------------------------------------------------------------------------------------------------------------------------------

void ANoxelMacroBase::setNodeColor(FNodeID id, ENoxelColor color)
{
	if (id.Object->SetNodeColor(id.Location, UFunctionLibrary::getColorFromJson(color))) 
	{
		if (color != ENoxelColor::NodeInactive) {
			ColoredNodes.AddUnique(id);
		}
		else {
			ColoredNodes.Remove(id);
		}
	}
	else 
	{
		ColoredNodes.Remove(id);
	}
}

void ANoxelMacroBase::resetNodesColor()
{
	for (int32 i = ColoredNodes.Num() -1; i>= 0; i--)
	{
		setNodeColor(ColoredNodes[i], ENoxelColor::NodeInactive);
	}
}

// Getting actors --------------------------------------------------------------------------------------------------------------------------------

UNodesContainer* ANoxelMacroBase::getSelectedNodesContainer()
{
	ANoxelPart* part = GetCurrentPart();
	if (part) {
		return part->GetNodesContainer();
	}
	UE_LOG(NoxelMacro, Warning, TEXT("[ANoxelMacroBase::getSelectedNodesContainer] Part not found"));
	return nullptr;
}

UNoxelContainer* ANoxelMacroBase::getSelectedNoxelContainer()
{
	ANoxelPart* part = GetCurrentPart();
	if (part) {
		return part->GetNoxelContainer();
	}
	return nullptr;
}

void ANoxelMacroBase::switchMacro(TSubclassOf<class ANoxelMacroBase> macro)
{
	owningActor->SetMacro(macro);
}

UVoxelComponent * ANoxelMacroBase::GetVoxel()
{
	ANoxelHangarBase* tempHangar = GetHangar();
	if (tempHangar) {
		return tempHangar->GetVoxel();
	}
	UE_LOG(NoxelMacro, Warning, TEXT("[ANoxelMacroBase::GetVoxel] Hangar not found"));
	return nullptr;
}

// Trace --------------------------------------------------------------------------------------------------------------------------------

bool ANoxelMacroBase::tracePart(FVector start, FVector end, ANoxelPart* & Part)
{
	FHitResult hit;
	if (GetWorld()->LineTraceSingleByChannel(hit, start, end, ECollisionChannel::ECC_GameTraceChannel1)) { //If something was hit in noxel collision channel
		if (hit.Actor->IsA<ANoxelPart>()) {
			Part = Cast<ANoxelPart>(hit.Actor.Get());
			return true;
		}
	}
	return false;
}

bool ANoxelMacroBase::traceNodes(FVector start, FVector end, FNodeID & id)
{
	FHitResult hit;
	if (GetWorld()->LineTraceSingleByChannel(hit, start, end, ECollisionChannel::ECC_GameTraceChannel1, UNoxelLibrary::getCollisionParameters())) { //If something was hit in noxel collision channel
		FNodeID node; //TODO
		if (UNodesContainer::GetNodeHit(hit, node)) {
			id = node;
			//UE_LOG(NoxelMacro, Warning, TEXT("Node was hit : %s"), *id.Location.ToString());
			return true;
		}
	}
	return false;
}

bool ANoxelMacroBase::tracePanels(FVector start, FVector end, FPanelID & id)
{
	FHitResult hit;
	if (GetWorld()->LineTraceSingleByChannel(hit, start, end, ECollisionChannel::ECC_GameTraceChannel1, UNoxelLibrary::getCollisionParameters())) { //If something was hit in noxel collision channel
		return UNoxelContainer::GetPanelHit(hit, id);
	}
	return false;
}

/*void ANoxelMacroBase::CreateTransformGizmo(FTransform position)
{
	if (!TransformGizmo) 
	{
		//UE_LOG(NoxelMacro, Log, TEXT("Adding transform gizmo"));
		TransformGizmo = NewObject<UTransformGizmo>(GetOwner(), UTransformGizmo::StaticClass());
		TransformGizmo->SetGizmoBaseLocation(position);
		//TransformGizmo->AttachToComponent(this, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
		TransformGizmo->RegisterComponent();
	}
}

void ANoxelMacroBase::DestroyTransformGizmo()
{
	if (TransformGizmo) 
	{
		TransformGizmo->DestroyComponent();
		TransformGizmo = NULL;
	}
}*/

void ANoxelMacroBase::DrawNode(FVector WorldPosition, float size)
{
	/*if (!NodeMeshMaterial || !NodeMeshStatic) { return; }
	FTransform ComponentTransform = GetComponentTransform();
	FVector PositionDelta = ComponentTransform.InverseTransformPosition(WorldPosition);
	FMeshData display = FMeshData(NodeMesh);
	for (int32 i = 0; i < display.Vertices.Num(); i++)
	{
		display.Vertices[i].Position = ComponentTransform.Rotator().UnrotateVector(display.Vertices[i].Position * size)  + PositionDelta;
		//UE_LOG(NoxelMacro, Log, TEXT("Vert %d, position : %s"), i, *display.Vertices[i].Position.ToString());
	}
	//MakeWireframe(display);
	//UE_LOG(NoxelMacro, Log, TEXT("[Macro] Drawing node with %d verts"), display.Vertices.Num());
	SetMeshSection<FVertexNoxel, int32>(GetNumSections(), display.Vertices, display.Triangles, false, EUpdateFrequency::Frequent);
	SetMaterial(GetNumSections() - 1, NodeMeshMaterial);*/
}

void ANoxelMacroBase::DrawPanel(FPanelData PanelData)
{
	/*EPanelError errorcode;
	FPanelData newData;
	FPanelID PanelID = FPanelID(getSelectedNoxelContainer());
	UMaterialInterface* material;
	if (UNoxelLibrary::CheckPanelValidity(PanelID, PanelData, true, errorcode, newData)) //Panel is valid
	{
		if (!PanelValidMaterial) { return; }
		material = PanelValidMaterial;
	}
	else //Panel is invalid
	{ 
		if (!PanelInvalidMaterial) { return; }
		if (errorcode == EPanelError::Not_enough_nodes || errorcode == EPanelError::ID_redundant || errorcode == EPanelError::Nodes_invalid)  //If the panel data wasn't updated
		{ 
			newData = UNoxelLibrary::ComputeProperties(UNoxelLibrary::getNoxelTransform(PanelID), PanelData, true, false);
		}
		material = PanelInvalidMaterial;
	}
	FMeshData display = UNoxelLibrary::RenderPanel(PanelID, newData).LOD[0];
	for (int32 i = 0; i < display.Vertices.Num(); i++)
	{
		display.Vertices[i].Position = GetComponentTransform().InverseTransformPosition(PanelID.Object->GetComponentTransform().TransformPosition(display.Vertices[i].Position));
	}
	CreateMeshSection<FVertexNoxel, int32>(GetNumSections(), display.Vertices, display.Triangles, false, EUpdateFrequency::Frequent);
	SetMaterial(GetNumSections() - 1, material);*/
}

void ANoxelMacroBase::DrawLine(FVector Start, FVector End, float thickness)
{
	//UE_LOG(NoxelMacro, Log, TEXT("[ANoxelMacroBase::DrawLine] TODO"));
	/*FVector middle = (Start + End) / 2;
	float sizeX = (End - Start).Size() / 2;
	float sizeY = thickness / 2;
	FVector scale = FVector(sizeX, sizeY, 1.0f);
	FVector CameraPos, CameraDir;
	getRay(CameraPos, CameraDir);
	FVector Normal = CameraPos - UKismetMathLibrary::FindClosestPointOnLine(CameraPos, Start, (End - Start).GetUnsafeNormal());
	FRotator RotationWorld = UKismetMathLibrary::MakeRotFromZX(Normal.GetUnsafeNormal(), (End-Start).GetUnsafeNormal());
	DrawPlaneMaterial(FTransform(RotationWorld, middle, scale), nullptr);*/

	/*FVector CameraPos, CameraDir;
	getRay(CameraPos, CameraDir);
	FVector Normal = CameraPos - UKismetMathLibrary::FindClosestPointOnLine(CameraPos, Start, (End - Start).GetUnsafeNormal());
	FRotator RotationWorld = UKismetMathLibrary::MakeRotFromZY(Normal.GetUnsafeNormal(), GetUpVector());
	FTransform ComponentTransform = GetComponentTransform();
	FRotator RotationLocal = (ComponentTransform.Inverse() * FTransform(RotationWorld)).Rotator;
	FVector LocalStart = ComponentTransform.InverseTransformPosition(Start);
	FVector LocalEnd = ComponentTransform.InverseTransformPosition(End);
	FVector Y = FTransform(RotationLocal).GetUnitAxis(EAxis::Y) * thickness /2;

	Normal.Normalize();
	TArray<FVector> Verts = { LocalStart + Y, LocalStart - Y, LocalEnd + Y, LocalEnd - Y};
	const TArray<int32> Tris = { 0,1,2,2,1,3 };
	const TArray<FVector> Normals = { Normal,Normal,Normal,Normal };
	const TArray<FVector2D> UV = { FVector2D(0.f,0.f),FVector2D(0.f,1.f),FVector2D(1.f,0.f),FVector2D(1.f,1.f) };

	int32 sectionIdx = GetNumSections();
	SetMeshSection(sectionIdx, Verts, Tris, Normals, UV, TArray<FColor>(), TArray<FRuntimeMeshTangent>(), false);*/
}

void ANoxelMacroBase::DrawPlane(FVector Position, FVector Normal)
{
	UE_LOG(NoxelMacro, Log, TEXT("[ANoxelMacroBase::DrawPlane] TODO"));
}

int32 ANoxelMacroBase::DrawPlaneMaterial(FTransform LocationWorld, UMaterialInterface* Material, bool bCreateComplexCollision)
{
	/*FTransform LocationRelative = LocationWorld * GetComponentTransform().Inverse(); //Plane->World * World->Local = Plane->Local
	FVector X = LocationRelative.GetScaledAxis(EAxis::X);
	FVector Y = LocationRelative.GetScaledAxis(EAxis::Y);
	FVector Normal = LocationRelative.GetUnitAxis(EAxis::Z);
	FVector RelPos = LocationRelative.GetLocation();
	TArray<FVector> Verts = {X+Y+RelPos, X-Y+RelPos, -X+Y+RelPos, -X-Y+RelPos};
	const TArray<int32> Tris = {0,1,2,2,1,3};
	const TArray<FVector> Normals = {Normal,Normal,Normal,Normal};
	const TArray<FVector2D> UV = {FVector2D(0.f,0.f),FVector2D(0.f,1.f),FVector2D(1.f,0.f),FVector2D(1.f,1.f)};
	int32 sectionIdx = GetNumSections();
	SetMeshSection(sectionIdx, Verts, Tris, Normals, UV, TArray<FColor>(), TArray<FRuntimeMeshTangent>(), bCreateComplexCollision);
	SetMaterial(sectionIdx, Material);
	return sectionIdx;*/
	return INDEX_NONE;
}

void ANoxelMacroBase::ClearDraws()
{
	//ClearAllMeshSections();
}

/*void ANoxelMacroBase::MakeWireframe(FMeshData & mesh)
{
	const TArray<FVector2D> UVs = TArray<FVector2D>({ FVector2D(0,0), FVector2D(1,0), FVector2D(0,1) });
	for (int32 i = 0; i < mesh.Vertices.Num(); i++)
	{
		mesh.Vertices[i].UV0 = UVs[i%UVs.Num()];
	}
}*/

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
// Mouse Events ------------------------------------------------
////////////////////////////////////////////////////////////////

void ANoxelMacroBase::leftClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Warning, TEXT("Left click not overriden by macro"));
}

void ANoxelMacroBase::middleClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Warning, TEXT("Middle click not overriden by macro"));
}

void ANoxelMacroBase::rightClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Warning, TEXT("Right click not overriden by macro"));
}

void ANoxelMacroBase::leftClickReleased_Implementation()
{
}

void ANoxelMacroBase::middleClickReleased_Implementation()
{
}

void ANoxelMacroBase::rightClickReleased_Implementation()
{
}

void ANoxelMacroBase::alternateModePressed_Implementation()
{
	switch (AlternationMethod)
	{
	case EAlternateType::Hold:
		Alternate = true;
		break;
	case EAlternateType::Switch:
		Alternate = !Alternate;
		break;
	default:
		break;
	}
}

void ANoxelMacroBase::alternateModeReleased_Implementation()
{
	if (AlternationMethod == EAlternateType::Hold) {
		Alternate = false;
	}
}

