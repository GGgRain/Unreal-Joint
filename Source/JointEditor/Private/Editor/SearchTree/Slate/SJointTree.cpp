//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "SearchTree/Slate/SJointTree.h"
#include "SearchTree/Slate/SJointTreeFilter.h"

#include "JointEditorToolkit.h"
#include "JointEdUtils.h"
#include "SearchTree/Builder/JointTreeBuilder.h"

#include "EdGraph/EdGraph.h"
#include "Filter/JointTreeFilter.h"
#include "SearchTree/Item/JointTreeItem_Node.h"
#include "Preferences/PersonaOptions.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Views/STreeView.h"

const FName SJointTree::Columns::Name("Name");
const FName SJointTree::Columns::Value("Value");


#define LOCTEXT_NAMESPACE "JointPropertyList"


void SJointTree::Construct(const FArguments& InArgs)
{
	if (!Builder.IsValid()) { Builder = MakeShareable(new FJointTreeBuilder(InArgs._BuilderArgs)); }
	if (!Filter.IsValid()) { Filter = MakeShareable(new FJointTreeFilter(SharedThis(this))); }

	Builder->Initialize(SharedThis(this),
	                    FOnFilterJointPropertyTreeItem::CreateSP(
		                    this, &SJointTree::HandleFilterJointPropertyTreeItem));

	Filter->OnJointFilterChanged.AddSP(this, &SJointTree::OnFilterDataChanged);

	TextFilterPtr = MakeShareable(new FTextFilterExpressionEvaluator(ETextFilterExpressionEvaluatorMode::BasicString));

	CreateTreeColumns();
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
				TreeView.ToSharedRef()
			]
		];
}

void SJointTree::OnColumnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId,
                                            const EColumnSortMode::Type InSortMode)
{
	SortByColumn = ColumnId;
	SortMode = InSortMode;

	SortColumns();
}


void SJointTree::BuildFromJointManagers()
{
	// save selected items
	Items.Empty();
	LinearItems.Empty();
	FilteredItems.Empty();

	FJointTreeBuilderOutput Output(Items, LinearItems);
	Builder->Build(Output);
	ApplyFilter();
}

void SJointTree::ApplyFilter()
{

	const FString ExtractedFilterItem = Filter->ExtractFilterItems().ToString();
	
	TextFilterPtr->SetFilterText(
		FText::FromString(
			!QueryInlineFilterText.IsEmpty() && !ExtractedFilterItem.IsEmpty() ? QueryInlineFilterText.ToString() + " && " + ExtractedFilterItem :
			!QueryInlineFilterText.IsEmpty() && ExtractedFilterItem.IsEmpty() ? QueryInlineFilterText.ToString() :
			QueryInlineFilterText.IsEmpty() && !ExtractedFilterItem.IsEmpty() ? ExtractedFilterItem : ""
		)
	);

	FilteredItems.Empty();

	FJointPropertyTreeFilterArgs FilterArgs(TextFilterPtr);
	FilterArgs.bFlattenHierarchyOnFilter = GetDefault<UPersonaOptions>()->bFlattenSkeletonHierarchyWhenFiltering;
	Builder->Filter(FilterArgs, Items, FilteredItems);


	for (TSharedPtr<IJointTreeItem>& Item : LinearItems)
	{
		if (Item->GetFilterResult() > EJointTreeFilterResult::Hidden)
		{
			TreeView->SetItemExpansion(Item, true);
		}
	}

	HandleTreeRefresh();
}

void SJointTree::OnFilterDataChanged()
{
	ApplyFilter();
}


void SJointTree::SortColumns()
{
	/*
	if( SortByColumn == SJointTree::Columns::Name )
	{
		if( SortMode == EColumnSortMode::Ascending )
		{
			struct FCompareEventsByName
			{
				FORCEINLINE bool operator()( const TSharedPtr<FVisualizerEvent> A, const TSharedPtr<FVisualizerEvent> B ) const { return A->EventName < B->EventName; }
			};
			Items.Sort( FCompareEventsByName() );
		}
		else if( SortMode == EColumnSortMode::Descending )
		{
			struct FCompareEventsByNameDescending
			{
				FORCEINLINE bool operator()( const TSharedPtr<FVisualizerEvent> A, const TSharedPtr<FVisualizerEvent> B ) const { return B->EventName < A->EventName; }
			};
			Events.Sort( FCompareEventsByNameDescending() );
		}
	}
	*/
}

void SJointTree::HandleTreeRefresh()
{
	if(!TreeView) return;
	
	TreeView->RequestTreeRefresh();
}

void SJointTree::SetInitialExpansionState()
{
	if(!TreeView) return;
	
	for (TSharedPtr<IJointTreeItem>& Item : LinearItems)
	{
		TreeView->SetItemExpansion(Item, Item->IsInitiallyExpanded());
	}
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

	if(!Obj) return;
	
	if(Cast<UJointNodeBase>(Obj) && Cast<UJointNodeBase>(Obj)->GetJointManager())
	{
		FJointEdUtils::OpenEditorFor(Cast<UJointNodeBase>(Obj)->GetJointManager(), ToolkitPtr);

		if (ToolkitPtr != nullptr) ToolkitPtr->JumpToHyperlink(Obj);
		
	}else if(Cast<UJointManager>(Obj))
	{
		FJointEdUtils::OpenEditorFor(Cast<UJointNodeBase>(Obj)->GetJointManager(), ToolkitPtr);
	}
}

EJointTreeFilterResult SJointTree::HandleFilterJointPropertyTreeItem(
	const FJointPropertyTreeFilterArgs& InArgs
	, const TSharedPtr<class IJointTreeItem>& InItem)
{
	EJointTreeFilterResult Result = EJointTreeFilterResult::ShownDescendant;

	if (InItem && InArgs.TextFilter.IsValid())
	{
		Result = InArgs.TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(InItem->GetFilterString())) ? EJointTreeFilterResult::ShownHighlighted : EJointTreeFilterResult::Hidden;
	}

	return Result;
}


#undef LOCTEXT_NAMESPACE
