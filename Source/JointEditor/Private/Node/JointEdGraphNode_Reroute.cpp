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

void UJointEdGraphNode_Reroute::NotifyConnectionChangedToConnectedNodes()
{
	
}

void UJointEdGraphNode_Reroute::NodeConnectionListChanged()
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin == nullptr) continue;

		for (UEdGraphPin* LinkedTo : Pin->LinkedTo)
		{
			if (UJointEdGraphNode* LinkedToGraphNode = CastPinOwnerToJointEdGraphNode(LinkedTo))
			{
				//LinkedToGraphNode->AllocateReferringNodeInstancesOnConnection(ConnectedNodes, LinkedTo);
			}
		}
	}
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
