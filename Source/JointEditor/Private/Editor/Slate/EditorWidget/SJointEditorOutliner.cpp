//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorWidget/SJointEditorOutliner.h"

#include "EdGraphSchema_K2_Actions.h"
#include "GraphEditorActions.h"
#include "JointEdGraphNode_Composite.h"
#include "JointEditorCommands.h"
#include "JointEditorLogChannels.h"
#include "JointEditorToolkit.h"
#include "JointEdUtils.h"
#include "JointManager.h"
#include "ScopedTransaction.h"
#include "EditorWidget/SJointGraphEditorActionMenu.h"
#include "EditorWidget/SJointGraphPalette.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 3, 0)
#include "GraphEditor/Private/GraphActionNode.h"
#else
#include "GraphActionNode.h"
#endif

#include "WorkflowOrientedApp/WorkflowTabManager.h"

#define LOCTEXT_NAMESPACE "JointEditorOutliner"

struct FGraphActionSort;

template <class SchemaActionType>
SchemaActionType* SelectionAsType(const TSharedPtr<SGraphActionMenu>& GraphActionMenu)
{
	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);

	SchemaActionType* Selection = NULL;

	TSharedPtr<FEdGraphSchemaAction> SelectedAction(SelectedActions.Num() > 0 ? SelectedActions[0] : NULL);
	if (SelectedAction.IsValid() &&
		SelectedAction->GetTypeId() == SchemaActionType::StaticGetTypeId())
	{
		// TODO Why not? StaticCastSharedPtr<>()

		Selection = (SchemaActionType*)SelectedActions[0].Get();
	}

	return Selection;
}

void SJointEditorOutliner::OnRequestRenameOnActionNode()
{
	// Attempt to rename in both menus, only one of them will have anything selected
	GraphActionMenu->OnRequestRenameOnActionNode();
}

void SJointEditorOutliner::Construct(const FArguments& InArgs)
{
	ToolKitPtr = InArgs._ToolKitPtr;

	SetCanTick(false);

	CommandList = MakeShareable(new FUICommandList);

	CommandList->Append(ToolKitPtr.Pin()->GetToolkitCommands());

	CommandList->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SJointEditorOutliner::OnRequestRenameOnActionNode),
		FCanExecuteAction::CreateSP(this, &SJointEditorOutliner::CanRequestRenameOnActionNode));
	
	CommandList->MapAction(FJointEditorCommands::Get().DeleteEntry,
	                       FExecuteAction::CreateSP(this, &SJointEditorOutliner::OnDeleteEntry),
	                       FCanExecuteAction::CreateSP(this, &SJointEditorOutliner::CanDeleteEntry));
	

	RebuildWidget();
}


bool SJointEditorOutliner::IsEditingMode() const
{
	TSharedPtr<FJointEditorToolkit> BlueprintEditorSPtr = ToolKitPtr.Pin();
	return BlueprintEditorSPtr.IsValid() && BlueprintEditorSPtr->IsInEditingMode();
}

void SJointEditorOutliner::OnDeleteEntry()
{
	if (FEdGraphSchemaAction_K2Graph* GraphAction = SelectionAsGraph())
	{
		OnDeleteGraph(GraphAction->EdGraph, GraphAction->GraphType);
	}
	
	Refresh();
}


void SJointEditorOutliner::OnDeleteGraph(UEdGraph* InGraph, EEdGraphSchemaAction_K2Graph::Type InGraphType)
{
	if (!ToolKitPtr.IsValid() || !ToolKitPtr.Pin()->GetJointManager()) return;
	
	if (InGraph && InGraph->bAllowDeletion)
	{
		if (const UEdGraphSchema* Schema = InGraph->GetSchema())
		{
			if (Schema->TryDeleteGraph(InGraph))
			{
				return;
			}
		}

		const FScopedTransaction Transaction( LOCTEXT("RemoveGraph", "Remove Graph") );
		ToolKitPtr.Pin()->GetJointManager()->Modify();

		InGraph->Modify();

		if (InGraphType == EEdGraphSchemaAction_K2Graph::Subgraph)
		{
			// Remove any composite nodes bound to this graph
			TArray<UJointEdGraphNode_Composite*> AllCompositeNodes;
			FJointEdUtils::GetAllNodesOfClass<UJointEdGraphNode_Composite>(ToolKitPtr.Pin()->GetJointManager(), AllCompositeNodes);

			const bool bDontRecompile = true;
			for (UJointEdGraphNode_Composite* CompNode : AllCompositeNodes)
			{
				if (CompNode->BoundGraph == InGraph)
				{
					FJointEdUtils::RemoveNode(CompNode);
				}
			}
		}

		FJointEdUtils::RemoveGraph(Cast<UJointEdGraph>(InGraph));
		
		ToolKitPtr.Pin()->CloseDocumentTab(InGraph);

		InGraph = NULL;
	}
}


bool SJointEditorOutliner::CanDeleteEntry() const
{
	// Cannot delete entries while not in editing mode
	if(!IsEditingMode())
	{
		return false;
	}

	if (FEdGraphSchemaAction_K2Graph* GraphAction = SelectionAsGraph())
	{
		return (GraphAction->EdGraph ? GraphAction->EdGraph->bAllowDeletion : false);
	}

	return false;
}

void SJointEditorOutliner::Refresh()
{
	GraphActionMenu->RefreshAllActions(/*bPreserveExpansion=*/ true);
}


TSharedRef<SWidget> SJointEditorOutliner::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	if (ToolKitPtr.IsValid()) return SNew(SJointGraphPaletteItem, InCreateData, ToolKitPtr.Pin());

	// The original code was designed to return a valid widget with a transient editor reference - TODO: make sure to handle this case properly,
	return SNullWidget::NullWidget;
}


void SJointEditorOutliner::GetChildGraphs(UEdGraph* InEdGraph, int32 const SectionId, TArray<TSharedPtr<FEdGraphSchemaAction_K2Graph>>& OutGraphActions, const FText& ParentCategory) const
{
	check(InEdGraph);

	// Grab display info
	FGraphDisplayInfo EdGraphDisplayInfo;
	if (const UEdGraphSchema* Schema = InEdGraph->GetSchema())
	{
		Schema->GetGraphDisplayInformation(*InEdGraph, EdGraphDisplayInfo);
	}
	const FText EdGraphDisplayName = EdGraphDisplayInfo.DisplayName;

	// Grab children graphs
	for (UEdGraph* Graph : InEdGraph->SubGraphs)
	{
		if (Graph == nullptr)
		{
			continue;
		}

		FGraphDisplayInfo ChildGraphDisplayInfo;
		if (const UEdGraphSchema* ChildSchema = Graph->GetSchema())
		{
			ChildSchema->GetGraphDisplayInformation(*Graph, ChildGraphDisplayInfo);
		}

		FText DisplayText = ChildGraphDisplayInfo.DisplayName;

		FText Category;
		if (!ParentCategory.IsEmpty())
		{
			Category = FText::Format(FText::FromString(TEXT("{0}|{1}")), ParentCategory, EdGraphDisplayName);
		}
		else
		{
			Category = EdGraphDisplayName;
		}

		const FName DisplayName = FName(*DisplayText.ToString());
		FText ChildTooltip = DisplayText;
		FText ChildDesc = MoveTemp(DisplayText);

		TSharedPtr<FEdGraphSchemaAction_K2Graph> NewChildAction = MakeShareable(
			new FEdGraphSchemaAction_K2Graph(
				EEdGraphSchemaAction_K2Graph::Subgraph,
				Category,
				MoveTemp(ChildDesc),
				MoveTemp(ChildTooltip),
				1,
				SectionId
			));

		NewChildAction->FuncName = DisplayName;
		NewChildAction->EdGraph = Graph;
		OutGraphActions.Add(NewChildAction);

		GetChildGraphs(Graph, NewChildAction->GetSectionID(), OutGraphActions, Category);
	}
}

void SJointEditorOutliner::CollectAllActions(FGraphActionListBuilderBase& GraphActionListBuilderBase)
{
	check(ToolKitPtr.Pin().Get());

	UJointManager* Manager = ToolKitPtr.Pin().Get()->GetJointManager();

	TArray<TSharedPtr<FEdGraphSchemaAction_K2Graph>> OutGraphActions;
	//Gather all graphs
	if (UEdGraph* Graph = Manager->GetJointGraphAs())
	{
		UJointEdGraph* JointGraph = Cast<UJointEdGraph>(Graph);

		if (JointGraph == nullptr) return;

		FGraphDisplayInfo DisplayInfo;
		JointGraph->GetSchema()->GetGraphDisplayInformation(*JointGraph, DisplayInfo);

		FText FunctionCategory = JointGraph->GetSchema()->GetGraphCategory(JointGraph);

		// Default, so place in 'non' category
		if (FunctionCategory.EqualTo(FText::FromString(Manager->GetName())) || FunctionCategory.EqualTo(UEdGraphSchema_K2::VR_DefaultCategory))
		{
			FunctionCategory = FText::GetEmpty();
		}

		FText ActionCategory = MoveTemp(FunctionCategory);

		TSharedPtr<FEdGraphSchemaAction_K2Graph> NewGraphAction = MakeShareable(new FEdGraphSchemaAction_K2Graph(
			EEdGraphSchemaAction_K2Graph::Graph,
			ActionCategory,
			DisplayInfo.DisplayName,
			DisplayInfo.Tooltip, 2, 0));
		NewGraphAction->FuncName = JointGraph->GetFName();
		NewGraphAction->EdGraph = JointGraph;
		NewGraphAction->GraphType = EEdGraphSchemaAction_K2Graph::Graph;
		OutGraphActions.Add(NewGraphAction);

		GetChildGraphs(JointGraph, NewGraphAction->GetSectionID(), OutGraphActions, ActionCategory);
	}

	for (const TSharedPtr<FEdGraphSchemaAction_K2Graph>& EdGraphSchemaAction_K2Graph : OutGraphActions)
	{
		GraphActionListBuilderBase.AddAction(EdGraphSchemaAction_K2Graph);
	}
}

bool SJointEditorOutliner::SelectionIsCategory() const
{
	return !SelectionHasContextMenu();
}

bool SJointEditorOutliner::CanRequestRenameOnActionNode() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction> > SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);

	// If there is anything selected in the GraphActionMenu, check the item for if it can be renamed.
	if (SelectedActions.Num() || SelectionIsCategory())
	{
		return GraphActionMenu->CanRequestRenameOnActionNode();
	}
	return false;
}

bool SJointEditorOutliner::CanRequestRenameOnActionNode(TWeakPtr<FGraphActionNode> InSelectedNode) const
{

	// TODO 
	return false;

	/*
	bool bIsReadOnly = true;

	// If checking if renaming is available on a category node, the category must have a non-native entry
	if (InSelectedNode.Pin()->IsCategoryNode())
	{
		TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;
		GraphActionMenu->GetCategorySubActions(InSelectedNode, Actions);

		for (TSharedPtr<FEdGraphSchemaAction> Action : Actions)
		{
			if (Action->GetPersistentItemDefiningObject().IsPotentiallyEditable())
			{
				bIsReadOnly = false;
				break;
			}
		}
	}
	else if (InSelectedNode.Pin()->IsActionNode())
	{
		//7>SJointEditorOutliner.cpp(321,27): Warning C4996 : 'FGraphActionNode::Actions': !! WARNING: This array is no longer populated!!
		//FGraphActionNode::Actions array only functioned with a single Action (GetPrimaryAction), access via Action - Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.
		check( InSelectedNode.Pin()->Actions.Num() > 0 && InSelectedNode.Pin()->Actions[0].IsValid() );
		// check type of action
		if (InSelectedNode.Pin()->Actions[0]->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
		{
			bIsReadOnly = false;
		}
	}

	return IsEditingMode() && !bIsReadOnly;
	*/
}


void SJointEditorOutliner::RebuildWidget()
{
	ChildSlot.DetachWidget();

	if (ToolKitPtr.Pin() == nullptr)
	{
		UE_LOG(LogJointEditor, Log, TEXT("Failed to find a valid editor reference. can not create a fragment palette."));

		return;
	}

	ChildSlot
	[
		SAssignNew(GraphActionMenu, SGraphActionMenu, false)
		.OnCreateWidgetForAction(this, &SJointEditorOutliner::OnCreateWidgetForAction)
		.OnCollectAllActions(this, &SJointEditorOutliner::CollectAllActions)
		//.OnCollectStaticSections(this, &SMyBlueprint::CollectStaticSections)
		.OnActionDragged(this, &SJointEditorOutliner::OnActionDragged)
		//.OnCategoryDragged(this, &SMyBlueprint::OnCategoryDragged)
		//.OnActionSelected(this, &SMyBlueprint::OnGlobalActionSelected)
		.OnActionDoubleClicked(this, &SJointEditorOutliner::OnActionDoubleClicked)
		.OnContextMenuOpening(this, &SJointEditorOutliner::OnContextMenuOpening)
		//.OnCategoryTextCommitted(this, &SJointEditorOutliner::OnCategoryNameCommitted)
		.OnCanRenameSelectedAction(this, &SJointEditorOutliner::CanRequestRenameOnActionNode)
		//.OnGetSectionTitle(this, &SMyBlueprint::OnGetSectionTitle)
		//.OnGetSectionWidget(this, &SMyBlueprint::OnGetSectionWidget)
		//.OnActionMatchesName(this, &SMyBlueprint::HandleActionMatchesName)
		.AlphaSortItems(false)
		.UseSectionStyling(true)
	];
}


FReply SJointEditorOutliner::OnActionDragged( const TArray< TSharedPtr<FEdGraphSchemaAction> >& InActions, const FPointerEvent& MouseEvent )
{
	if (!ToolKitPtr.IsValid())
	{
		return FReply::Unhandled();
	}

	TSharedPtr<FEdGraphSchemaAction> InAction( InActions.Num() > 0 ? InActions[0] : nullptr );
	if(InAction.IsValid())
	{
		if(InAction->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Graph* FuncAction = (FEdGraphSchemaAction_K2Graph*)InAction.Get();

			if (FuncAction->EdGraph && FuncAction->EdGraph->GetSchema() && FuncAction->EdGraph->GetSchema()->CanGraphBeDropped(InAction))
			{
				return FuncAction->EdGraph->GetSchema()->BeginGraphDragAction(InAction, FPointerEvent());
			}
		}
	}

	return FReply::Unhandled();
}


void SJointEditorOutliner::OnActionDoubleClicked(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions)
{
	if (!ToolKitPtr.IsValid()) return;

	TSharedPtr<FEdGraphSchemaAction> InAction(InActions.Num() > 0 ? InActions[0] : NULL);
	ExecuteAction(InAction);
}

bool SJointEditorOutliner::SelectionHasContextMenu() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);
	return SelectedActions.Num() > 0;
}

FEdGraphSchemaAction_K2Graph* SJointEditorOutliner::SelectionAsGraph() const
{
	return SelectionAsType<FEdGraphSchemaAction_K2Graph>(GraphActionMenu);
}


TSharedPtr<SWidget> SJointEditorOutliner::OnContextMenuOpening()
{
	if (!ToolKitPtr.IsValid())
	{
		return TSharedPtr<SWidget>();
	}

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, CommandList);

	// Check if the selected action is valid for a context menu
	if (SelectionHasContextMenu())
	{
		FEdGraphSchemaAction_K2Graph* Graph = SelectionAsGraph();
		const bool bExpandFindReferences = Graph || false || false; // this is for the other types of actions that we don't have here yet. (var, func, etc)

		MenuBuilder.BeginSection("BasicOperations");
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename,
				NAME_None,
				LOCTEXT("Rename", "Rename"),
				LOCTEXT("Rename_Tooltip", "Renames the selected item - we turned it off for now because we couldn't get it to work properly, please change the name on the graph directly (by renaming the composite node (collapsed graph)"));

			// Depending on context, FindReferences can be a button or an expandable menu. For example, the context menu
			// for functions now lets you choose whether to do search by-name (fast) or by-function (smart).
			if (!bExpandFindReferences)
			{
				// No expandable menu: display the simple 'Find References' action
				MenuBuilder.AddMenuEntry(FGraphEditorCommands::Get().FindReferences);
			}

			MenuBuilder.AddMenuEntry(FJointEditorCommands::Get().DeleteEntry);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}


void SJointEditorOutliner::ExecuteAction(TSharedPtr<FEdGraphSchemaAction> InAction)
{
	// Force it to open in a new document if shift is pressed
	const bool bIsShiftPressed = FSlateApplication::Get().GetModifierKeys().IsShiftDown();
	FDocumentTracker::EOpenDocumentCause OpenMode = bIsShiftPressed ? FDocumentTracker::ForceOpenNewDocument : FDocumentTracker::OpenNewDocument;

	UJointManager* JointManager = ToolKitPtr.Pin()->GetJointManager();
	if (InAction.IsValid())
	{
		if (InAction->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)InAction.Get();

			if (GraphAction->EdGraph)
			{
				ToolKitPtr.Pin()->JumpToHyperlink(GraphAction->EdGraph);
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE
