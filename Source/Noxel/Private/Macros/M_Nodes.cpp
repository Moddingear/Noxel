//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_Nodes.h"
#include "Noxel/NoxelCombatLibrary.h"
#include "Noxel/NodesContainer.h"
#include "EditorCharacter.h"
#include "Components/WidgetInteractionComponent.h"

AM_Nodes::AM_Nodes() 
{
	AlternationMethod = EAlternateType::Hold;
}

// Called when the game starts
void AM_Nodes::BeginPlay()
{
	Super::BeginPlay();
	//UE_LOG(NoxelMacro, Warning, TEXT("Macro added"));
}

// Called every frame
void AM_Nodes::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (false/*TransformGizmo*/) 
	{
		FVector Position, Direction;
		getRay(Position, Direction);
		//FTransform DeltaMove = TransformGizmo->UpdateCursor(Position, Direction);
		//FTransform Base = TransformGizmo->getGizmoBaseLocation();
		for (FNodeID node : selectedNodes)
		{
			//DrawNode((DeltaMove * Base).TransformPosition(Base.InverseTransformPosition(node.toWorld())), 2*node.Object->NodeSize);
		}
		/*switch (TransformGizmo->GetTransformType())
		{
		case EGizmoTransformType::Translate:

			break;
		case EGizmoTransformType::Rotate:
			break;
		default:
			break;
		}*/

		if (!Alternate) 
		{
			LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "GizmoMove", "Move axis");
			RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "GizmoConfirm", "Confirm");
		}
		else 
		{
			/*switch (TransformGizmo->GetRelativeMode())
			{
			case EGizmoRelativeMode::Absolute:
				LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "GizmoRelativeMode", "Set axis to Relative");
				break;
			case EGizmoRelativeMode::Relative:
				LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "GizmoRelativeBaseMode", "Set axis Relative to start");
				break;
			case EGizmoRelativeMode::RelativeBase:
				LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "GizmoAbsoluteMode", "Set axis to Absolute");
				break;
			default:
				break;
			}
			switch (TransformGizmo->GetTransformType())
			{
			case EGizmoTransformType::Translate:
				RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "GizmoRotate", "Set axis to rotate");
				break;
			case EGizmoTransformType::Rotate:
				RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "GizmoTranslate", "Set axis to translate");
				break;
			case EGizmoTransformType::Scale:
				break;
			default:
				break;
			}*/
		}
	}
	else 
	{
		if (selectedNodes.Num() >= 3) 
		{
			DrawPanel(FPanelData(selectedNodes, 1.0f, false));
		}
		else if (!GetOwningActor()->GetInteractionWidget()->IsOverInteractableWidget())
		{
			FVector LocationRelative = getNodePlacement(placementDistance, getSelectedNodesContainer());
			DrawNode(getSelectedNodesContainer()->GetComponentTransform().TransformPosition(LocationRelative));
		}

		if (Alternate) {
			LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "SelectNodes", "Select Nodes");
			if (selectedNodes.Num() > 0) {
				RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "MoveNodes", "Move nodes");
			}
			else {
				RightClickHint = FText();
			}
		}
		else {
			if (selectedNodes.Num() >= 3) { //If a panel can be added
				LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "AddPanel", "Add panel");
			}
			else {
				LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "AddNode", "Add node");
			}

			if (selectedNodes.Num() >= 1) { //If nodes are selected
				RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "RemoveSelected", "Remove selected");
			}
			else {
				RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "RemoveTargeted", "Remove targeted");
			}
		}
	}
	
}

void AM_Nodes::leftClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Left click"));
	if (false/*TransformGizmo*/)
	{
		/*if (Alternate)
		{
			EGizmoRelativeMode newMode = EGizmoRelativeMode::Relative;
			switch (TransformGizmo->GetRelativeMode())
			{
			case EGizmoRelativeMode::Absolute:
				newMode = EGizmoRelativeMode::Relative;
				break;
			case EGizmoRelativeMode::Relative:
				newMode = EGizmoRelativeMode::RelativeBase;
				break;
			case EGizmoRelativeMode::RelativeBase:
				newMode = EGizmoRelativeMode::Absolute;
				break;
			default:
				break;
			}
			TransformGizmo->SetRelativeMode(newMode);
		}
		else
		{
			FVector Position, Direction;
			getRay(Position, Direction);
			TransformGizmo->CaptureCursor(Position, Direction);
		}*/
	}
	else
	{
		if (!Alternate) { //Adding nodes or panels
			if (selectedNodes.Num() < 3) { //If a panel can't be created
				FNodeID id;
				id.Location = getNodePlacement(placementDistance, getSelectedNodesContainer());
				id.Object = getSelectedNodesContainer();
				//networkingAgent->AddNode(id);
			}
			else {
				//networkingAgent->AddPanel(FPanelID(getSelectedNoxelContainer()), FPanelData(selectedNodes, 1.0f, false));
				//if (getSelectedNoxelContainer()->Panels.Num() >= 2) {
					//TArray<FPanelID> keys;
					//getSelectedNoxelContainer()->Panels.GetKeys(keys);
					//bool result = UNoxelCombatLibrary::isPanelConnected(keys[0], keys[keys.Num() - 1]);
					//UE_LOG(NoxelMacro, Warning, TEXT("AStar : %s"), (result ? TEXT("True") : TEXT("False")));
				//}
			}
			resetNodesColor(); //Clear all selected node
			selectedNodes.Empty();
		}
		else { //Selecting
			FVector location, direction, end;
			getRay(location, direction);
			end = location + direction * ray_length;
			FNodeID id;
			if (traceNodes(location, end, id)) {
				//UE_LOG(NoxelMacro, Warning, TEXT("Trace sucessful"));
				if (!selectedNodes.Contains(id)) { //If the node wasn't already selected
					setNodeColor(id, ENoxelColor::NodeSelected1); //Set the color of the node
					selectedNodes.Add(id);
				}
				else {
					setNodeColor(id, ENoxelColor::NodeInactive); //Set the color of the node
					selectedNodes.Remove(id);
				}
			}
		}
	}
}

void AM_Nodes::leftClickReleased_Implementation()
{
	/*if(TransformGizmo)
	{
		FVector Position, Direction;
		getRay(Position, Direction);
		TransformGizmo->ReleaseCursor(Position, Direction);
	}*/
}

void AM_Nodes::middleClickPressed_Implementation()
{
}

void AM_Nodes::rightClickPressed_Implementation()
{
	UE_LOG(NoxelMacro, Log, TEXT("Right click"));

	if (false/*TransformGizmo*/) 
	{
		/*if (Alternate)
		{
			TransformGizmo->SetTransformType(TransformGizmo->GetTransformType() == EGizmoTransformType::Translate ? EGizmoTransformType::Rotate : EGizmoTransformType::Translate);
		}
		else 
		{
			FTransform DeltaMove = TransformGizmo->getGizmoDeltaMove();
			FTransform Base = TransformGizmo->getGizmoBaseLocation();
			for (FNodeID node : selectedNodes)
			{
				FVector newpos = node.Object->GetComponentTransform().InverseTransformPosition((DeltaMove * Base).TransformPosition(Base.InverseTransformPosition(node.toWorld())));
				networkingAgent->MoveNode(node, newpos);
			}
			selectedNodes.Empty();
			DestroyTransformGizmo();
		}*/
	}
	else 
	{
		if (!Alternate) { //Remove nodes or panels
			FNodeID node;
			FPanelID panel;
			FVector location, direction, end;
			getRay(location, direction);
			end = location + direction * ray_length;
			if (selectedNodes.Num() > 0) {
				resetNodesColor();
				for (int i = 0; i < selectedNodes.Num(); i++)
				{
					//UE_LOG(NoxelMacro, Warning, TEXT("Asking to remove selected node, macro"));
					//networkingAgent->RemoveNode(selectedNodes[i]);
				}
				selectedNodes.Empty();
			}
			else if (traceNodes(location, end, node))
			{
				//UE_LOG(NoxelMacro, Warning, TEXT("Asking to remove node, macro %s"), *node.Location.ToString());
				//networkingAgent->RemoveNode(node);
			}
			else if (tracePanels(location, end, panel))
			{
				//networkingAgent->RemovePanel(panel);
			}
		}
		else { //Move nodes
			if (selectedNodes.Num() > 0) {
				FVector Center = FVector::ZeroVector;
				for (FNodeID node : selectedNodes)
				{
					Center += node.ToWorld() / selectedNodes.Num();
				}
				//CreateTransformGizmo(FTransform(Center));
			}
		}
	}
}
