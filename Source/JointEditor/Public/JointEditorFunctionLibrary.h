//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SharedType/JointSharedTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JointEditorFunctionLibrary.generated.h"

class UJointEdGraphNode;
class UJointManager;

/**
 * 
 */
UCLASS()
class JOINTEDITOR_API UJointEditorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	UJointEditorFunctionLibrary();
	
public:

	/** 
	 * Get the Joint Ed Graph Node for the provided Node Guid.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static UJointEdGraphNode* GetBaseNodeGraphNodeForNodeGuid(
		const UJointManager* TargetJointManager,
		const FGuid& NodeGuid
	);
	
	/** 
	 * Get the Joint Ed Graph Node for the provided Fragment Node Guid.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static UJointEdGraphNode* GetFragmentGraphNodeForNodeGuid(
		const UJointManager* TargetJointManager,
		const FGuid& NodeGuid
	);
	
	
	/**
	 * Add a base Joint Ed Graph Node to the provided graph.
	 * @param TargetJointManager The target Joint manager that will own the created node.
	 * @param OptionalTargetGraph The target graph to add the node to. If null, it will add to the root graph of the Target Joint Manager.
	 * @param NodeClass The class of the Joint Ed Graph Node to create.
	 * @param NodePosition The position to place the created node at.
	 * @return Created Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static UJointEdGraphNode* AddBaseNode(
		UJointManager* TargetJointManager, 
		UEdGraph* OptionalTargetGraph, 
		TSubclassOf<UJointNodeBase> NodeClass,
		const FVector2D& NodePosition
	);
	
	/**
	 * Add a sub Joint Ed Graph Node to the provided parent node.
	 * @param TargetJointManager The target Joint manager that will own the created node.
	 * @param ParentEdNode The parent Joint Ed Graph Node to add the sub node to.
	 * @param NodeClass The class of the Joint Ed Graph Node to create.
	 * @return Created Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static UJointEdGraphNode* AddSubNode(
		UJointManager* TargetJointManager,
		UJointEdGraphNode* ParentEdNode,
		TSubclassOf<UJointNodeBase> NodeClass
	);

	/**
	 * Add or get existing base Joint Ed Graph Node for the provided Node Guid. (amalgamation of GetBaseNodeGraphNodeForNodeGuid & AddBaseNode for the convenience)
	 * @return Existing or created Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static UJointEdGraphNode* AddOrGetBaseNodeGraphNodeForNodeGuid(
		UJointManager* TargetJointManager,
		const FGuid& NodeGuid,
		UEdGraph* OptionalTargetGraph,
		TSubclassOf<UJointNodeBase> NodeClass,
		const FVector2D& NodePosition
	);
	
	/**
	 * Add or get existing fragment Joint Ed Graph Node for the provided Node Guid. (amalgamation of GetFragmentGraphNodeForNodeGuid & AddSubNode for the convenience)
	 * @return Existing or created Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static UJointEdGraphNode* AddOrGetFragmentGraphNodeForNodeGuid(
		UJointManager* TargetJointManager,
		const FGuid& NodeGuid,
		UJointEdGraphNode* ParentEdNode,
		TSubclassOf<UJointNodeBase> NodeClass
	);

public:
	
	/**
	 * Fit the base node size to its content (mostly for the sub nodes)
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void FitBaseNodeSizeToContent(UJointEdGraphNode* TargetEdNode);
	
};
