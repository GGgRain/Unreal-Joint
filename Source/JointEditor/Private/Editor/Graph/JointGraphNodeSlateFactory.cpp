//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointGraphNodeSlateFactory.h"

#include "Styling/SlateTypes.h"

#include "GraphNode/SJointGraphNodeBase.h"
#include "GraphNode/SJointGraphNodeSubNodeBase.h"

#include "Node/SubNode/JointEdGraphNode_Fragment.h"

TSharedPtr<class SGraphNode> FJointGraphNodeSlateFactory::CreateNode(class UEdGraphNode* InNode) const
{

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