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


class UJointNodeBase;
class UJointManager;
class IPersonaPreviewScene;
class IJointTreeItem;

/** Options for skeleton building */
struct JOINTEDITOR_API FJointPropertyTreeBuilderArgs
{
	FJointPropertyTreeBuilderArgs(
		const bool bShowJointManagers = true,
		const bool bShowNodes = true,
		const bool bShowProperties = true)
		: bShowJointManagers(bShowJointManagers)
		, bShowNodes(bShowNodes)
		, bShowProperties(bShowProperties)
	{}

	bool bShowJointManagers;
	bool bShowNodes;
	bool bShowProperties;
};

class JOINTEDITOR_API FJointTreeBuilder : public IJointTreeBuilder
{
public:
	
	FJointTreeBuilder(const FJointPropertyTreeBuilderArgs& InBuilderArgs);

	~FJointTreeBuilder();

public:

	virtual void Initialize(const TSharedRef<class SJointTree>& InTree, FOnFilterJointPropertyTreeItem InOnFilterSkeletonTreeItem) override;
	
	virtual void Build(FJointTreeBuilderOutput& Output) override;
	virtual void Filter(const FJointPropertyTreeFilterArgs& InArgs, const TArray<TSharedPtr<class IJointTreeItem>>& InItems, TArray<TSharedPtr<class IJointTreeItem>>& OutFilteredItems) override;
	virtual EJointTreeFilterResult FilterItem(const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<class IJointTreeItem>& InItem) override;

	virtual bool IsShowingJointManagers() const override;
	virtual bool IsShowingNodes() const override;
	virtual bool IsShowingProperties() const override;

protected:

	void AddJointManagers(FJointTreeBuilderOutput& Output);
	
	void AddNodes(FJointTreeBuilderOutput& Output);

	void AddProperties(FJointTreeBuilderOutput& Output);

	TSharedPtr<IJointTreeItem> CreateManagerTreeItem(TWeakObjectPtr<UJointManager> ManagerPtr);

	TSharedPtr<IJointTreeItem> CreateNodeTreeItem(UJointNodeBase* NodePtr);

	TSharedPtr<IJointTreeItem> CreatePropertyTreeItem(FProperty* Property, UObject* InObject);
	
	/** Helper function for filtering */
	EJointTreeFilterResult FilterRecursive(const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<IJointTreeItem>& InItem, TArray<TSharedPtr<IJointTreeItem>>& OutFilteredItems);

protected:
	/** Options for building */
	FJointPropertyTreeBuilderArgs BuilderArgs;

	/** Delegate used for filtering */
	FOnFilterJointPropertyTreeItem OnFilterJointPropertyTreeItem;

	/** The tree we will build against */
	TWeakPtr<class SJointTree> TreePtr;

public:

	virtual const bool& IsBuilding() const override;

public:

	bool bIsBuilding = false;

public:

	virtual void SetShouldAbandonBuild(bool bNewInShouldAbandonBuild) override;

	virtual const bool GetShouldAbandonBuild() const override;

protected:

	bool bShouldAbandonBuild = false;
	
};
