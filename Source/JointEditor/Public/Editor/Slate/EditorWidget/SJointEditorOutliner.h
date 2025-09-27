//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphSchemaActions.h"
#include "SGraphActionMenu.h"
#include "Editor/Kismet/Private/BlueprintDetailsCustomization.h"
#include "Widgets/SCompoundWidget.h"

class SGraphActionMenu;
class SScrollBox;
class UJointEdGraphNode;
class FJointEditorToolkit;

//////////////////////////////////////////////////////////////////////////
// SJointEditorOutliner

class JOINTEDITOR_API SJointEditorOutliner : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SJointEditorOutliner){}
		SLATE_ARGUMENT(TWeakPtr<FJointEditorToolkit>, ToolKitPtr)
	SLATE_END_ARGS()
public:
	
	void Construct(const FArguments& InArgs);
	bool IsEditingMode() const;

protected:
	void RebuildWidget();

public:

	void OnDeleteEntry();
	void OnDeleteGraph(UEdGraph* InGraph, EEdGraphSchemaAction_K2Graph::Type InGraphType);
	bool CanDeleteEntry() const;

public:

	bool SelectionHasContextMenu() const;
	FEdGraphSchemaAction_K2Graph* SelectionAsGraph() const;

public:

	void CollectAllActions(FGraphActionListBuilderBase& GraphActionListBuilderBase);
	TSharedPtr<SWidget> OnContextMenuOpening();
	void OnActionDoubleClicked(const TArray< TSharedPtr<FEdGraphSchemaAction> >& InActions);
	FReply OnActionDragged( const TArray< TSharedPtr<FEdGraphSchemaAction> >& InActions, const FPointerEvent& MouseEvent );

	bool CanRequestRenameOnActionNode() const;
	bool CanRequestRenameOnActionNode(TWeakPtr<FGraphActionNode> InSelectedNode) const;
	void OnRequestRenameOnActionNode();
	
	bool SelectionIsCategory() const;
	
public:

	void Refresh();
	
public:

	TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData);
	
	void GetChildGraphs(UEdGraph* InEdGraph, int32 const SectionId, TArray<TSharedPtr<FEdGraphSchemaAction_K2Graph>>& OutGraphActions, const FText& ParentCategory) const;
	
	void ExecuteAction(TSharedPtr<FEdGraphSchemaAction> InAction);

private:

	TWeakPtr<FJointEditorToolkit> ToolKitPtr;

	/** List of UI Commands for this scope */
	TSharedPtr<FUICommandList> CommandList;

public:

	TArray< TSharedPtr<struct FJointFragmentPaletteAction> > ActionEntries;

public:

	TSharedPtr<SGraphActionMenu> GraphActionMenu;

};
