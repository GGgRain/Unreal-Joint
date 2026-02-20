//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "EditorWidget/SJointGraphEditorActionMenu.h"

#include "JointEdGraphNode.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "EditorStyleSet.h"
#include "JointAdvancedWidgets.h"

#include "SGraphActionMenu.h"
#include "JointEdGraphSchema.h"
#include "JointEditorStyle.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"

#include "Misc/EngineVersionComparison.h"

void SJointGraphEditorActionMenu::Construct( const FArguments& InArgs )
{
	this->GraphObj = InArgs._GraphObj;
	this->GraphNodes = InArgs._GraphNodes;
	this->DraggedFromPins = InArgs._DraggedFromPins;
	this->NewNodePosition = InArgs._NewNodePosition;
	this->AutoExpandActionMenu = InArgs._AutoExpandActionMenu;
	this->bUseCustomActionSelected = InArgs._bUseCustomActionSelected;
	
	this->OnCollectAllActionsCallback = InArgs._OnCollectAllActions;
	this->OnActionSelectedCallback = InArgs._OnActionSelected;
	this->OnCreateWidgetForActionCallback = InArgs._OnCreateWidgetForAction;
	this->OnClosedCallback = InArgs._OnClosedCallback;

	SetCanTick(false);
	
	// Build the widget layout
	SBorder::Construct( SBorder::FArguments()
	                    .BorderImage( FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Menu.Background") )
	                    .Padding(FJointEditorStyle::Margin_Normal)
		[
			// Achieving fixed width by nesting items within a fixed width box.
			SNew(SBox)
			.WidthOverride(400)
			[
				SAssignNew(GraphActionMenuSlate, SGraphActionMenu)
				.OnActionSelected(this, &SJointGraphEditorActionMenu::OnActionSelected)
				.OnCollectAllActions(this, &SJointGraphEditorActionMenu::CollectAllActions)
				.OnCreateCustomRowExpander_Static(&SJointActionMenuExpander::CreateExpander)
				.OnCreateWidgetForAction(this, &SJointGraphEditorActionMenu::OnCreateWidgetForAction)
				.AutoExpandActionMenu(AutoExpandActionMenu)
			]
		]
	);
}

SJointGraphEditorActionMenu::~SJointGraphEditorActionMenu()
{
	OnClosedCallback.ExecuteIfBound();
}

void SJointGraphEditorActionMenu::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, ESelectInfo::Type InSelectionType)
{
	// If the user has bound a custom action selected handler, use that instead of the default behavior
	if(bUseCustomActionSelected)
	{
		OnActionSelectedCallback.ExecuteIfBound(SelectedActions,InSelectionType);
	}else
	{
		// fallback to default behavior (performing the action)
		ActivateAction(SelectedActions, InSelectionType);	
	}
}


void SJointGraphEditorActionMenu::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	// If the user has bound a custom action collector, use that instead of the default behavior
	if(OnCollectAllActionsCallback.IsBound())
	{
		OnCollectAllActionsCallback.Execute(OutAllActions);
	}else
	{
		CollectActionsFromJointGraphSchema(OutAllActions);
	}
}

TSharedRef<SWidget> SJointGraphEditorActionMenu::CreateDefaultGraphActionWidgetForAction(FCreateWidgetForActionData* CreateWidgetForActionData)
{
	return SNew(SDefaultGraphActionWidget, CreateWidgetForActionData);
}

TSharedRef<SWidget> SJointGraphEditorActionMenu::OnCreateWidgetForAction(FCreateWidgetForActionData* CreateWidgetForActionData)
{
	if(OnCreateWidgetForActionCallback.IsBound())
	{
		return OnCreateWidgetForActionCallback.Execute(CreateWidgetForActionData);
	}else
	{
		return CreateDefaultGraphActionWidgetForAction(CreateWidgetForActionData);
	}
}

void SJointGraphEditorActionMenu::ActivateAction(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, ESelectInfo::Type InSelectionType)
{
	// Guard against invalid graph object
	if (!GraphObj) return;
	
	if (InSelectionType == ESelectInfo::OnMouseClick  || InSelectionType == ESelectInfo::OnKeyPress || SelectedActions.Num() == 0)
	{
		bool bDoDismissMenus = false;
		
		for (const TSharedPtr<FEdGraphSchemaAction>& Action : SelectedActions)
		{
			if (Action.IsValid())
			{
				// Perform the action.
				Action->PerformAction(GraphObj, DraggedFromPins, NewNodePosition);
						
				bDoDismissMenus = true;
			}
		}
		
		if (bDoDismissMenus) FSlateApplication::Get().DismissAllMenus();
	}
}

void SJointGraphEditorActionMenu::CollectActionsFromJointGraphSchema(FGraphActionListBuilderBase& GraphActionListBuilderBase)
{
	// Build up the context object
	FGraphContextMenuBuilder ContextMenuBuilder(GraphObj);
	for (UJointEdGraphNode* GraphNode : GraphNodes)
	{
		if(!(GraphNode != nullptr && GraphNode->IsValidLowLevel())) continue;
		
		ContextMenuBuilder.SelectedObjects.Add((UObject*)GraphNode);
	}
	
	// If we were dragging from a pin, add that information to the context
	ContextMenuBuilder.FromPin = DraggedFromPins.Num() > 0 ? DraggedFromPins[0] : nullptr;
	
	// Determine all possible actions
	if(GraphObj && GraphObj->GetSchema())
	{
		if (const UJointEdGraphSchema* MySchema = Cast<const UJointEdGraphSchema>(GraphObj->GetSchema()))
		{
			MySchema->ImplementAddFragmentActions(ContextMenuBuilder);
			MySchema->ImplementAddNodePresetActions(ContextMenuBuilder);
		}
	}

	// Copy the added options back to the main list
	GraphActionListBuilderBase.Append(ContextMenuBuilder);
}

TSharedRef<SEditableTextBox> SJointGraphEditorActionMenu::GetFilterTextBox() const
{
	return GraphActionMenuSlate.Pin()->GetFilterTextBox();
}

FText SJointGraphEditorActionMenu::GetFilterText() const
{
	return GetFilterTextBox()->GetText();
}


void SJointActionMenuExpander::Construct(const FArguments& InArgs, const FCustomExpanderData& ActionMenuData)
{
	OwnerRowPtr  = ActionMenuData.TableRow;
#if UE_VERSION_OLDER_THAN(5, 7, 0)
	IndentAmount = InArgs._IndentAmount;
#else
	SetIndentAmount(InArgs._IndentAmount);
#endif
	ActionPtr    = ActionMenuData.RowAction;

	if (!ActionPtr.IsValid())
	{
		SExpanderArrow::FArguments SuperArgs;
		SuperArgs._IndentAmount = InArgs._IndentAmount;

		SExpanderArrow::Construct(SuperArgs, ActionMenuData.TableRow);
	}
	else
	{			
		ChildSlot.Padding(TAttribute<FMargin>(this, &SJointActionMenuExpander::GetCustomIndentPadding));
	}
}

FMargin SJointActionMenuExpander::GetCustomIndentPadding() const
{
	FMargin CustomPadding = SExpanderArrow::GetExpanderPadding() * 1.5;
	return CustomPadding;
}

TSharedRef<SExpanderArrow> SJointActionMenuExpander::CreateExpander(const FCustomExpanderData& ActionMenuData)
{
	return SNew(SJointActionMenuExpander, ActionMenuData);
}

void SJointGraphActionWidget::Construct(const FArguments& InArgs, const FCreateWidgetForActionData* InCreateData)
{
	ActionPtr = InCreateData->Action;
	MouseButtonDownDelegate = InCreateData->MouseButtonDownDelegate;
	
	
	const uint16 IconSizeY = 16;
	uint16 IconSizeX = 16;
	if (InArgs._IconBrush)
	{
		IconSizeX = InArgs._IconBrush->ImageSize.X * IconSizeY / InArgs._IconBrush->ImageSize.Y;
	}
	
	this->ChildSlot
	[
		SNew(SHorizontalBox)
		.ToolTipText(InCreateData->Action->GetTooltipDescription())
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Tiny)
		[
			SNew(SBox)
				.WidthOverride(IconSizeX)
				.HeightOverride(IconSizeY)
				[
					SNew(SImage)
					.Image(InArgs._IconBrush)
					.ColorAndOpacity(InArgs._IconColor)
				]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Tiny)
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.Text(InCreateData->Action->GetMenuDescription())
			.HighlightText(InArgs._HighlightText)
		]
	];
}

FReply SJointGraphActionWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if( MouseButtonDownDelegate.Execute( ActionPtr ) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

