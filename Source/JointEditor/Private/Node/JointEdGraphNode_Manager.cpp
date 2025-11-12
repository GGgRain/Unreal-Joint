//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/JointEdGraphNode_Manager.h"

#include "JointFunctionLibrary.h"
#include "JointManager.h"
#include "ScopedTransaction.h"
#include "EdGraph/EdGraph.h"

#define LOCTEXT_NAMESPACE "JointGraphNode_Manager"


UJointEdGraphNode_Manager::UJointEdGraphNode_Manager()
{
	NodeWidth = 200;
	NodeHeight = 200;
	
	bCanRenameNode = false;
	
}


bool UJointEdGraphNode_Manager::CanUserDeleteNode() const
{
	return false;
}

bool UJointEdGraphNode_Manager::CanDuplicateNode() const
{
	return false;
}

bool UJointEdGraphNode_Manager::CanReplaceNodeClass()
{
	return false;
}

bool UJointEdGraphNode_Manager::CanReplaceEditorNodeClass()
{
	return false;
}

FLinearColor UJointEdGraphNode_Manager::GetNodeTitleColor() const
{
	return FColor::Turquoise;
}

FText UJointEdGraphNode_Manager::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return (NodeInstance)
			   ? FText::FromName(NodeInstance->GetFName())
			   : FText::FromName(FName("ERROR: NO NODE MANAGER. HOW DID YOU SEE THIS MESSAGE ANYWAY?"));
}

bool UJointEdGraphNode_Manager::CanHaveBreakpoint() const
{
	return false;
}

void UJointEdGraphNode_Manager::AllocateDefaultPins()
{
	PinData.Empty();

	PinData.Add(FJointEdPinData("Start", EEdGraphPinDirection::EGPD_Output));

	//It doesn't be executed by the editor by default so we need to call it
	UpdatePins();
}

void UJointEdGraphNode_Manager::ReallocatePins()
{
	TArray<FJointEdPinData> NewPinData;
	NewPinData.Add(FJointEdPinData("Start", EEdGraphPinDirection::EGPD_Output));
	PinData = UJointFunctionLibrary::ImplementPins(PinData, NewPinData);
}

void UJointEdGraphNode_Manager::ReconstructNode()
{
	UpdatePins();

	RequestUpdateSlate();
}

void UJointEdGraphNode_Manager::NodeConnectionListChanged()
{

	UJointManager* Manager = GetParentManager();

	if(!Manager) return;

	Manager->StartNodes.Empty();
	
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin == nullptr) continue;

		TArray<TObjectPtr<UJointNodeBase>> Nodes;

		Nodes.Empty();
		
		for (UEdGraphPin* LinkedTo : Pin->LinkedTo)
		{
			if (UJointEdGraphNode* LinkedPinOwner = CastPinOwnerToJointEdGraphNode(LinkedTo))
			{
				LinkedPinOwner->AllocateReferringNodeInstancesOnConnection(Nodes, LinkedTo);
			}
		}
		
		Manager->StartNodes.Append(Nodes);
	}
}

UJointManager* UJointEdGraphNode_Manager::GetParentManager()
{
	UObject* Outermost = GetOutermostObject();

	if(!Outermost) return nullptr;

	return Cast<UJointManager>(Outermost);
}

void UJointEdGraphNode_Manager::UpdateNodeInstance()
{
	BindNodeInstance();

	UpdateNodeInstanceOuterToJointManager();

	SyncNodeInstanceSubNodeListFromGraphNode();
}

void UJointEdGraphNode_Manager::UpdateNodeInstanceOuterToJointManager() const
{
	//Revert if this node is sub node since the sub node's outer should not be the Joint manager.
	if (IsSubNode()) return;

	UJointManager* Manager = Cast<UJointManager>(GetGraph()->GetOuter());

	SetNodeInstanceOuterAs(Manager);

	//Propagate the execution to the children sub nodes to make sure all the sub nodes' instances are correctly assigned to its parent node.
	UpdateSubNodesInstanceOuterToJointManager();
}


void UJointEdGraphNode_Manager::SyncNodeInstanceSubNodeListFromGraphNode()
{
	UJointManager* JointManager = GetParentManager();

	if (!JointManager) return;

	//Refresh the sub node array from the graph node's sub nodes.
	JointManager->ManagerFragments.Empty();

	for (UJointEdGraphNode* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		//Something went wrong.
		if (SubNode == this) continue;

		UJointNodeBase* SubNodeInstance = SubNode->GetCastedNodeInstance();

		if (!SubNodeInstance) continue;

		JointManager->ManagerFragments.Add(SubNodeInstance);

		//Propagate to the sub node.
		SubNode->SyncNodeInstanceSubNodeListFromGraphNode();
	}
}

#undef LOCTEXT_NAMESPACE
