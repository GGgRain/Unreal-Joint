//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointNodeStyleFactory.h"

#include "Styling/SlateTypes.h"

#include "GraphNode/SJointGraphNodeBase.h"
#include "GraphNode/SJointGraphNodeSubNodeBase.h"

#include "Node/SubNode/JointEdGraphNode_Fragment.h"

TSharedPtr<class SGraphNode> FJointNodeStyleFactory::CreateNode(class UEdGraphNode* InNode) const
{

	if (UJointEdGraphNode_Fragment* SubNode = Cast<UJointEdGraphNode_Fragment>(InNode))
	{
		//Reuse the old one.
		if(SubNode->GetGraphNodeSlate().IsValid())
		{
			SubNode->GetGraphNodeSlate().Pin()->UpdateGraphNode();
			
			return SubNode->GetGraphNodeSlate().Pin();
		}
		return SNew(SJointGraphNodeSubNodeBase, SubNode);
	}
	
	if (UJointEdGraphNode* BaseNode = Cast<UJointEdGraphNode>(InNode))
	{
		//Reuse the old one.
		if(BaseNode->GetGraphNodeSlate().IsValid())
		{
			BaseNode->GetGraphNodeSlate().Pin()->UpdateGraphNode();
			
			return BaseNode->GetGraphNodeSlate().Pin();
		}
		
		return SNew(SJointGraphNodeBase, BaseNode);
	}


	return nullptr;
}