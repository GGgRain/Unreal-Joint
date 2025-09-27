//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointGraphPinSlateFactory.h"

#include "JointEdGraphNode.h"
#include "JointEdGraphSchema.h"
#include "Rendering/DrawElements.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

#include "SGraphPin.h"
#include "GraphNode/SJointGraphPin.h"

TSharedPtr<class SGraphPin> FJointGraphPinSlateFactory::CreatePin(class UEdGraphPin* InPin) const
{
	
	if(InPin->GetOwningNodeUnchecked())
	{
		if(UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(InPin->GetOwningNodeUnchecked()))
		{
			return SNew(SJointGraphPinBase, InPin);
		}
	}
	return nullptr;


}
