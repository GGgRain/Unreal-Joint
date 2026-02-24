//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointGraphNodeSlateFactory.h"

#include "JointEdGraphNode_Reroute.h"
#include "SGraphNodeKnot.h"
#include "Styling/SlateTypes.h"

#include "GraphNode/SJointGraphNodeBase.h"
#include "GraphNode/SJointGraphNodeSubNodeBase.h"

#include "Node/SubNode/JointEdGraphNode_Fragment.h"

class SJointGraphNodeKnot : public SGraphNodeKnot
{
public:
	SLATE_BEGIN_ARGS(SJointGraphNodeKnot) {}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, UJointEdGraphNode_Reroute* InKnot)
	{
		SGraphNodeKnot::Construct(SGraphNodeKnot::FArguments(), InKnot);
		//InKnot->OnVisualsChanged().AddSP(this, &SJointGraphNodeKnot::HandleJointNodeChanged);
	}

private:
	void HandleJointNodeChanged(UJointEdGraphNode* InNode)
	{
		UpdateGraphNode();
	}
};

TSharedPtr<class SGraphNode> FJointGraphNodeSlateFactory::CreateNode(class UEdGraphNode* InNode) const
{

	if (UJointEdGraphNode_Reroute* RerouteNode = Cast<UJointEdGraphNode_Reroute>(InNode))
	{
		return SNew(SJointGraphNodeKnot, RerouteNode);
	}
	
	if (UJointEdGraphNode_Fragment* SubNode = Cast<UJointEdGraphNode_Fragment>(InNode))
	{
		return SNew(SJointGraphNodeSubNodeBase, SubNode);
	}
	
	if (UJointEdGraphNode* BaseNode = Cast<UJointEdGraphNode>(InNode))
	{
		return SNew(SJointGraphNodeBase, BaseNode);
	}


	return nullptr;
}


TSharedPtr<class SGraphNode> FJointGraphNodeSlateFactory::CreateNodeForGraphPanel(class UEdGraphNode* InNode, const TSharedPtr<SGraphPanel>& InGraphPanel)
{
	if (UJointEdGraphNode_Reroute* RerouteNode = Cast<UJointEdGraphNode_Reroute>(InNode))
	{
		return SNew(SJointGraphNodeKnot, RerouteNode);
	}
	
	if (UJointEdGraphNode_Fragment* SubNode = Cast<UJointEdGraphNode_Fragment>(InNode))
	{
		TWeakPtr<SJointGraphNodeBase> Slate = SubNode->GetGraphNodeSlateForPanel(InGraphPanel);
		
		if (Slate.IsValid()) return Slate.Pin();// If the subnode already has a slate for this graph panel, return it instead of creating a new one
		
		return SNew(SJointGraphNodeSubNodeBase, SubNode);
	}
	
	if (UJointEdGraphNode* BaseNode = Cast<UJointEdGraphNode>(InNode))
	{
		TWeakPtr<SJointGraphNodeBase> Slate = BaseNode->GetGraphNodeSlateForPanel(InGraphPanel);
		
		if (Slate.IsValid()) return Slate.Pin();// If the subnode already has a slate for this graph panel, return it instead of creating a new one
		
		return SNew(SJointGraphNodeBase, BaseNode);
	}

	return nullptr;
}
