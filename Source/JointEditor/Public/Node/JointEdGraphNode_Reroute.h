//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphNode_Tunnel.h"
#include "Node/JointEdGraphNode.h"
#include "JointEdGraphNode_Reroute.generated.h"

class UJointManager;

/**
 * User-placeable tunnel node for Joint Graph.
 */
UCLASS()
class JOINTEDITOR_API UJointEdGraphNode_Reroute : public UJointEdGraphNode
{
	GENERATED_BODY()

public:

	UJointEdGraphNode_Reroute();

public:
	
	virtual FLinearColor GetNodeTitleColor() const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual bool GetShouldHideNameBox() const override;
	
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const;

public:
	
	virtual FVector2D GetNodeMinimumSize() const override;

public:

	virtual void AllocateDefaultPins() override;

public:

	virtual bool CanDuplicateNode() const override;
	
	virtual void ReconstructNode() override;

	virtual void PostPlacedNewNode() override;

	virtual bool CanHaveSubNode() const override;

	virtual bool CanReplaceNodeClass() override;

	virtual bool CanReplaceEditorNodeClass() override;

public:
	
	virtual void AllocateReferringNodeInstancesOnConnection(TArray<TObjectPtr<UJointNodeBase>>& Nodes, UEdGraphPin* SourcePin) override;
	
	virtual void NodeConnectionListChanged() override;

public:
	
	virtual void UpdateNodeInstance() override;
	
	virtual void UpdateNodeInstanceOuterToJointManager() const override;

	virtual void DestroyNode() override;

public:

	virtual bool CanHaveBreakpoint() const override;

public:
	
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	
public:
	
	UPROPERTY(Transient, VisibleAnywhere, Category="Reroute Node")
	TArray<TWeakObjectPtr<UJointEdGraphNode>> ConnectedRerouteNodes;
	
};
