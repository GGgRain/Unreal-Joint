//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Node/JointEdGraphNode.h"
#include "UnrealEdGlobals.h"
#include "Editor/Debug/JointNodeDebugData.h"
#include "EdGraph/EdGraph.h"
#include "JointEdGraph.generated.h"


class FJointEditorToolkit;
DECLARE_MULTICAST_DELEGATE(FOnGraphRequestUpdate);

UCLASS()
class JOINTEDITOR_API UJointEdGraph : public UEdGraph
{
	GENERATED_BODY()
	
public:

	UPROPERTY()
	class UJointManager* JointManager;
	
public:
	
	/**
	 * Debug data for the graph nodes.
	 * Only contains the data for the nodes that have a debug data.
	 * Debug data will not be duplicated to the other assets - because it literally don't need to.
	 */
	UPROPERTY(DuplicateTransient)
	TArray<FJointNodeDebugData> DebugData;

public:

	/**
	 * Optional toolkit of the graph that is currently editing the graph.
	 */
	TWeakPtr<FJointEditorToolkit> Toolkit;

public:

	/**
	 * Pointer to the MessageLogListing instance this graph has.
	 * This property will not be always valid so make sure the check whether it is valid or not first. It will be accessible only when the system need this.
	 */
	TSharedPtr<class IMessageLogListing> CompileResultPtr;
	
public:

	//Discard unnecessary data
	void UpdateDebugData();

public:
	
	FORCEINLINE class UJointManager* GetJointManager() const {return JointManager;}
	
public:

	//Called whenever the toolkit loads this graph and start to open this graph. Can be used to initialize any features in the editor initialization stage if needed.
	virtual void Initialize();
	
	//Called when this graph is newly created.
	virtual void OnCreated();

	//Called when this graph is loaded from the disk and started to be edited.
	virtual void OnLoaded();
	
	//Called when this graph is saved.
	virtual void OnSave();

	//Called when this graph is closed on editing.
	virtual void OnClosed();

public:
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:

	/**
	 * Notify the change on the graph and request visual representation rebuild. Consider using NotifyGraphRequestUpdate() instead if you don't want to rebuild the graph panel and graph node slates.
	 */
	virtual void NotifyGraphChanged() override;

	void ResetGraphNodeSlates();
	void ResetNodeDepth();
	void ResetGraphNodeToolkits();

private:

	virtual void NotifyGraphChanged(const FEdGraphEditAction& InAction) override;
	void RecacheNodes();

public:

	/**
	 * Broadcast a notification whenever the graph has changed so need to update the following sub-objects (such as a tree view).
	 * Also update the graph object itself to meet changes in the data.
	 */
	void NotifyGraphRequestUpdate();

	/**
	 * Add a listener for OnGraphRequestUpdate events
	 */
	FDelegateHandle AddOnGraphRequestUpdateHandler( const FOnGraphRequestUpdate::FDelegate& InHandler );

	/**
	 * Remove a listener for OnGraphRequestUpdate events
	 */
	void RemoveOnGraphRequestUpdateHandler( FDelegateHandle Handle );

private:
	
	/**
	 * A delegate that broadcasts a notification whenever the graph has changed so need to update the following sub-objects (such as a tree view).
	 */
	FOnGraphRequestUpdate OnGraphRequestUpdate;

public:

	//Clear and reallocate all the node instance references of the graph nodes on this graph to Joint manager.
	void ReallocateGraphNodesToJointManager();

	//Reconstruct the nodes on this graph.
	void ReconstructAllNodes(bool bPropagateUnder = false);

public:

	//Update class data of the graph nodes.
	void CleanUpNodes();

public:


	//Update class data of the graph nodes.
	void UpdateClassData();

	//Update class data of the provided graph node. Can choose whether to propagate to the children nodes.
	void UpdateClassDataForNode(UJointEdGraphNode* Node, const bool bPropagateToSubNodes = true);
	
	//Patch node instance from stored node class of the node.
	void TryReinstancingUnknownNodeClasses();
	
	//Patch node instance from stored node class of the node. Can choose whether to propagate to the children nodes.
	void PatchNodeInstanceFromStoredNodeClass(TObjectPtr<UEdGraphNode> TargetNode, const bool bPropagateToSubNodes = true);

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

	void InitializeCompileResultIfNeeded();

	//Compile the provided graph node. Can choose whether to propagate to the children nodes.
	void CompileJointGraphForNode(UJointEdGraphNode* Node, const bool bPropagateToSubNodes = true);
	
	//Compile the graph.
	void CompileJointGraph();


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

	//Update sub node chains of the whole nodes.
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
	TSet<TSoftObjectPtr<UObject>> GetCacheJointNodeInstances(const bool bForce = false);
	
	/**
	 * Get cached Joint graph nodes. This action includes the sub nodes. (Sub nodes are not being stored in the Joint manager directly.)
	 */
	TSet<TSoftObjectPtr<UJointEdGraphNode>> GetCacheJointGraphNodes(const bool bForce = false);

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
	TSet<TSoftObjectPtr<UObject>> CachedJointNodeInstances;
	
	/**
	 * Cached graph nodes for the search action. This variable includes the sub nodes. (Sub nodes are not being stored in the Joint manager directly.)
	 */
	UPROPERTY(Transient)
	TSet<TSoftObjectPtr<UJointEdGraphNode>> CachedJointGraphNodes;

public:
	
	virtual void OnNodesPasted(const FString& ImportStr);
	
	void BindEdNodeEvents();

public:
	
	virtual bool CanRemoveNestedObject(UObject* TestObject) const;

	void RemoveOrphanedNodes();
	
	virtual void OnNodeInstanceRemoved(UObject* NodeInstance);

public:

	const bool& IsLocked() const;
	void LockUpdates();
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
	
};
