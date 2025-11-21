//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "EditorWidget/SJointGraphEditorActionMenu.h"

#include "JointEdGraphNode.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "EditorStyleSet.h"

#include "SGraphActionMenu.h"
#include "JointEdGraphSchema.h"
#include "JointEditorStyle.h"

#include "Misc/EngineVersionComparison.h"

SJointGraphEditorActionMenu::~SJointGraphEditorActionMenu()
{
	OnClosedCallback.ExecuteIfBound();
}

void SJointGraphEditorActionMenu::Construct( const FArguments& InArgs )
{
	this->GraphObj = InArgs._GraphObj;
	this->GraphNodes = InArgs._GraphNodes;
	this->DraggedFromPins = InArgs._DraggedFromPins;
	this->NewNodePosition = InArgs._NewNodePosition;
	this->OnClosedCallback = InArgs._OnClosedCallback;
	this->AutoExpandActionMenu = InArgs._AutoExpandActionMenu;
	this->bUseCustomActionSelected = InArgs._bUseCustomActionSelected;
	this->OnActionSelectedCallback = InArgs._OnActionSelected;

	SetCanTick(false);
	
	// Build the widget layout
	SBorder::Construct( SBorder::FArguments()
		.BorderImage( FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Menu.Background") )
		.Padding(5)
		[
			// Achieving fixed width by nesting items within a fixed width box.
			SNew(SBox)
			.WidthOverride(400)
			[
				SAssignNew(GraphActionMenu, SGraphActionMenu)
				.OnActionSelected(this, &SJointGraphEditorActionMenu::OnActionSelected)
				.OnCollectAllActions(this, &SJointGraphEditorActionMenu::CollectAllActions)
				.OnCreateCustomRowExpander_Static(&SJointActionMenuExpander::CreateExpander)
				.AutoExpandActionMenu(AutoExpandActionMenu)
			]
		]
	);
}

void SJointGraphEditorActionMenu::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedAction, ESelectInfo::Type InSelectionType)
{

	if(bUseCustomActionSelected)
	{
		OnActionSelectedCallback.ExecuteIfBound(SelectedAction,InSelectionType);
	}
	else
	{
		if (InSelectionType == ESelectInfo::OnMouseClick  || InSelectionType == ESelectInfo::OnKeyPress || SelectedAction.Num() == 0)
		{
			bool bDoDismissMenus = false;

			if (GraphObj)
			{
				for ( int32 ActionIndex = 0; ActionIndex < SelectedAction.Num(); ActionIndex++ )
				{
					TSharedPtr<FEdGraphSchemaAction> CurrentAction = SelectedAction[ActionIndex];

					if ( CurrentAction.IsValid() )
					{
						CurrentAction->PerformAction(GraphObj, DraggedFromPins, NewNodePosition);
						
						bDoDismissMenus = true;
					}
				}
			}

			if (bDoDismissMenus)
			{
				FSlateApplication::Get().DismissAllMenus();
			}
		}
	}
}


void SJointGraphEditorActionMenu::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	// Build up the context object
	FGraphContextMenuBuilder ContextMenuBuilder(GraphObj);

	for (UJointEdGraphNode* GraphNode : GraphNodes)
	{
		if(!(GraphNode != nullptr && GraphNode->IsValidLowLevel())) continue;
		
		ContextMenuBuilder.SelectedObjects.Add((UObject*)GraphNode);

	}
	if (DraggedFromPins.Num() > 0)
	{
		ContextMenuBuilder.FromPin = DraggedFromPins[0];
	}

	// Determine all possible actions

	if(GraphObj && GraphObj->GetSchema())
	{
		const UJointEdGraphSchema* MySchema = Cast<const UJointEdGraphSchema>(GraphObj->GetSchema());
		
		if (MySchema) MySchema->GetGraphNodeContextActions(ContextMenuBuilder);
	}

	// Copy the added options back to the main list
	//@TODO: Avoid this copy
	OutAllActions.Append(ContextMenuBuilder);
}

TSharedRef<SEditableTextBox> SJointGraphEditorActionMenu::GetFilterTextBox()
{
	return GraphActionMenu->GetFilterTextBox();
}

TSharedRef<SExpanderArrow> SJointActionMenuExpander::CreateExpander(const FCustomExpanderData& ActionMenuData)
{
	return SNew(SJointActionMenuExpander, ActionMenuData);
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
