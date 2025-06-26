//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "JointEdGraphSchemaActions.h"
#include "JointWiggleWireSimulator.h"
#include "JointEdGraphSchema.generated.h"

class FJointGraphConnectionDrawingPolicy;
class UJointEdGraphNode;


UCLASS()
class JOINTEDITOR_API UJointEdGraphSchema : public UEdGraphSchema
{
public:
	
	GENERATED_BODY()

public:

	UJointEdGraphSchema(const class FObjectInitializer&);

public:

	//Spawn default graph node for the new graph.
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;

public:

	//Implement the Graph Context Actions for the Joint graph when you get if you right click on the graph.
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;

	//Implement the Graph Context Actions for the Joint graph node when you get if you right click on the graph node.
	virtual void GetGraphNodeContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const;

	//Implement actions for the add comment action on the provided ContextMenuBuilder.
	static void ImplementAddCommentAction(FGraphContextMenuBuilder& ContextMenuBuilder);

	//Implement actions for the add connector action on the provided ContextMenuBuilder.
	static void ImplementAddConnectorAction(FGraphContextMenuBuilder& ContextMenuBuilder);

	//Implement actions for the add node action on the provided ContextMenuBuilder. Each node types will be implemented as each actions.
	static void ImplementAddNodeActions(FGraphContextMenuBuilder& ContextMenuBuilder);

	//Implement actions for the add fragment action on the provided ContextMenuBuilder. Each fragment types will be implemented as each actions.
	static void ImplementAddFragmentActions(FGraphContextMenuBuilder& ContextMenuBuilder);

public:
	
	static TSharedPtr<FJointSchemaAction_NewNode> CreateNewNodeAction(const FText& Category, const FText& MenuDesc, const FText& Tooltip);

	static TSharedPtr<FJointSchemaAction_NewSubNode> CreateNewSubNodeAction(const FText& Category, const FText& MenuDesc, const FText& Tooltip);
	
public:

	//Context Menu

	/**
	 * Gets actions that should be added to the right-click context menu for a node or pin
	 * 
	 * @param	Menu				The menu to append actions to.
	 * @param	Context				The menu's context.
	 */
	virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	FName GetContextMenuName() const;
	
	FName GetContextMenuName(UClass* InClass) const;
	
	FName GetParentContextMenuName() const override;

public:

	//Connection & Merge

	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;

	virtual const FPinConnectionResponse CanMergeNodes(const UEdGraphNode* A, const UEdGraphNode* B) const override;

public:

	//Visuals
	
	virtual FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;

	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;

public:

	virtual bool ShouldAlwaysPurgeOnModification() const override;

	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;

	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const;
	
	virtual bool FadeNodeWhenDraggingOffPin(const UEdGraphNode* Node, const UEdGraphPin* Pin) const override;

	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;

	virtual void ReconstructNode(UEdGraphNode& TargetNode, bool bIsBatchRequest=false) const override;

public:
	
	TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const override;

public:
	
	int32 GetNodeSelectionCount(const UEdGraph* Graph) const override;
	
};
