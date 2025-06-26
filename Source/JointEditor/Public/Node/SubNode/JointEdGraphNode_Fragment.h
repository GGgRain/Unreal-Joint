//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Node/JointEdGraphNode.h"
#include "JointEdGraphNode_Fragment.generated.h"

class UJointNodeBase;

/**
 * Sub node for expression of additional data of a node. This type of node doesn't have any pins.
 */
UCLASS(Category = "Joint")
class JOINTEDITOR_API UJointEdGraphNode_Fragment : public UJointEdGraphNode
{
	GENERATED_BODY()

public:

	UJointEdGraphNode_Fragment();

public:
	
	virtual void ResizeNode(const FVector2D& NewSize) override;

	virtual TSubclassOf<UJointNodeBase> SupportedNodeClass() override;

public:

	virtual void AllocateDefaultPins() override;


public:
	
	/**
	 * Return whether to implement this fragment's graph slate on the parent node's sub node box automatically or not.
	 * This is useful when you have to attach its graph node at somewhere else.
	 * Check out the UJointEdFragment_Condition and UJointEdFragment_ConditionInstance.
	 */
	virtual bool ShouldManuallyImplementSlate() const;
	
};
