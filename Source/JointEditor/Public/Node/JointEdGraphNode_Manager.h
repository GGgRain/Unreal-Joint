//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Node/JointEdGraphNode.h"
#include "JointEdGraphNode_Manager.generated.h"

class UJointManager;
/**
 * 
 */
UCLASS()
class JOINTEDITOR_API UJointEdGraphNode_Manager : public UJointEdGraphNode
{
	GENERATED_BODY()

public:

	UJointEdGraphNode_Manager();

public:

	virtual bool CanUserDeleteNode() const override;

	virtual bool CanDuplicateNode() const override;

public:
	
	virtual bool CanReplaceNodeClass() override;

	virtual bool CanReplaceEditorNodeClass() override;

public:
	
	virtual FLinearColor GetNodeTitleColor() const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

public:
	
	virtual bool CanHaveBreakpoint() const override;

public:

	virtual void AllocateDefaultPins() override;

	virtual void ReallocatePins() override;

public:
	
	virtual void ReconstructNode() override;

public:

	virtual void NodeConnectionListChanged() override;

	//Get the Joint manager.
	UJointManager* GetParentManager();

public:
	
	virtual void UpdateNodeInstance() override;
	
	virtual void UpdateNodeInstanceOuterToJointManager() const override;

	virtual void SyncNodeInstanceSubNodeListFromGraphNode() override;
	
};
