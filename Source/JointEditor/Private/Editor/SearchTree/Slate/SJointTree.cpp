//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "SearchTree/Slate/SJointTree.h"

#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "SearchTree/Slate/SJointTreeFilter.h"

#include "JointEditorToolkit.h"
#include "JointEdUtils.h"
#include "Async/Async.h"
#include "SearchTree/Builder/JointTreeBuilder.h"

#include "EdGraph/EdGraph.h"
#include "Filter/JointTreeFilter.h"
#include "Item/JointTreeItem_Graph.h"
#include "Misc/ScopeTryLock.h"
#include "SearchTree/Item/JointTreeItem_Node.h"
#include "Preferences/PersonaOptions.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Views/STreeView.h"

const FName SJointTree::Columns::Name("Name");
const FName SJointTree::Columns::Value("Value");


#define LOCTEXT_NAMESPACE "JointPropertyList"


void SJointTree::Construct(const FArguments& InArgs)
{
	if (!Builder.IsValid())
	{
		Builder = MakeShareable(new FJointTreeBuilder());
		Builder->OnJointTreeBuildStartedDele.BindSP(this, &SJointTree::OnJointTreeBuildStarted);
		Builder->OnJointTreeBuildFinishedDele.BindSP(this, &SJointTree::OnJointTreeBuildFinished);
		Builder->OnJointTreeBuildCancelledDele.BindSP(this, &SJointTree::OnJointTreeBuildCancelled);
	}
	if (!Filter.IsValid()) { Filter = MakeShareable(new FJointTreeFilter(SharedThis(this))); }

	BuilderArgsAttr = InArgs._BuilderArgs;
	FilterArgsAttr = InArgs._FilterArgs;

	Builder->Initialize(SharedThis(this), FOnFilterJointPropertyTreeItem::CreateSP(this, &SJointTree::HandleFilterJointPropertyTreeItem));

	Filter->OnJointFilterChanged.AddSP(this, &SJointTree::OnFilterDataChanged);

	TextFilterPtr = MakeShareable(new FTextFilterExpressionEvaluator(ETextFilterExpressionEvaluatorMode::BasicString));
	CreateTreeColumns();
}

SJointTree::~SJointTree()
{
	if (Builder.IsValid())
	{
		Builder->AbandonBuild();
	}
}

void SJointTree::CreateTreeColumns()
{
	TreeView = SNew(STreeView<TSharedPtr<IJointTreeItem>>)
		.OnGenerateRow(this, &SJointTree::HandleGenerateRow)
		.OnGetChildren(this, &SJointTree::HandleGetChildren)
		.TreeItemsSource(&FilteredItems)
		.AllowOverscroll(EAllowOverscroll::Yes)
		.HeaderRow(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(SJointTree::Columns::Name)
			.DefaultLabel(LOCTEXT("RowTitle_Name", "Name"))
			.OnSort(this, &SJointTree::OnColumnSortModeChanged)
			.FillWidth(0.5)
			+ SHeaderRow::Column(SJointTree::Columns::Value)
			.DefaultLabel(LOCTEXT("RowTitle_Value", "Value"))
			.OnSort(this, &SJointTree::OnColumnSortModeChanged)
			.FillWidth(0.5)
		);

	this->ChildSlot
	    .VAlign(VAlign_Fill)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		[
			SNew(SJointTreeFilter)
			.Filter(Filter)
		]
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				TreeView.ToSharedRef()
			]
			+ SOverlay::Slot()
			[
				PopulateLoadingStateWidget().ToSharedRef()
			]
		]
	];
}

void SJointTree::OnColumnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId,
                                         const EColumnSortMode::Type InSortMode)
{
	SortByColumn = ColumnId;
	SortMode = InSortMode;
}

void SJointTree::BuildFromJointManagers(const TArray<TWeakObjectPtr<UJointManager>>& JointManagersToShow)
{
	if (!Builder.IsValid()) return;

	ShowLoadingStateWidget();
	
	Builder->RequestBuild(JointManagersToShow);
}

void SJointTree::ApplyFilter()
{
	if (Builder->GetShouldAbandonBuild()) return;
	
	const FString ExtractedFilterItem = Filter->ExtractFilterItems().ToString();

	FString QueryText = QueryInlineFilterText.ToString();
	QueryText = QueryText.Replace(TEXT(" "), TEXT("_"));
	
	TextFilterPtr->SetFilterText(
		FText::FromString(
			!QueryText.IsEmpty() && !ExtractedFilterItem.IsEmpty()
				? QueryText + " && " + ExtractedFilterItem
				: !QueryText.IsEmpty() && ExtractedFilterItem.IsEmpty()
				? QueryText
				: QueryText.IsEmpty() && !ExtractedFilterItem.IsEmpty()
				? ExtractedFilterItem
				: ""
		)
	);

	FilteredItems.Empty();

	FJointPropertyTreeFilterArgs Args = FilterArgsAttr.Get();
	Args.TextFilter = TextFilterPtr;
	Builder->Filter(Args, Items, FilteredItems);

	for (TSharedPtr<IJointTreeItem>& Item : LinearItems)
	{
		if (Item->GetFilterResult() > EJointTreeFilterResult::Hidden)
		{
			TreeView->SetItemExpansion(Item, true);
		}
	}

	HideLoadingStateWidget();
	
	HandleTreeRefresh();
}

void SJointTree::OnFilterDataChanged()
{
	ApplyFilter();
}

void SJointTree::OnJointTreeBuildStarted()
{
	ShowLoadingStateWidget();
}

void SJointTree::OnJointTreeBuildFinished(const FJointTreeBuilderOutput InOutput)
{
	Items = InOutput.Items;
	LinearItems = InOutput.LinearItems;

	ApplyFilter();
	
	HideLoadingStateWidget();
}

void SJointTree::OnJointTreeBuildCancelled()
{
	HideLoadingStateWidget();
}

TSharedPtr<SWidget> SJointTree::PopulateLoadingStateWidget()
{
	if (!LoadingStateSlate.IsValid())
	{
		LoadingStateSlate = SNew(SBorder)
			.Visibility(EVisibility::Collapsed)
			.Padding(0)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Solid"))
			.BorderBackgroundColor(FLinearColor(0, 0, 0, 0.1))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SCircularThrobber)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LoadingStateText", "Loading entries..."))
				]
			];
	}

	return LoadingStateSlate;
}


void SJointTree::ShowLoadingStateWidget()
{
	PopulateLoadingStateWidget();

	if (LoadingStateSlate.IsValid())
	{
		LoadingStateSlate->SetVisibility(EVisibility::Visible);
	}
}

void SJointTree::HideLoadingStateWidget()
{
	if (LoadingStateSlate.IsValid())
	{
		LoadingStateSlate->SetVisibility(EVisibility::Collapsed);
	}
}

void SJointTree::HandleTreeRefresh()
{
	if (!TreeView) return;

	TreeView->RequestTreeRefresh();
}


TSharedRef<ITableRow> SJointTree::HandleGenerateRow(TSharedPtr<IJointTreeItem> Item,
                                                    const TSharedRef<STableViewBase>& OwnerTable)
{
	check(Item.IsValid());

	return Item->MakeTreeRowWidget(
		OwnerTable
		, TAttribute<FText>::CreateLambda([this]() { return GetHighlightInlineFilterText(); }));
}

void SJointTree::HandleGetChildren(TSharedPtr<IJointTreeItem> Item,
                                   TArray<TSharedPtr<IJointTreeItem>>& OutChildren)
{
	check(Item.IsValid());
	OutChildren = Item->GetFilteredChildren();
}

const FText& SJointTree::GetQueryInlineFilterText()
{
	return QueryInlineFilterText;
}

const FText& SJointTree::GetHighlightInlineFilterText()
{
	return HighlightInlineFilterText;
}

void SJointTree::SetQueryInlineFilterText(const FText& NewFilterText)
{
	QueryInlineFilterText = NewFilterText;

	ApplyFilter();
}

void SJointTree::SetHighlightInlineFilterText(const FText& NewFilterText)
{
	HighlightInlineFilterText = NewFilterText;
}

void SJointTree::JumpToHyperlink(UObject* Obj)
{
	FJointEditorToolkit* ToolkitPtr = nullptr;

	if (!Obj) return;

	if (Cast<UJointNodeBase>(Obj) && Cast<UJointNodeBase>(Obj)->GetJointManager())
	{
		FJointEdUtils::OpenEditorFor(Cast<UJointNodeBase>(Obj)->GetJointManager(), ToolkitPtr);

		if (ToolkitPtr != nullptr) ToolkitPtr->JumpToHyperlink(Obj);
	}
	else if (Cast<UJointManager>(Obj))
	{
		FJointEdUtils::OpenEditorFor(Cast<UJointNodeBase>(Obj)->GetJointManager(), ToolkitPtr);
	}else if (Cast<UJointEdGraph>(Obj) && Cast<UJointEdGraph>(Obj)->GetJointManager())
	{
		FJointEdUtils::OpenEditorFor(Cast<UJointEdGraph>(Obj)->GetJointManager(), ToolkitPtr);

		if (ToolkitPtr != nullptr) ToolkitPtr->JumpToHyperlink(Obj);
	}else if (Cast<UJointEdGraphNode>(Obj) && Cast<UJointEdGraphNode>(Obj)->GetJointManager())
	{
		FJointEdUtils::OpenEditorFor(Cast<UJointEdGraphNode>(Obj)->GetJointManager(), ToolkitPtr);

		if (ToolkitPtr != nullptr) ToolkitPtr->JumpToHyperlink(Obj);
	}
}

EJointTreeFilterResult SJointTree::HandleFilterJointPropertyTreeItem(
	const FJointPropertyTreeFilterArgs& InArgs
	, const TSharedPtr<class IJointTreeItem>& InItem)
{
	EJointTreeFilterResult Result = EJointTreeFilterResult::ShownDescendant;

	if (!InItem) return Result;

	if (InArgs.TextFilter.IsValid())
	{
		if (!(InArgs.TextFilter->GetFilterType() == ETextFilterExpressionType::Empty || InArgs.TextFilter->GetFilterType() == ETextFilterExpressionType::Invalid))
		{
			Result = InArgs.TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(InItem->GetFilterString()))
				         ? EJointTreeFilterResult::ShownHighlighted
				         : EJointTreeFilterResult::Hidden;

			return Result;
		}
	}

	if (InItem->IsOfType<FJointTreeItem_Graph>())
	{
		TSharedRef<FJointTreeItem_Graph> GraphItem = StaticCastSharedRef<FJointTreeItem_Graph>(InItem.ToSharedRef());

		if (!GraphItem->GetObject()) return EJointTreeFilterResult::Hidden;

		UObject* Obj = GraphItem->GetObject();

		if (!InArgs.GraphsToShow.IsEmpty())
		{
			bool bFound = InArgs.GraphsToShow.Contains(Obj);

			//UE_LOG(LogJointEditor, Log, TEXT("Filtering graph: %s, found: %s"), *Obj->GetName(), bFound ? TEXT("true") : TEXT("false"));
			// if the graph is not in the list, hide it
			return bFound ? EJointTreeFilterResult::Shown : EJointTreeFilterResult::Hidden;
		}

		return EJointTreeFilterResult::Shown;
	}
	else
	{
		if (InItem->GetParent() && InItem->GetParent()->GetFilterResult() == EJointTreeFilterResult::Hidden)
		{
			return EJointTreeFilterResult::Hidden;
		}
	}


	return Result;
}


#undef LOCTEXT_NAMESPACE
