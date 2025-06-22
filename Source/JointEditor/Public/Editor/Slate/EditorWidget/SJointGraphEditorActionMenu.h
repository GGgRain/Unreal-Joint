//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphSchemaActions.h"
#include "GraphEditor.h"
#include "SGraphActionMenu.h"
#include "Widgets/Layout/SBorder.h"

#include "Misc/EngineVersionComparison.h"


class SEditableTextBox;
class SGraphActionMenu;
class UJointEdGraphNode;
class UJointNodeBase;

/////////////////////////////////////////////////////////////////////////////////////////////////

class JOINTEDITOR_API SJointGraphEditorActionMenu : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SJointGraphEditorActionMenu)
		: _GraphObj( static_cast<UEdGraph*>(NULL) )
		, _NewNodePosition( FVector2D::ZeroVector )
		, _bUseCustomActionSelected( false )
		, _AutoExpandActionMenu( false )
		{}

	SLATE_ARGUMENT( UEdGraph*, GraphObj )
	SLATE_ARGUMENT(TArray<UJointEdGraphNode*>, GraphNodes)

#if UE_VERSION_OLDER_THAN(5,6,0)
	SLATE_ARGUMENT( FVector2D, NewNodePosition )
#else
	SLATE_ARGUMENT( FVector2f, NewNodePosition )
#endif
		
		SLATE_EVENT( SGraphActionMenu::FOnActionSelected, OnActionSelected )
		SLATE_ARGUMENT( bool, bUseCustomActionSelected )

		SLATE_ARGUMENT( TArray<UEdGraphPin*>, DraggedFromPins )
		SLATE_ARGUMENT( SGraphEditor::FActionMenuClosed, OnClosedCallback )
		
		SLATE_ARGUMENT( bool, AutoExpandActionMenu )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	~SJointGraphEditorActionMenu();

	TSharedRef<SEditableTextBox> GetFilterTextBox();

protected:

	UEdGraph* GraphObj;
	TArray<UJointEdGraphNode*> GraphNodes;
	TArray<UEdGraphPin*> DraggedFromPins;

#if UE_VERSION_OLDER_THAN(5,6,0)
	FVector2D NewNodePosition;
#else
	FVector2f NewNodePosition;
#endif
	
	bool bUseCustomActionSelected;
	bool AutoExpandActionMenu;

	SGraphActionMenu::FOnActionSelected OnActionSelectedCallback;
	SGraphEditor::FActionMenuClosed OnClosedCallback;
	TSharedPtr<SGraphActionMenu> GraphActionMenu;

public:

	void OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedAction, ESelectInfo::Type InSelectionType);
	
	/** Callback used to populate all actions list in SGraphActionMenu */
	void CollectAllActions(FGraphActionListBuilderBase& OutAllActions);
};


class JOINTEDITOR_API SJointActionMenuExpander : public SExpanderArrow
{
	SLATE_BEGIN_ARGS( SJointActionMenuExpander ) {}
	SLATE_ATTRIBUTE(float, IndentAmount)
SLATE_END_ARGS()

public:
	/**
	 * Constructs a standard SExpanderArrow widget if the associated menu item 
	 * is a category or separator, otherwise, for action items, it constructs
	 * a favoriting toggle (plus indent) in front of the action entry.
	 * 
	 * @param  InArgs			A set of slate arguments for this widget type (defined above).
	 * @param  ActionMenuData	A set of useful data for detailing the specific action menu row this is for.
	 */
	void Construct(const FArguments& InArgs, const FCustomExpanderData& ActionMenuData);

private:
	/**
	 * Action menu expanders are also responsible for properly indenting the 
	 * menu entries, so this returns the proper margin padding for the menu row
	 * (based off its indent level).
	 * 
	 * @return Padding to construct around this widget (so the menu entry is properly indented).
	 */
	FMargin GetCustomIndentPadding() const;

	/** The action associated with the menu row this belongs to */
	TWeakPtr<FEdGraphSchemaAction> ActionPtr;

public:

	static TSharedRef<SExpanderArrow> CreateExpander(const FCustomExpanderData& ActionMenuData);

};
