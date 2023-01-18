//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/M_Nodes.h"
#include "Noxel/NoxelCombatLibrary.h"
#include "Noxel/NodesContainer.h"
#include "EditorCharacter.h"
#include "Noxel/NoxelNetworkingAgent.h"
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
	if (!IsValid(GetOwningActor()))
	{
		return;
	}
	//UE_LOG(NoxelMacro, Log, TEXT("[AM_Nodes::leftClickPressed_Implementation] Macro is at transform %s"), *GetTransform().ToString());
	if (false/*TransformGizmo*/)
	{
		FVector Position, Direction;
		GetRayFromFollow(Position, Direction);
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
		else if (!GetOwningActor()->GetInteractionWidget()->IsOverInteractableWidget() && IsValid(
			GetSelectedNodesContainer()))
		{
			FVector LocationRelative = GetNodePlacement(placementDistance, GetSelectedNodesContainer());
			DrawNode(GetSelectedNodesContainer()->GetComponentTransform().TransformPosition(LocationRelative));
		}

		if (Alternate)
		{
			LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "SelectNodes", "Select Nodes");
			if (selectedNodes.Num() > 0)
			{
				RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "MoveNodes", "Move nodes");
			}
			else
			{
				RightClickHint = FText();
			}
		}
		else
		{
			if (selectedNodes.Num() >= 3) //If a panel can be added
			{
				
				LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "AddPanel", "Add panel");
			}
			else
			{
				LeftClickHint = NSLOCTEXT(MACROS_NAMESPACE, "AddNode", "Add node");
			}

			if (selectedNodes.Num() >= 1) //If nodes are selected
			{
				RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "RemoveSelected", "Remove selected");
			}
			else
			{
				RightClickHint = NSLOCTEXT(MACROS_NAMESPACE, "RemoveTargeted", "Remove targeted");
			}
		}
	}
}

void AM_Nodes::leftClickPressed_Implementation()
{
	//UE_LOG(NoxelMacro, Log, TEXT("[AM_Nodes::leftClickPressed_Implementation] Macro is at transform %s"), *GetTransform().ToHumanReadableString());
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
		if (!Alternate)
		{
			//Adding nodes or panels
			if (selectedNodes.Num() < 3 && IsValid(GetSelectedNodesContainer()))
			{
				//If a panel can't be created
				FNodeID id; //Add a node
				id.Location = GetNodePlacement(placementDistance, GetSelectedNodesContainer());
				id.Object = GetSelectedNodesContainer();
				FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
				queue->AddNodeReferenceOrder({id.Location}, id.Object);
				queue->AddNodeAddOrder({0});
				GetNoxelNetworkingAgent()->SendCommandQueue(queue);
			}
			else if (IsValid(GetSelectedNoxelContainer()))
			{
				//Connect a panel
				FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
				auto nodeMap = queue->CreateNodeReferenceOrdersFromNodeList(selectedNodes);
				queue->AddPanelReferenceOrder({
					                              GetNoxelNetworkingAgent()->GetReservedPanels(
						                              GetSelectedNoxelContainer())[0]
				                              }, GetSelectedNoxelContainer());
				queue->AddPanelAddOrder({0});
				queue->AddPanelPropertiesOrder({0}, 5, 5, false);
				TArray<int32> noderefs = queue->NodeListToNodeReferences(selectedNodes, nodeMap);
				TArray<int32> panelrefs;
				panelrefs.Init(0, selectedNodes.Num());
				queue->AddNodeConnectOrder(noderefs, panelrefs);
				GetNoxelNetworkingAgent()->SendCommandQueue(queue);
			}
			ResetNodesColor(); //Clear all selected node
			selectedNodes.Empty();
		}
		else
		{
			//Selecting
			FVector location, direction, end;
			GetRayFromFollow(location, direction);
			end = location + direction * ray_length;
			FNodeID id;
			if (TraceNodes(location, end, id))
			{
				//UE_LOG(NoxelMacro, Warning, TEXT("[AM_Nodes::leftClickPressed_Implementation]Trace sucessful"));
				if (!selectedNodes.Contains(id))
				{
					//If the node wasn't already selected
					SetNodeColor(id, ENoxelColor::NodeSelected1); //Set the color of the node
					selectedNodes.Add(id);
				}
				else
				{
					SetNodeColor(id, ENoxelColor::NodeInactive); //Set the color of the node
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
		if (!Alternate)
		{
			//Remove nodes or panels
			FNodeID node;
			FPanelID panel;
			FVector location, direction, end;
			GetRayFromFollow(location, direction);
			end = location + direction * ray_length;
			if (selectedNodes.Num() > 0)
			{
				ResetNodesColor();
				FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
				auto nodeMap = queue->CreateNodeReferenceOrdersFromNodeList(selectedNodes);
				TArray<int32> noderefs = queue->NodeListToNodeReferences(selectedNodes, nodeMap);
				queue->AddNodeRemoveOrder(noderefs);
				GetNoxelNetworkingAgent()->SendCommandQueue(queue);
				selectedNodes.Empty();
			}
			else if (TraceNodes(location, end, node))
			{
				UE_LOG(NoxelMacro, Warning, TEXT("Asking to remove node, location %s"), *node.Location.ToString());
				FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
				queue->AddNodeReferenceOrder({node.Location}, node.Object);
				queue->AddNodeRemoveOrder({0});
				GetNoxelNetworkingAgent()->SendCommandQueue(queue);
			}
			else if (TracePanels(location, end, panel))
			{
				UE_LOG(NoxelMacro, Warning, TEXT("Asking to remove panel, id %d"), panel);
				FPanelData pdata;
				if (panel.Object->GetPanelByPanelIndex(panel.PanelIndex, pdata))
				{
					FEditorQueue* queue = GetNoxelNetworkingAgent()->CreateEditorQueue();
					auto nodemap = queue->CreateNodeReferenceOrdersFromNodeList(pdata.Nodes);
					auto nodes = queue->NodeListToNodeReferences(pdata.Nodes, nodemap);
					TArray<int32> panels;
					panels.Init(0, pdata.Nodes.Num());
					queue->AddPanelReferenceOrder({panel.PanelIndex}, panel.Object);
					queue->AddNodeDisconnectOrder(nodes, panels);
					queue->AddPanelRemoveOrder({0});
					GetNoxelNetworkingAgent()->SendCommandQueue(queue);
				}
			}
		}
		else
		{
			//Move nodes
			if (selectedNodes.Num() > 0)
			{
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
