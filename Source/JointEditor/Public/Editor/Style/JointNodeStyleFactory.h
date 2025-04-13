//Copyright 2022~2024 DevGrain. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

struct JOINTEDITOR_API FJointNodeStyleFactory : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* InNode) const override;
};