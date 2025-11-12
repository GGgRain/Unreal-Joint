//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointManager.h"
#include "SearchTree/Builder/JointTreeBuilder.h"
#include "SearchTree/Item/IJointTreeItem.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class UJointEdGraphNode;
class FJointEditorToolkit;
struct FJointPropertyTreeFilterArgs;
struct FJointPropertyTreeBuilderArgs;
class IJointTreeItem;
class UTexture2D;


/**
 * A tree list view.
 */

class JOINTEDITOR_API SJointTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJointTree)
	{
	}
		SLATE_ATTRIBUTE(FJointPropertyTreeBuilderArgs, BuilderArgs)
		SLATE_ATTRIBUTE(FJointPropertyTreeFilterArgs, FilterArgs)
	SLATE_END_ARGS()
	
public:

	struct Columns
	{
		static const FName Name;
		static const FName Value;
		static const FName Compile;
	};
	
public:
	void Construct(const FArguments& InArgs);

	~SJointTree();

public:

	/** Creates the tree control and then populates */
	void CreateTreeColumns();

	void OnColumnSortModeChanged(EColumnSortPriority::Type SortPriority, const FName& ColumnId,
	                             EColumnSortMode::Type InSortMode);
	
	void BuildFromJointManagers(const TArray<TWeakObjectPtr<UJointManager>>& JointManagersToShow);

	/** Apply filtering to the tree */
	void ApplyFilter();

	void OnFilterDataChanged();

public:

	void OnJointTreeBuildStarted();
	void OnJointTreeBuildFinished(const FJointTreeBuilderOutput InOutput);
	void OnJointTreeBuildCancelled();

public:

	TSharedPtr<SWidget> PopulateLoadingStateWidget();

	void ShowLoadingStateWidget();

	void HideLoadingStateWidget();

public:
	
	TSharedPtr<STreeView<TSharedPtr<IJointTreeItem>>> TreeView;

	/** A tree of unfiltered items */
	TArray<TSharedPtr<class IJointTreeItem>> Items;

	/** A "mirror" of the tree as a flat array for easier searching */
	TArray<TSharedPtr<class IJointTreeItem>> LinearItems;

	/** Filtered view of the skeleton tree. This is what is actually used in the tree widget */
	TArray<TSharedPtr<class IJointTreeItem>> FilteredItems;
	
public:

	TSharedPtr<class FJointTreeFilter> Filter;
	
	TSharedPtr<class FJointTreeBuilder> Builder;

	TSharedPtr<class FTextFilterExpressionEvaluator> TextFilterPtr;

public:

	TSharedPtr<class SWidget> LoadingStateSlate;

public:
	
	FName SortByColumn;
	EColumnSortMode::Type SortMode;
	
public:
	
	/** Delegate handler for when the tree needs refreshing */
	void HandleTreeRefresh();
	
	EJointTreeFilterResult HandleFilterJointPropertyTreeItem(const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<class IJointTreeItem>& InItem);

public:
	
	//Tree callback
	TSharedRef<ITableRow> HandleGenerateRow(TSharedPtr<IJointTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	
	void HandleGetChildren(TSharedPtr<IJointTreeItem> Item, TArray<TSharedPtr<IJointTreeItem>>& OutChildren);

public:

	TAttribute<FJointPropertyTreeFilterArgs> FilterArgsAttr;
	TAttribute<FJointPropertyTreeBuilderArgs> BuilderArgsAttr;

private:

	FText QueryInlineFilterText;
	FText HighlightInlineFilterText;

public:

	const FText& GetQueryInlineFilterText();
	const FText& GetHighlightInlineFilterText();
	
	void SetQueryInlineFilterText(const FText& NewFilterText);
	void SetHighlightInlineFilterText(const FText& NewFilterText);

public:

	void JumpToHyperlink(UObject* Obj);

public:

	FJointTreeBuilderOutput CurrentOutput;

};

