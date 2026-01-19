//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/JointEdGraphNode_Reroute.h"
#include "JointEdUtils.h"
#include "EdGraph/EdGraph.h"

#define LOCTEXT_NAMESPACE "UJointEdGraphNode_Reroute"


UJointEdGraphNode_Reroute::UJointEdGraphNode_Reroute()
{
	bIsNodeResizeable = false;

	bCanRenameNode = false;
}


bool UJointEdGraphNode_Reroute::GetShouldHideNameBox() const
{
	return true;
}

bool UJointEdGraphNode_Reroute::ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const
{
	OutInputPinIndex = 0;
	OutOutputPinIndex = 1;
	return true;
}

bool UJointEdGraphNode_Reroute::CanReplaceNodeClass()
{
	return false;
}

bool UJointEdGraphNode_Reroute::CanReplaceEditorNodeClass()
{
	return false;
}

FText UJointEdGraphNode_Reroute::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("RerouteNodeTitle", "Reroute Node");
}

void UJointEdGraphNode_Reroute::AllocateDefaultPins()
{
	PinData.Add(FJointEdPinData(
		"Input",
		EEdGraphPinDirection::EGPD_Input
		));

	PinData.Add(FJointEdPinData(
		"Output",
		EEdGraphPinDirection::EGPD_Output
		));
	
	RequestPopulationOfPinWidgets();
}


FLinearColor UJointEdGraphNode_Reroute::GetNodeTitleColor() const
{
	return FColor::Cyan;
}

FVector2D UJointEdGraphNode_Reroute::GetNodeMinimumSize() const
{
	return FVector2D(20, 20);
}

bool UJointEdGraphNode_Reroute::CanDuplicateNode() const
{
	return true; //Take care the occasion when there is another output node with the same Guid.
}

void UJointEdGraphNode_Reroute::ReconstructNode()
{
	UpdateNodeInstance();

	UpdatePins();

	NodeConnectionListChanged();
}

void UJointEdGraphNode_Reroute::PostPlacedNewNode()
{
	UpdatePins();

	Super::PostPlacedNewNode();
}

bool UJointEdGraphNode_Reroute::CanHaveSubNode() const
{
	return false;
}

void UJointEdGraphNode_Reroute::AllocateReferringNodeInstancesOnConnection(TArray<TObjectPtr<UJointNodeBase>>& Nodes, UEdGraphPin* SourcePin)
{
	// SEE NodeConnectionListChanged() FOR DETAILS
	// 1. if a reroute node's output pin is connected to other nodes, we cache that on ConnectedRerouteNodes.
	// 2. if a reroute node's output pin is connected to other reroute nodes, we use the cached ConnectedRerouteNodes of the leaf reroute node to propagate the connection.
	// 3. when a reroute node's connection has been changed, we notify it to the left side (input side) reroute nodes to update their cached ConnectedRerouteNodes.
	
	for (TWeakObjectPtr<UJointEdGraphNode> ConnectedRerouteNode : ConnectedRerouteNodes)
	{
		if (ConnectedRerouteNode.IsValid())
		{
			if (UJointNodeBase* ConnectedNodeInstance = ConnectedRerouteNode->GetCastedNodeInstance())
			{
				Nodes.Add(ConnectedNodeInstance);
			}
		}
	}
}


void UJointEdGraphNode_Reroute::NodeConnectionListChanged()
{
	
	// 1. if a reroute node's output pin is connected to other nodes, we cache that on ConnectedRerouteNodes.
	// 2. if a reroute node's output pin is connected to other reroute nodes, we use the cached ConnectedRerouteNodes of the leaf reroute node to propagate the connection.
	// 3. when a reroute node's connection has been changed, we notify it to the left side (input side) reroute nodes to update their cached ConnectedRerouteNodes.
	
	// Clear current cached connected reroute nodes.
	ConnectedRerouteNodes.Empty();
	
	// Get output pin and input pin.
	
	UEdGraphPin* InputPin = nullptr;
	UEdGraphPin* OutputPin = nullptr;
	
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EEdGraphPinDirection::EGPD_Input)
		{
			InputPin = Pin;
		}
		else if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
		{
			OutputPin = Pin;
		}
	}
	
	// Process output pin connections. (1 & 2)
	if (OutputPin)
	{
		for (UEdGraphPin* LinkedTo : OutputPin->LinkedTo)
		{
			// Check if the linked node is a reroute node. (2)
			if (UJointEdGraphNode_Reroute* LinkedRerouteNode = Cast<UJointEdGraphNode_Reroute>(CastPinOwnerToJointEdGraphNode(LinkedTo)))
			{
				// If connected to another reroute node, propagate its cached connected reroute nodes. (2)
				for (TWeakObjectPtr<UJointEdGraphNode> CachedConnectedRerouteNode : LinkedRerouteNode->ConnectedRerouteNodes)
				{
					if (CachedConnectedRerouteNode.IsValid())
					{
						ConnectedRerouteNodes.AddUnique(CachedConnectedRerouteNode);
					}
				}
			}
			// Non-reroute node. (1)
			else 
			{
				// If connected to a non-reroute node, add it to the cached connected reroute nodes.
				if (UJointEdGraphNode* LinkedJointNode = Cast<UJointEdGraphNode>(CastPinOwnerToJointEdGraphNode(LinkedTo)))
				{
					ConnectedRerouteNodes.AddUnique(LinkedJointNode);
				}
			}
		}
	}
	
	// Notify input side reroute nodes to update their cached connected reroute nodes. (3)
	if (InputPin)
	{
		for (UEdGraphPin* LinkedTo : InputPin->LinkedTo)
		{
			if (UJointEdGraphNode* LinkedRerouteNode = CastPinOwnerToJointEdGraphNode(LinkedTo))
			{
				LinkedRerouteNode->NodeConnectionListChanged();
			}
		}
	}
	
	Super::NodeConnectionListChanged();
}

void UJointEdGraphNode_Reroute::UpdateNodeInstance()
{
}

void UJointEdGraphNode_Reroute::UpdateNodeInstanceOuterToJointManager() const
{
	//bubble up (because we don't have own node instance)
}

void UJointEdGraphNode_Reroute::DestroyNode()
{
	Super::DestroyNode();
}

bool UJointEdGraphNode_Reroute::CanHaveBreakpoint() const
{
	return false;
}

void UJointEdGraphNode_Reroute::GetNodeContextMenuActions(UToolMenu* Menu,
                                                               UGraphNodeContextMenuContext* Context) const
{
	return;
}

#undef LOCTEXT_NAMESPACE
