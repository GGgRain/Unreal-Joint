//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IJointTreeBuilder.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"


class UJointEdGraph;
class UJointNodeBase;
class UJointManager;
class IPersonaPreviewScene;
class IJointTreeItem;

/** Options for skeleton building */
struct JOINTEDITOR_API FJointPropertyTreeBuilderArgs
{
	FJointPropertyTreeBuilderArgs() : bShowJointManagers(true), bShowGraphs(true), bShowNodes(true), bShowProperties(true)
	{
	}

	FJointPropertyTreeBuilderArgs(
		const bool bShowJointManagers,
		const bool bShowGraphs,
		const bool bShowNodes,
		const bool bShowProperties)
		: bShowJointManagers(bShowJointManagers)
		, bShowGraphs(bShowGraphs)
		, bShowNodes(bShowNodes)
		, bShowProperties(bShowProperties)
	{}

	bool bShowJointManagers;
	bool bShowGraphs;
	bool bShowNodes;
	bool bShowProperties;
};

class JOINTEDITOR_API FJointTreeBuilder : public IJointTreeBuilder, public TSharedFromThis<FJointTreeBuilder>
{
public:
	
	FJointTreeBuilder();
	~FJointTreeBuilder();

public:

	virtual void Initialize(const TSharedRef<class SJointTree>& InTree, FOnFilterJointPropertyTreeItem InOnFilterJointPropertyTreeItem) override;
	
	virtual void Build(FJointTreeBuilderOutput& Output) override;
	virtual void Filter(const FJointPropertyTreeFilterArgs& InArgs, const TArray<TSharedPtr<class IJointTreeItem>>& InItems, TArray<TSharedPtr<class IJointTreeItem>>& OutFilteredItems) override;
	virtual EJointTreeFilterResult FilterItem(const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<class IJointTreeItem>& InItem) override;


public:

	// Completely reworked on the async build system
	// call RequestBuild to start building the tree - it will call OnJointTreeBuildFinished or OnJointTreeBuildCancelledDele when it's finished.
	// call AbandonBuild to abandon the current build.

public:

	bool bUseMultithreading = true;

public:

	/**
	 * Request the builder to build the tree. (threadsafe)
	 * Use OnJointTreeBuildFinished to attach action after the build is finished.
	 */
	virtual void RequestBuild(TArray<TWeakObjectPtr<UJointManager>> InJointManagersToShow);

public:

	void RunBuildSync();

	void RunBuildAsync();

	
public:

	/**
	 * Request to abandon the current build. (threadsafe)
	 * Use OnJointTreeBuildCancelled to attach action after the build is cancelled.
	 */
	void AbandonBuild();
	void WaitForBuildToFinish();

private:
	
	void CleanUpCollectedReferences();
	
	void CollectReferencesToBuild(const TArray<TWeakObjectPtr<UJointManager>>& InJointManagers);

public:

	virtual void SetShouldAbandonBuild(bool bNewInShouldAbandonBuild) override;

	virtual const bool GetShouldAbandonBuild() const override;

protected:

	bool bShouldAbandonBuild = false;

	bool bIsBuilding = false;

public:

	void SetIsNewBuildRequested(const bool bNewInIsNewBuildRequested);

	bool GetIsNewBuildRequested() const;

protected:
	
	bool bIsNewBuildRequested = false;

public:

	FCriticalSection BuildMutex;

private:

	void OnJointTreeBuildStarted();
	void OnJointTreeBuildFinished(const FJointTreeBuilderOutput Output);
	void OnJointTreeBuildCancelled();
	void OnCleanUpBlocking();

public:

	// Threadsafe queued jointmanagers to build with - when a multiple build request comes in while a build is in progress, it will be queued here.
	TArray<TWeakObjectPtr<UJointManager>> QueuedBuildTargetJointManagers;

public:
	
	//Queued jointmanagers to build with.
	TArray<UJointManager*> BuildTargetJointManagers;
	/**
	 * Every nodes on the graphs - including sub nodes and fragments, comment, etc.
	 */
	TArray<UEdGraphNode*> BuildTargetEditorNodes;
	TArray<UJointEdGraph*> BuildTargetGraphs;

protected:

	void ReserveJointManagerInfo(TArray<FJointTreeJointManagerInfo>& ManagerInfos);
	
	void ReserveGraphInfo(TArray<FJointTreeGraphInfo>& Graphs);

	void ReserveNodeInfo(TArray<FJointTreeNodeInfo>& NodeInfos);

	void ReservePropertyInfo(TArray<FJointTreePropertyInfo>& Properties);

	
	void BuildJointManagerInfo(const FJointTreeJointManagerInfo& JointManagerInfo, FJointTreeBuilderOutput& Output);

	void BuildGraphInfo(const FJointTreeGraphInfo& GraphInfo, FJointTreeBuilderOutput& Output);
	
	void BuildNodeInfo(const FJointTreeNodeInfo& NodeInfo, FJointTreeBuilderOutput& Output);

	void BuildPropertyInfo(FJointTreePropertyInfo& PropertyInfo,FJointTreeBuilderOutput& Output);

	TSharedPtr<IJointTreeItem> CreateManagerTreeItem(TWeakObjectPtr<UJointManager> ManagerPtr);

	TSharedPtr<IJointTreeItem> CreateGraphTreeItem(UJointEdGraph* GraphPtr);
	
	TSharedPtr<IJointTreeItem> CreateNodeTreeItem(UEdGraphNode* NodePtr);

	TSharedPtr<IJointTreeItem> CreatePropertyTreeItem(FProperty* Property, UObject* InObject);
	
	/** Helper function for filtering */
	EJointTreeFilterResult FilterRecursive(const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<IJointTreeItem>& InItem, TArray<TSharedPtr<IJointTreeItem>>& OutFilteredItems);


public:

	FOnJointTreeBuildStarted OnJointTreeBuildStartedDele;
	FOnJointTreeBuildFinished OnJointTreeBuildFinishedDele;
	FOnJointTreeBuildCancelled OnJointTreeBuildCancelledDele;
	
protected:

	/** Delegate used for filtering */
	FOnFilterJointPropertyTreeItem OnFilterJointPropertyTreeItem;
	
	/** The tree we will build against */
	TWeakPtr<class SJointTree> TreePtr;
	
};
