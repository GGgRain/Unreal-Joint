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

/**
 * A wrapper around SGraphActionMenu that is used to display context menus for the Joint graph editor. 
 */
class JOINTEDITOR_API SJointGraphEditorActionMenu : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SJointGraphEditorActionMenu)
		: _GraphObj( nullptr )
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
	SLATE_ARGUMENT( bool, bUseCustomActionSelected )
	SLATE_ARGUMENT( TArray<UEdGraphPin*>, DraggedFromPins )
	SLATE_ARGUMENT( SGraphEditor::FActionMenuClosed, OnClosedCallback )
	SLATE_ARGUMENT( bool, AutoExpandActionMenu )
		
	// Callbacks for extending functionality.
	SLATE_EVENT( SGraphActionMenu::FOnActionSelected, OnActionSelected )
	SLATE_EVENT( SGraphActionMenu::FOnCollectAllActions, OnCollectAllActions )
	SLATE_EVENT( SGraphActionMenu::FOnCreateWidgetForAction, OnCreateWidgetForAction )
		
	SLATE_END_ARGS()
	
public:
	
	void Construct( const FArguments& InArgs );
	
public:

	virtual ~SJointGraphEditorActionMenu() override;
	
public:
	
	//extension callbacks and events.
	
	void OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, ESelectInfo::Type InSelectionType);
	void CollectAllActions(FGraphActionListBuilderBase& OutAllActions);
	TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* CreateWidgetForActionData);

public:
	
	/**
	 * Activates the provided action(s) by performing them on the graph object that is currently being edited.
	 * @param SelectedActions The actions to activate (perform).
	 * @param InSelectionType The type of selection that caused the activation (e.g. mouse click, key press, etc.).
	 */
	void ActivateAction(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, ESelectInfo::Type InSelectionType);

	/**
	 * Collects all available actions for the current graph context by building up a context menu builder and passing it to the user-provided callback for populating.
	 * @param GraphActionListBuilderBase A list builder to populate with all available actions for the current graph context.
	 */
	void CollectActionsFromJointGraphSchema(FGraphActionListBuilderBase& GraphActionListBuilderBase);
	
	/**
	 * Creates a default widget for the provided action data. This is used as a fallback if the user of this menu does not provide their own widget creation callback.
	 * @param CreateWidgetForActionData A struct containing information about the action to create a widget for, as well as delegates for handling mouse events and rename requests on the widget.
	 * @return A slate widget to represent the provided action in the menu.
	 */
	TSharedRef<SWidget> CreateDefaultGraphActionWidgetForAction(FCreateWidgetForActionData* CreateWidgetForActionData);

public:
	
	SGraphActionMenu::FOnActionSelected OnActionSelectedCallback;
	SGraphActionMenu::FOnCollectAllActions OnCollectAllActionsCallback;
	SGraphActionMenu::FOnCreateWidgetForAction OnCreateWidgetForActionCallback;
	
	SGraphEditor::FActionMenuClosed OnClosedCallback;
	
public:
	
	/**
	 * Slate for the graph action menu.
	 */
	TWeakPtr<SGraphActionMenu> GraphActionMenuSlate;
	
public:

	TSharedRef<SEditableTextBox> GetFilterTextBox() const;
	FText GetFilterText() const;

public:

	// external accesible data for menu context -> use them for the default behavior of the menu, or use them in your custom callbacks to provide more customized functionality!
	UEdGraph* GraphObj = nullptr;
	TArray<UJointEdGraphNode*> GraphNodes;
	TArray<UEdGraphPin*> DraggedFromPins;

protected:
#if UE_VERSION_OLDER_THAN(5,6,0)
	FVector2D NewNodePosition;
#else
	FVector2f NewNodePosition;
#endif
	
	bool bUseCustomActionSelected = false;
	bool AutoExpandActionMenu = false;
	
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

class JOINTEDITOR_API SJointGraphActionWidget_Fragment : public SCompoundWidget
{
	SLATE_BEGIN_ARGS( SJointGraphActionWidget_Fragment ) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UJointFragment>, Fragment)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()
	
public:
	
	void CreateActionWidgetWithNodeSettings(const FArguments& InArgs, const FCreateWidgetForActionData* InCreateData, const FJointEdNodeSetting* NodeSetting);
	void Construct(const FArguments& InArgs, const FCreateWidgetForActionData* InCreateData);
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	TSharedRef<SWidget> GetIcon(const FSlateBrush* IconBrush);

private:
	
	/** The item that we want to display with this widget */
	TWeakPtr<FEdGraphSchemaAction> ActionPtr;
	/** Delegate executed when mouse button goes down */
	FCreateWidgetMouseButtonDown MouseButtonDownDelegate;
	
private:
	
	TWeakObjectPtr<UJointFragment> FragmentPtr;
};

class JOINTEDITOR_API SJointGraphActionWidget_NodePreset : public SCompoundWidget
{
	SLATE_BEGIN_ARGS( SJointGraphActionWidget_NodePreset ) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UJointNodePreset>, NodePreset)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()
	
public:
	
	void Construct(const FArguments& InArgs, const FCreateWidgetForActionData* InCreateData);
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	
public:
	
	void CreateActionWidget(const FArguments& InArgs, const FCreateWidgetForActionData* InCreateData);
	TSharedRef<SWidget> GetIcon(const FSlateBrush* IconBrush);
	
private:
	
	/** The item that we want to display with this widget */
	TWeakPtr<FEdGraphSchemaAction> ActionPtr;
	/** Delegate executed when mouse button goes down */
	FCreateWidgetMouseButtonDown MouseButtonDownDelegate;
	
private:
	
	TWeakObjectPtr<UJointNodePreset> NodePresetPtr;
};
