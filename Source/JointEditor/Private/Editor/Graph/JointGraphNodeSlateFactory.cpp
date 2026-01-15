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
		//Reuse the old one.
		if(SubNode->GetGraphNodeSlate().IsValid())
		{
			TSharedPtr<SJointGraphNodeBase> ExistingSlate = SubNode->GetGraphNodeSlate().Pin();
			
			ExistingSlate->UpdateGraphNode();
			
			return ExistingSlate;
		}
		return SNew(SJointGraphNodeSubNodeBase, SubNode);
	}
	
	if (UJointEdGraphNode* BaseNode = Cast<UJointEdGraphNode>(InNode))
	{
		//Reuse the old one.
		if (BaseNode->GetGraphNodeSlate().IsValid())
		{
			TSharedPtr<SJointGraphNodeBase> ExistingSlate = BaseNode->GetGraphNodeSlate().Pin();
			
			ExistingSlate->UpdateGraphNode();
			
			return ExistingSlate;
		}
		
		return SNew(SJointGraphNodeBase, BaseNode);
	}


	return nullptr;
}