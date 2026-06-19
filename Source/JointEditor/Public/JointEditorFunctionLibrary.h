//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/SharedType/JointEditorSharedTypes.h"
#include "SharedType/JointSharedTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Editor/Script/JointScriptLinker.h"
#include "Editor/Slate/Markdown/SJointMDSlate_Admonitions.h"
#include "Node/JointFragment.h"
#include "JointEditorFunctionLibrary.generated.h"

enum class EJointMDAdmonitionType : uint8;
class UJointNodePreset;
class UJointEdGraph;
class UJointEdGraphNode;
class UJointManager;
class UEdGraphNode;
class UEdGraphPin;

/**
 * A Blueprint function library for Joint Editor related utilities.
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
	 * Get the Joint Ed Graph Node for the provided Manager Fragment Node Guid.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static UJointEdGraphNode* GetManagerFragmentGraphNodeForNodeGuid(
		const UJointManager* TargetJointManager,
		const FGuid& NodeGuid
	);

	static UEdGraphNode* FindGraphNodeForNodeInstance(const UJointNodeBase* NodeInstance);

	static UJointEdGraphNode* FindGraphNodeWithProvidedNodeInstanceGuid(
		UJointManager* JointManager,
		const FGuid& NodeGuid
	);
	
public:
	
	/**
	 * Add a Joint Ed Graph Node based on the provided node preset to the provided graph.
	 * @param TargetJointManager The target Joint manager that will own the created node.
	 * @param OptionalTargetGraph The target graph to add the node to. If null, it will add to the root graph of the Target Joint Manager.
	 * @param NodePreset The node preset to create the node with.
	 * @param Position The position to place the created node at.
	 * @return Created Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static TArray<UJointEdGraphNode*> AddNodePreset(
		UJointManager* TargetJointManager, 
		UEdGraph* OptionalTargetGraph,
		UJointNodePreset* NodePreset,
		const FVector2D Position
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
		const FVector2D NodePosition,
		const FVector2D NodeSize = FVector2D(300.f, 250.f)
	);
	
	/**
	 * Add a sub Joint Ed Graph Node to the provided parent node.
	 * @param TargetJointManager The target Joint manager that will own the created node.
	 * @param ParentEdNode The parent Joint Ed Graph Node to add the sub node to.
	 * @param NodeClass The class of the Joint Ed Graph Node to create.
	 * @return Created Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static UJointEdGraphNode* AddFragment(
		UJointManager* TargetJointManager,
		UJointEdGraphNode* ParentEdNode,
		TSubclassOf<UJointNodeBase> NodeClass
	);
	
	/**
	 * Add a manager fragment Joint Ed Graph Node to the provided Joint manager's root graph.
	 * @param TargetJointManager The target Joint manager that will own the created node.
	 * @param NodeClass The class of the Joint Ed Graph Node to create.
	 * @return Created Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static UJointEdGraphNode* AddManagerFragment(
		UJointManager* TargetJointManager,
		TSubclassOf<UJointNodeBase> NodeClass
	);
	
public:
	
	/**
	 * Remove the provided Joint Ed Graph Node from its Joint manager.
	 * @param TargetEdNode The target Joint Ed Graph Node to remove.
	 * @param OptionalFileEntry The optional script file entry associated with the node, which will be used to clear the node's Guid from the script linkage after removing the node.
	 * @return true if the node was successfully removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static bool RemoveNode(
		UEdGraphNode* TargetEdNode,
		const FJointScriptLinkerFileEntry& OptionalFileEntry = FJointScriptLinkerFileEntry()
	);
	
	/**
	 * Remove Graph Nodes by the provided Node Guids from the Joint manager.
	 * @param TargetJointManager The target Joint manager to remove the nodes from.
	 * @param OptionalFileEntry The optional script file entry associated with the nodes. If provided, it will be used to clear the nodes' Guids from the script linkage after removing the nodes.
	 * @param NodeGuidsToRemove The array of Node Guids to remove.
	 * @return true if the action was successfully executed.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static bool RemoveNodesByGuids(
		UJointManager* TargetJointManager,
		const TArray<FGuid>& NodeGuidsToRemove,
		const FJointScriptLinkerFileEntry& OptionalFileEntry = FJointScriptLinkerFileEntry()
	);

	/**
	 * Remove graph nodes by the provided mapping keys, that has been recognized by the script linker.
	 * @param TargetJointManager The target Joint manager to remove the nodes from.
	 * @param NodeKeysToRemove 
	 * @param FileEntry 
	 * @return true if the action was successfully executed.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static bool RemoveNodesByMappingKey(
		UJointManager* TargetJointManager,
		const TArray<FString>& NodeKeysToRemove,
		const FJointScriptLinkerFileEntry& FileEntry = FJointScriptLinkerFileEntry()
	);
	
	/**
	 * Remove all Joint Ed Graph Nodes linked with the provided script file entry from the Joint manager.
	 * @param TargetJointManager The target Joint manager to remove the nodes from.
	 * @param FileEntry The script file entry associated with the nodes.
	 * @return true if the action was successfully executed.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void RemoveAllNodesLinkedWithScript(
		UJointManager* TargetJointManager,
		const FJointScriptLinkerFileEntry& FileEntry
	);

public:
	
	/**
	 * Remove the provided Joint Ed Graph from its Joint manager.
	 * This cannot remove the root graph of a Joint manager.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void RemoveGraph(UJointEdGraph* GraphToRemove);

public:
	
	/**
	 * Try to get existing base Joint Ed Graph Nodes for the provided Node Guids, and if none exists, create at least one base Joint Ed Graph Node.
	 * @return Array of existing or created Joint Ed Graph Nodes.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static TArray<UJointEdGraphNode*> GetBaseNodesById(
		const UJointManager* TargetJointManager,
		const FJointScriptLinkerFileEntry& FileEntry,
		const FString& KnownId
	);

	/**
	 * Try to get existing base Joint Ed Graph Nodes for the provided Node Guids, and if none exists, create at least one base Joint Ed Graph Node.
	 * @return Array of existing or created Joint Ed Graph Nodes.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static TArray<UJointEdGraphNode*> GetOrCreateBaseNodesById(
		UJointManager* TargetJointManager,
		const FJointScriptLinkerFileEntry& FileEntry,
		const FString& KnownId,
		UEdGraph* OptionalTargetGraphForCreation,
		TSubclassOf<UJointNodeBase> NodeClassForCreation,
		const FVector2D NodePositionForCreation,
		bool& bCreatedNewNode,
		const FVector2D NodeSize = FVector2D(300.f, 250.f),
		bool bLinkWithScript = true,
		bool bClearIdIfCreatedNewNode = true
	);
	
	
	/**
	 * Check if the provided Joint Ed Graph Node is linked with the script file entry and external identifier.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static TArray<UJointEdGraphNode*> GetFragmentNodesById(
		const UJointManager* TargetJointManager,
		const FJointScriptLinkerFileEntry& FileEntry,
		const FString& KnownId
	);
	
	/**
	 * Try to get existing fragment Joint Ed Graph Nodes for the provided Node Guids, and if none exists, create at least one fragment Joint Ed Graph Node.
	 * @return Array of existing or created Joint Ed Graph Nodes.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static TArray<UJointEdGraphNode*> GetOrCreateFragmentNodesByIds(
		 UJointManager* TargetJointManager,
		 const FJointScriptLinkerFileEntry& FileEntry,
		 const FString& KnownId,
		UJointEdGraphNode* ParentEdNodeForCreation,
		TSubclassOf<UJointNodeBase> NodeClassForCreation,
		bool& bCreatedNewNode,
		bool bLinkWithScript = true,
		bool bClearIdIfCreatedNewNode = true
	);
	
	
	/**
	 * Check if the provided Joint Ed Graph Node is linked with the script file entry and external identifier.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static TArray<UJointEdGraphNode*> GetManagerFragmentNodesById(
		const UJointManager* TargetJointManager,
		const FJointScriptLinkerFileEntry& FileEntry,
		const FString& KnownId
	);
	
	/**
	 * Try to get existing manager fragment Joint Ed Graph Nodes for the provided Node Guids, and if none exists, create at least one manager fragment Joint Ed Graph Node.
	 * @return Array of existing or created Joint Ed Graph Nodes.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static TArray<UJointEdGraphNode*> GetOrCreateManagerFragmentNodesById(
		UJointManager* TargetJointManager,
		const FJointScriptLinkerFileEntry& FileEntry,
		const FString& KnownId,
		TSubclassOf<UJointNodeBase> NodeClassForCreation,
		bool& bCreatedNewNode,
		bool bLinkWithScript = true,
		bool bClearIdIfCreatedNewNode = true
	);
	
	
public:
	
	/**
	 * Link the provided Joint Ed Graph Node with the script file entry and external identifier.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void LinkNodeWithScript(
		UJointEdGraphNode* TargetEdNode,
		const FJointScriptLinkerFileEntry& FileEntry,
		const FString& Id
	);
	
	/**
	 * Link the provided Joint Script Parser with the script file entry.
	 * This will also save the parser's settings to the script linker, so that the parser can be used for reimporting and other operations that require the parser's settings.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void LinkParserWithScript(
		UJointScriptParser* Parser,
		const FJointScriptLinkerFileEntry& FileEntry
	);

	static void OpenJointScriptFileSelectionWindow(
		TArray<FString>& OutFilePaths,
		bool bAllowMultipleSelection
	);

	static void ImportFileToJointManager(
		UJointManager* TargetManager,
		const FString& FilePath,
		UJointScriptParser* Parser,
		const bool& bFireNotifications = true
	);

	/**
	 * Unlink the provided Joint Ed Graph Node from its script linkage.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void UnlinkNodeFromScript(
		UJointEdGraphNode* TargetEdNode
	);
	
	/**
	 * Clear the provided external identifier from the script linkage, which will unlink all nodes linked with the external identifier.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void ClearIdKeyFromScriptLinkage(
		const FJointScriptLinkerFileEntry& FileEntry,
		const FString& Id
	);
	
	/**
	 * Clear the provided node Guid from the script linkage, which will unlink the node with the node Guid.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void ClearNodeGuidFromScriptLinkage(
		const FJointScriptLinkerFileEntry& FileEntry,
		const FGuid& NodeGuid
	);
	
	/**
	 * Link the provided Joint Ed Graph Nodes with the script file entry and external identifier in bulk.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void LinkNodesWithScriptBulk(
		const TArray<UJointEdGraphNode*>& TargetEdNodes,
		const FJointScriptLinkerFileEntry& FileEntry,
		const FString& Id
	);
	
	/**
	 * Unlink the provided Joint Ed Graph Nodes from their script linkage in bulk.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void UnlinkNodesFromScriptBulk(
		const TArray<UJointEdGraphNode*>& TargetEdNodes
	);
	
public:
	
	/**
	 * Get the Joint Node instance from the provided Joint Ed Graph Node, casted to the provided Node Class.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities", meta=(DeterminesOutputType="NodeClass"))
	static UJointNodeBase* GetNodeInstanceAs(
		const UJointEdGraphNode* TargetEdNode, 
		const TSubclassOf<UJointNodeBase> NodeClass
	);
	
public:
	
	/**
	 * Get the external identifier linked to the provided Joint Ed Graph Node from the script file entry.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static FJointScriptLinkerNodeSet GetNodeGuidsLinkedWithScript(
		UJointManager* TargetJointManager,
		const FJointScriptLinkerFileEntry& FileEntry,
		const FString& Id
	);
	
	/**
	 * Get all the node Guids linked with the provided script file entry, regardless of the external identifier.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static TMap<FString, FJointScriptLinkerNodeSet> GetAllNodeGuidsLinkedWithScript(
		UJointManager* TargetJointManager,
		const FJointScriptLinkerFileEntry& FileEntry
	);
	
public:
	
	/**
	 * Set the property that makes the provided Joint Ed Graph Node fit its size to its content.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void SetBaseNodeFitSizeToContent(UJointEdGraphNode* TargetEdNode, bool bFit = true);
	
	/**
	 * Resize the provided Joint Ed Graph Node to the new size.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void ResizeNode(UJointEdGraphNode* TargetEdNode, const FVector2D& NewSize);
	
	/**
	 * Get the position of the provided Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static FVector2D GetNodePosition(const UJointEdGraphNode* TargetEdNode);
	
	/**
	 * Set the position of the provided Joint Ed Graph Node to the new position.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void SetNodePosition(UJointEdGraphNode* TargetEdNode, const FVector2D& NewPosition);

	/**
	 * Automatically align all the nodes in the Joint graph of the provided Joint manager, starting from the entry node (the root node), and aligning the nodes in a top-down manner.
	 * @param TargetJointManager 
	 * @param LevelSpacingX 
	 * @param NodeSpacingY 
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void AutomaticallyAlignNodesFromEntry(
		UJointManager* TargetJointManager,
		float LevelSpacingX = 600.f,
		float NodeSpacingY = 600.f
	);
	
public:
	
	/**
	 * Get the Joint Ed Node settings from the provided Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static FJointEdNodeSetting GetEdNodeSettings(UJointEdGraphNode* TargetEdNode);
	
	/**
	 * Get the Joint Ed Node settings from the provided Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void SetEdNodeSettings(
		UJointEdGraphNode* TargetEdNode,
		const FJointEdNodeSetting& NewSettings
	);
	
public:
	
	/**
	 * Set the orientation of the sub node box from the provided Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void SetSubNodeBoxOrientation(
		UJointEdGraphNode* TargetEdNode,
		EOrientation NewOrientation
	);
	
public:

	/**
	 * Connect two pins together.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void ConnectPins(
		UJointEdGraphNode* ANode,
		FJointEdPinData APin,
		UJointEdGraphNode* BNode,
		FJointEdPinData BPin
	);
	
	/**
	 * Disconnect the provided pin from all its connections.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void DisconnectPins(
		UJointEdGraphNode* Node,
		FJointEdPinData Pin
	);
	
	/**
	 * Connect the provided pin to the start node of the Joint graph (the root node).
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void ConnectToRootNode(
		UJointEdGraphNode* Node,
		FJointEdPinData Pin
	);

	static void MakeConnectionFromTheDraggedPin(
		UEdGraphPin* FromPin,
		UEdGraphNode* ConnectedNode
	);

	static bool TryMakeConnectionBetweenPins(
		UEdGraphPin* FromPin,
		UEdGraphPin* ToPin
	);

	
public:
	
	/**
	 * Get an input pin from the provided Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static FJointEdPinData GetInputPin(
		UJointEdGraphNode* TargetEdNode,
		int Index = 0,
		bool bSearchSubNodes = false
	);
	
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static TArray<FJointEdPinData> GetInputPins(
		UJointEdGraphNode* TargetEdNode,
		bool bSearchSubNodes = false
	);
	
	/**
  	 * Get an output pin from the provided Joint Ed Graph Node.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static FJointEdPinData GetOutputPin(
		UJointEdGraphNode* TargetEdNode,
		int Index = 0,
		bool bSearchSubNodes = false
	);
	
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static TArray<FJointEdPinData> GetOutputPins(
		UJointEdGraphNode* TargetEdNode,
		bool bSearchSubNodes = false
	);
	
public:

	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static UJointEdGraphNode* FindEntryNode(
		UJointManager* TargetJointManager
	);
	
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static TArray<UJointEdGraphNode*> GetConnectedNodesOnInputPins(
		UJointEdGraphNode* TargetEdNode,
		bool bSearchSubNodes = true
	);
	
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static TArray<UJointEdGraphNode*> GetConnectedNodesOnOutputPins(
		UJointEdGraphNode* TargetEdNode,
		bool bSearchSubNodes = true
	);
	
public:
	
	/**
	 * Modify the target object (for undo/redo support, Introduced for the Blueprint)
	 * calling this function will 'save' the current state of the object to the transaction buffer for undo/redo support, so it can restore to this state later.
	 * @note see UObject::Modify for more details.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void ModifyObject(UObject* TargetObject);
	
public:
	
	/**
	 * Get a gameplay tag from the provided string.
	 */
	UFUNCTION(BlueprintPure, Category="Joint Editor Utilities")
	static FGameplayTag GetGameplayTagFromString(const FString& TagString);
	
public:
	
	/**
	 * Fire a notification.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Editor Utilities")
	static void FireNotification(
		FText NotificationTitleText,
		FText NotificationText,
		EJointMDAdmonitionType AdmonitionType = EJointMDAdmonitionType::Info,
		float DurationSeconds = 5.f,
		bool bReportOnLog = true
	);
	
};
