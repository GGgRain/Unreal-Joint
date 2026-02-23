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
	
	virtual void PostPlacedNewNode() override;

	
public:

	const bool& IsDissolvedSubNode() const { return bIsDissolvedSubNode; }

	void DissolveSelf();
	void SolidifySelf();

private:

	/**
	 * Whether this sub node is merged form.
	 * Joint 2.10.0: Now subnodes can be merged into a single node.
	 * Merge subnodes doesn't need to render everything - only few things - so we can optimize the rendering and interaction.
	 */
	UPROPERTY()
	bool bIsDissolvedSubNode = false;
	
	
};
