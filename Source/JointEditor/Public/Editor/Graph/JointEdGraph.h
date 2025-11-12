//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Node/JointEdGraphNode.h"
#include "UnrealEdGlobals.h"
#include "Editor/Debug/JointNodeDebugData.h"
#include "EdGraph/EdGraph.h"
#include "JointEdGraph.generated.h"


class UJointEdGraphSchema;
class FJointEditorToolkit;
DECLARE_MULTICAST_DELEGATE(FOnGraphRequestUpdate);

UCLASS()
class JOINTEDITOR_API UJointEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:

	UJointEdGraph();
	
public:

	UPROPERTY()
	TObjectPtr<UJointManager> JointManager;

public:
	
	/**
	 * Debug data for the graph nodes.
	 * Only contains the data for the nodes that have a debug data.
	 * Debug data will not be duplicated to the other assets - because it literally don't need to.
	 */
	UPROPERTY(DuplicateTransient)
	TArray<FJointNodeDebugData> DebugData;

private:

	/**
	 * Optional toolkit of the graph that is currently editing the graph.
	 */
	TWeakPtr<FJointEditorToolkit> Toolkit;

public:
	
	/**
	 * Get the parent graph of this graph if exists.
	 * @return Parent graph of this graph if exists, nullptr otherwise
	 */
	UJointEdGraph* GetParentGraph() const;

	/**
	 * Get the root graph of this graph. If this graph is the root graph, it will return itself.
	 * @return root graph of this graph
	 */
	UJointEdGraph* GetRootGraph() const;

	/**
	 * Check whether this graph is the root graph of the Joint manager.
	 * @return true if this graph is the root graph of the Joint manager, false otherwise
	 */
	bool IsRootGraph() const;

	/**
	 * Get all the sub graphs that are under this graph.
	 * @return All the sub graphs that are under this graph
	 */
	TArray<UJointEdGraph*> GetAllSubGraphsRecursively() const;

	/**
	 * Get all the sub graphs that are directly under this graph.
	 * @return All the sub graphs that are directly under this graph
	 */
	TArray<UJointEdGraph*> GetDirectSubGraphs() const;
	
public:

	static TArray<UJointEdGraph*> GetAllGraphsFrom(UEdGraph* InGraph);
	
	static TArray<UJointEdGraph*> GetAllGraphsFrom(const UJointManager* InJointManager);

public:

	/**
	 * Set the toolkit of the graph.
	 * @param InToolkit Toolkit to set
	 */
	void SetToolkit(const TSharedPtr<FJointEditorToolkit>& InToolkit);
	
	/**
	 * Get the toolkit of the graph.
	 * @return Toolkit of the graph
	 */
	const TWeakPtr<FJointEditorToolkit>& GetToolkit() const { return Toolkit; }


public:

	//Discard unnecessary data
	void UpdateDebugData();

public:
	
	FORCEINLINE class UJointManager* GetJointManager() const {return JointManager;}


public:
	
	//Called when this graph is newly created.
	virtual void OnGraphObjectCreated();

public:

	/**
	 * Called when this graph is started to be opened as a document on a toolkit and started to be edited.
	 */
	virtual void OnLoaded();
	
	/**
	 * Called when this graph is saved.
	 */
	virtual void OnSave();

	/**
	 * Called when this graph's document has been closed on a toolkit.
	 */
	virtual void OnClosed();

public:
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	
	/**
	 * Notify the change on the graph and request visual representation rebuild. Consider using NotifyGraphRequestUpdate() instead if you don't want to rebuild the graph panel and graph node slates.
	 */
	virtual void NotifyGraphChanged() override;

	virtual void NotifyGraphChanged(const FEdGraphEditAction& InAction) override;
	
	/**
	 * Broadcast a notification whenever the graph has changed so need to update the following sub-objects (such as a tree view).
	 * Also update the graph object itself to meet changes in the data.
	 */
	void NotifyGraphRequestUpdate();
	
public:

	/**
	 * Update the graph's data for the changes on its properties.
	 * This will not do anything when the graph is locked, and also lock the graph during the process to avoid multiple updates.
	 */
	void UpdateGraph();

private:

	// Utility functions for the updating.
	
	void ResetGraphNodeSlates();
	void RecalculateNodeDepth();
	void FeedToolkitToGraphNodes();
	void NotifyNodeConnectionChanged();
	void BindEdNodeEvents();
	
	//Clear and reallocate all the node instance references of the graph nodes on this graph to Joint manager.
	void AllocateBaseNodesToJointManager();
	
	//Allocate all the node instance references of the graph nodes on this graph to Joint manager.
	static void AllocateThisGraphBaseNodesToJointManager(UJointManager* JointManager, UJointEdGraph* Graph);

	//Update class data of the provided graph node. Can choose whether to propagate to the children nodes.
	void UpdateClassDataForNode(UJointEdGraphNode* Node, const bool bPropagateToSubNodes = true);

	void GrabUnknownClassDataFromNode(UJointEdGraphNode* Node, const bool bPropagateToSubNodes = true);
	
	//Patch node instance from stored node class of the node.
	void TryReinstancingUnknownNodeClasses();
	
	//Patch node instance from stored node class of the node. Can choose whether to propagate to the children nodes.
	void PatchNodeInstanceFromStoredNodeClass(TObjectPtr<UEdGraphNode> TargetNode, const bool bPropagateToSubNodes = true);

public:

	// Duplication Related
	virtual void PreDuplicate(FObjectDuplicationParameters& DupParams) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;

	//
	virtual void PrepareForCopy();
	virtual void PostCopy();

public:

	//Update class data of the graph nodes.
	void UpdateClassData();
	
	//Grab Unknown class data from the graph
	void GrabUnknownClassDataFromGraph();
	
public:

	void ReallocateGraphPanelToGraphNodeSlates(TSharedPtr<SGraphPanel> GraphPanel);

private:
	
	void RecacheNodes();

public:

	//Update class data of the graph nodes.
	void CleanUpNodes();
	
	//Reconstruct the nodes on this graph.
	void ReconstructAllNodes(bool bPropagateUnder = false);

public:

	//Patch node pickers with invalid data.
	void PatchNodePickers();

public:

	//Functor execution
	
	/**
	 * Execute the function with the cached nodes.
	 * @param Func Function to execute
	 */
	void ExecuteForAllNodesInHierarchy(const TFunction<void(UEdGraphNode*)>& Func);

public:

	/**
	 * Initialize the compile result if it is not valid.
	 * for the sub graphs, it will use the root graph's compile result.
	 */
	void InitializeCompileResultIfNeeded();

	//Compile the graph.
	void CompileAllJointGraphFromRoot();

private:
	
	//Compile the provided graph node. Can choose whether to propagate to the children nodes.
	void CompileJointGraphForNode(UJointEdGraphNode* Node, const bool bPropagateToSubNodes = true);
	
	//Compile the graph.
	void CompileJointGraph();

public:
	
	struct FJointGraphCompileInfo
	{
		int NodeCount;
		double ElapsedTime = 0;

		FJointGraphCompileInfo(const int& InNodeCount, const double& InElapsedTime) : NodeCount(InNodeCount), ElapsedTime(InElapsedTime) {}
	};

	DECLARE_DELEGATE_OneParam(FOnCompileFinished, const FJointGraphCompileInfo&)
	//Internal delegate that inform the editor that the compilation has been finished.
	FOnCompileFinished OnCompileFinished;

public:

	/**
	 * Pointer to the MessageLogListing instance this graph has.
	 * This property will not be always valid so make sure the check whether it is valid or not first. It will be accessible only when the system need this.
	 * Joint 2.10 : sub graphs will not have their own message log listing - only the root graph will have it and sub graphs will use the root graph's one.
	 */
	TSharedPtr<class IMessageLogListing> CompileResultPtr;
	
public:

	UPROPERTY(Transient)
	bool bIsCompiling = false;
	
public:

	//Update sub node chains of the whole nodes - let the sub nodes' node instances have valid parent node reference based on the graph node's tree structure.
	void UpdateSubNodeChains();

public:

	/**
	 * Find a graph node for the provided node instance.
	 * @param NodeInstance Provided node instance for the search action.
	 * @return Found graph node instance
	 */
	UEdGraphNode* FindGraphNodeForNodeInstance(const UObject* NodeInstance);

public:

	/**
	 * Get cached Joint node instances. This action includes the sub nodes. (Sub nodes are not being stored in the Joint manager directly.)
	 */
	TSet<TWeakObjectPtr<UObject>> GetCachedJointNodeInstances(const bool bForceRecache = false);
	
	/**
	 * Get cached Joint graph nodes. This action includes the sub nodes. (Sub nodes are not being stored in the Joint manager directly.)
	 */
	TSet<TWeakObjectPtr<UJointEdGraphNode>> GetCachedJointGraphNodes(const bool bForceRecache = false);

public:

	/**
	 * Cache Joint node instances. This action includes the sub nodes. (Sub nodes are not being stored in the Joint manager directly.)
	 */
	void CacheJointNodeInstances();
	
	/**
	 * Cache Joint graph nodes. This action includes the sub nodes. (Sub nodes are not being stored in the Joint manager directly.)
	 */
	void CacheJointGraphNodes();

	
private:

	/**
	 * Cached node instances for the search action. This variable includes the sub nodes. (Sub nodes are not being stored in the Joint manager directly.)
	 */
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<UObject>> CachedJointNodeInstances;
	
	/**
	 * Cached graph nodes for the search action. This variable includes the sub nodes. (Sub nodes are not being stored in the Joint manager directly.)
	 */
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<UJointEdGraphNode>> CachedJointGraphNodes;

private:
	
	FCriticalSection CachedJointNodeInstancesMutex;

	FCriticalSection CachedJointGraphNodesMutex;

public:
	
	virtual void OnNodesPasted(const FString& ImportStr);

public:
	
	virtual bool CanRemoveNestedObject(UObject* TestObject) const;
	
	void RemoveOrphanedNodes();
	
	virtual void OnNodeInstanceRemoved(UObject* NodeInstance);

public:
	
	/**
	 * Check whether the graph is locked for updates.
	 * @return true if the graph is locked for updates, false otherwise
	 */
	const bool& IsLocked() const;

	/**
	 * Lock the graph for updates. While the graph is locked, it will not respond to any update requests.
	 */
	void LockUpdates();

	/**
	 * Unlock the graph for updates.
	 */
	void UnlockUpdates();

public:
	
	UPROPERTY()
	bool bIsLocked = false;

public:

#if WITH_EDITOR
	
	/**
	 * Starts caching of platform specific data for the target platform
	 * Called when cooking before serialization so that object can prepare platform specific data
	 * Not called during normal loading of objects
	 * 
	 * @param	TargetPlatform	target platform to cache platform specific data for
	 */
	virtual void BeginCacheForCookedPlatformData( const ITargetPlatform* TargetPlatform ) override;
	
#endif


public:
	
#if WITH_EDITOR

	UPROPERTY(Category="Developer", VisibleAnywhere, Transient, DuplicateTransient, SkipSerialization)
	TArray<TObjectPtr<class UEdGraphNode>> Nodes_Captured;
	
	UPROPERTY(Category="Developer", VisibleAnywhere, Transient, DuplicateTransient, SkipSerialization)
	TArray<FJointNodeDebugData> DebugData_Captured;
	
	UPROPERTY(Category="Developer", VisibleAnywhere, Transient, DuplicateTransient, SkipSerialization)
	TArray<FJointNodeDebugData> Schema_Captured;

#endif
	
public:
	
	static UJointEdGraph* CreateNewJointGraph(UObject* InOuter, UJointManager* InJointManager, const FName& GraphName);
	
};
