//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Node/JointEdGraphNode.h"
#include "JointEdGraphNode_Tunnel.generated.h"

class UK2Node_Tunnel;
class UJointManager;

/**
 * This is a special type of JointEdGraphNode that functions as a tunnel of nodes.
 * The input pins of this tunnel go to the output pins of InputSinkNode (can be NULL).
 * The output pins of this tunnel node came from the input pins of OutputSourceNode (can be NULL).
 *
 * This is mainly used to connect nodes across different graphs - for example, between a parent graph and a sub graph.
 * The Connector nodes are quite similar, but they're mainly used by the users, but this type of node is mainly used by the system - and can't be destroyed by users.
 */
UCLASS()
class JOINTEDITOR_API UJointEdGraphNode_Tunnel : public UJointEdGraphNode
{
	GENERATED_BODY()

public:

	UJointEdGraphNode_Tunnel();
	
public:

	virtual void LockSynchronizing();
	virtual void UnlockSynchronizing();

	virtual void SynchronizeTunnelNodePins();

	bool bIsSynchronizingPins = false;

public:

	//UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool GetShouldHideNameBox() const override;

	virtual FVector2D GetNodeMinimumSize() const override;

	virtual void AllocateDefaultPins() override;
	virtual void ReallocatePins() override;
	
	virtual bool CanDuplicateNode() const override;
	virtual void ReconstructNode() override;
	virtual void PostPlacedNewNode() override;
	virtual bool CanHaveSubNode() const override;
	virtual bool CanReplaceNodeClass() override;
	virtual bool CanReplaceEditorNodeClass() override;

	
	virtual void NodeConnectionListChanged() override;

	bool bIsUpdatingNodeConnection = false;
	
	virtual void AllocateReferringNodeInstancesOnConnection(TArray<TObjectPtr<UJointNodeBase>>& Nodes, UEdGraphPin* SourcePin) override;
	
	virtual void UpdateNodeInstance() override;
	virtual bool CanUserDeleteNode() const override;
	virtual void DestroyNode() override;
	virtual void ModifyGraphNodeSlate() override;
	
	virtual bool CanHaveBreakpoint() const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	
public:
	
	// The input pins of this tunnel go to the output pins of InputSinkNode (can be NULL).
	virtual UJointEdGraphNode_Tunnel* GetInputSink() const;

	// The output pins of this tunnel node came from the input pins of OutputSourceNode (can be NULL).
	virtual UJointEdGraphNode_Tunnel* GetOutputSource() const;

public:
	
	// Whether this node is allowed to have inputs - it means if it is true, this node must be an output tunnel node (because it has inputs).
	UPROPERTY(EditAnywhere, Category="Tunnel Info")
	uint32 bCanHaveInputs:1;

	// Whether this node is allowed to have outputs - it means if it is true, this node must be an input tunnel node (because it has outputs).
	UPROPERTY(EditAnywhere, Category="Tunnel Info")
	uint32 bCanHaveOutputs:1;

public:
	
	// The output pins of this tunnel node came from the input pins of OutputSourceNode
	UPROPERTY()
	TObjectPtr<UJointEdGraphNode_Tunnel> OutputSourceNode;

	// The input pins of this tunnel go to the output pins of InputSinkNode
	UPROPERTY()
	TObjectPtr<UJointEdGraphNode_Tunnel> InputSinkNode;
	
};
