//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "EditorWidget/SJointGraphEditorActionMenu.h"

#include "JointEdGraphNode.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "EditorStyleSet.h"
#include "JointAdvancedWidgets.h"
#include "JointEdGraph.h"

#include "SGraphActionMenu.h"
#include "JointEdGraphSchema.h"
#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "JointEdUtils.h"
#include "GraphNode/JointGraphNodeSharedSlates.h"

#include "Misc/EngineVersionComparison.h"
#include "Node/JointFragment.h"

#define LOCTEXT_NAMESPACE "SJointGraphEditorActionMenu"

void SJointGraphEditorActionMenu::Construct(const FArguments& InArgs)
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
	SBorder::Construct(SBorder::FArguments()
	.BorderImage(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Menu.Background"))
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
	if (bUseCustomActionSelected)
	{
		OnActionSelectedCallback.ExecuteIfBound(SelectedActions, InSelectionType);
	}
	else
	{
		// fallback to default behavior (performing the action)
		ActivateAction(SelectedActions, InSelectionType);
	}
}


void SJointGraphEditorActionMenu::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	// If the user has bound a custom action collector, use that instead of the default behavior
	if (OnCollectAllActionsCallback.IsBound())
	{
		OnCollectAllActionsCallback.Execute(OutAllActions);
	}
	else
	{
		CollectActionsFromJointGraphSchema(OutAllActions);
	}
}

TSharedRef<SWidget> SJointGraphEditorActionMenu::OnCreateWidgetForAction(FCreateWidgetForActionData* CreateWidgetForActionData)
{
	if (OnCreateWidgetForActionCallback.IsBound())
	{
		return OnCreateWidgetForActionCallback.Execute(CreateWidgetForActionData);
	}
	else
	{
		if (CreateWidgetForActionData == nullptr || CreateWidgetForActionData->Action == nullptr)
		{
			return SNew(STextBlock).Text(LOCTEXT("InvalidAction", "Invalid Action"));
		}
	
		FName ActionTypeId = CreateWidgetForActionData->Action->GetTypeId();
	
		if (ActionTypeId == FJointSchemaAction_NewSubNode::StaticGetTypeId())
		{
			TSharedPtr<FJointSchemaAction_NewSubNode> SubNodeAction = StaticCastSharedPtr<FJointSchemaAction_NewSubNode>(CreateWidgetForActionData->Action);

			if (SubNodeAction->NodeTemplate != nullptr)
			{
				UClass* NodeClass = SubNodeAction->NodeTemplate->NodeClassData.GetClass();
				UJointFragment* CDO = NodeClass ? Cast<UJointFragment>(NodeClass->GetDefaultObject()) : nullptr;
			
				if (CDO) 
				{
					return SNew(SJointGraphActionWidget_Fragment, CreateWidgetForActionData)
					.Fragment(CDO);
				}
			}
		}else if (ActionTypeId == FJointSchemaAction_NewNodePreset::StaticGetTypeId())
		{
			TSharedPtr<FJointSchemaAction_NewNodePreset> NodePresetAction = StaticCastSharedPtr<FJointSchemaAction_NewNodePreset>(CreateWidgetForActionData->Action);

			if (NodePresetAction->NodePreset != nullptr)
			{
				return SNew(SJointGraphActionWidget_NodePreset, CreateWidgetForActionData)
					.NodePreset(NodePresetAction->NodePreset);
			}
		}
	
		return CreateDefaultGraphActionWidgetForAction(CreateWidgetForActionData);
	}
}

void SJointGraphEditorActionMenu::ActivateAction(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, ESelectInfo::Type InSelectionType)
{
	// Guard against invalid graph object
	if (!GraphObj) return;

	if (InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress || SelectedActions.Num() == 0)
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
		if (!GraphNode) continue;

		ContextMenuBuilder.SelectedObjects.Add(GraphNode);
	}

	// If we were dragging from a pin, add that information to the context
	ContextMenuBuilder.FromPin = DraggedFromPins.Num() > 0 ? DraggedFromPins[0] : nullptr;
	
	
	bool bShouldAddNodePresets = true;
	
	// Determine all possible actions
	if (GraphObj && GraphObj->GetSchema())
	{
		if (UJointEdGraph* JointGraph = Cast<UJointEdGraph>(GraphObj))
		{
			if (ContextMenuBuilder.SelectedObjects.Num() > 0)
			{
				//We're not going to add a node preset option if there are already nodes selected (only fragments)
				bShouldAddNodePresets = false;
			}else if (FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(JointGraph, false, false))
			{
				bShouldAddNodePresets = !Toolkit->IsInNodePresetEditingMode();
			}
		}
		
		if (const UJointEdGraphSchema* MySchema = Cast<const UJointEdGraphSchema>(GraphObj->GetSchema()))
		{
			MySchema->ImplementAddFragmentActions(ContextMenuBuilder);
			if (bShouldAddNodePresets) MySchema->ImplementAddNodePresetActions(ContextMenuBuilder);
		}
	}

	// Copy the added options back to the main list
	GraphActionListBuilderBase.Append(ContextMenuBuilder);
}

TSharedRef<SWidget> SJointGraphEditorActionMenu::CreateDefaultGraphActionWidgetForAction(FCreateWidgetForActionData* CreateWidgetForActionData)
{
	return SNew(SDefaultGraphActionWidget, CreateWidgetForActionData);
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
	OwnerRowPtr = ActionMenuData.TableRow;
#if UE_VERSION_OLDER_THAN(5, 7, 0)
	IndentAmount = InArgs._IndentAmount;
#else
	SetIndentAmount(InArgs._IndentAmount);
#endif
	ActionPtr = ActionMenuData.RowAction;

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

void SJointGraphActionWidget_Fragment::CreateActionWidgetWithNodeSettings(const SJointGraphActionWidget_Fragment::FArguments& InArgs, const FCreateWidgetForActionData* InCreateData, const FJointEdNodeSetting* NodeSetting)
{
	const FSlateBrush* IconBrush = &NodeSetting->IconicNodeImageBrush;

	FLinearColor NormalColor;
	FLinearColor HoverColor;
	FLinearColor OutlineNormalColor;
	FLinearColor OutlineHoverColor;

	FLinearColor Color = NodeSetting->NodeBodyColor;
	const FLinearColor HSV = Color.LinearRGBToHSV();
	const float Value = HSV.B;
	const FLinearColor OffsetColor = FLinearColor::White * (1 - Value) * 0.001;

	NormalColor = Color;
	HoverColor = Color;
	OutlineNormalColor = Color * 1.25 + OffsetColor;
	OutlineHoverColor = Color * 2.5 + OffsetColor * 15;

	const FSlateBrush* InnerBorderImage = nullptr;
	const FSlateBrush* OuterBorderImage = nullptr;
		
	InnerBorderImage = FJointEditorStyle::Get().GetBrush("JointUI.Border.Round");
	OuterBorderImage = FJointEditorStyle::Get().GetBrush("JointUI.Border.Round");

	if (NodeSetting->bUseCustomInnerNodeBodyImageBrush) InnerBorderImage = &NodeSetting->InnerNodeBodyImageBrush;
	if (NodeSetting->bUseCustomOuterNodeBodyImageBrush) OuterBorderImage = &NodeSetting->OuterNodeBodyImageBrush;
	
	TWeakPtr<FJointSchemaAction_NewSubNode> WeakAction = StaticCastSharedPtr<FJointSchemaAction_NewSubNode>(InCreateData->Action);
	
	UClass* NodeClass = WeakAction.Pin()->NodeTemplate->NodeClassData.GetClass();
	
	this->ChildSlot
	[
		SNew(SHorizontalBox)
		.ToolTip(
			SNew(SToolTip)
			[
				SNew(SJointNodeDescription)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.ClassToDescribe(NodeClass)	
			]
		)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Small)
		[
			SNew(SJointOutlineBorder)
			.RenderTransformPivot(FVector2D(0.5))
			.NormalColor(NormalColor)
			.HoverColor(HoverColor)
			.OutlineNormalColor(OutlineNormalColor)
			.OutlineHoverColor(OutlineHoverColor)
			.UnHoverAnimationSpeed(9)
			.HoverAnimationSpeed(9)
			.InnerBorderImage(InnerBorderImage)
			.OuterBorderImage(OuterBorderImage)
			.ContentPadding(FMargin(0))
			[
				GetIcon(IconBrush)
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Small)
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.Text(InCreateData->Action->GetMenuDescription())
			.HighlightText(InArgs._HighlightText)
		]
	];
}


void SJointGraphActionWidget_Fragment::Construct(const FArguments& InArgs, const FCreateWidgetForActionData* InCreateData)
{
	ActionPtr = InCreateData->Action;
	MouseButtonDownDelegate = InCreateData->MouseButtonDownDelegate;
	FragmentPtr = InArgs._Fragment;

	const FJointEdNodeSetting* NodeSetting = &FragmentPtr->EdNodeSetting;
	if (!NodeSetting) return;
	CreateActionWidgetWithNodeSettings(InArgs, InCreateData, NodeSetting);
}

FReply SJointGraphActionWidget_Fragment::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseButtonDownDelegate.Execute(ActionPtr))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedRef<SWidget> SJointGraphActionWidget_Fragment::GetIcon(const FSlateBrush* IconBrush)
{
	if (IconBrush && IconBrush->DrawAs != ESlateBrushDrawType::NoDrawType && !IconBrush->ImageSize.IsNearlyZero())
	{
		const uint16 IconSizeY = 14;
		uint16 IconSizeX = IconBrush->ImageSize.X * IconSizeY / IconBrush->ImageSize.Y;

		return SNew(SBox)
			.WidthOverride(IconSizeX)
			.HeightOverride(IconSizeY)
			[
				SNew(SImage)
				.Image(IconBrush)
			];
	}
	else
	{
		return SNew(SBox)
			.WidthOverride(8)
			.HeightOverride(8);
	}
}


void SJointGraphActionWidget_NodePreset::CreateActionWidget(const FArguments& InArgs, const FCreateWidgetForActionData* InCreateData)
{
	UJointNodePreset* NodePreset = NodePresetPtr.Get(); 
	
	if (!NodePreset) return;
	
	const FSlateBrush* IconBrush = NodePreset->bUseCustomIcon ? &NodePreset->PresetIconBrush : FJointEditorStyle::Get().GetBrush("ClassIcon.JointNodePreset");
	const FLinearColor Color = NodePreset->PresetColor;
	
	const FSlateBrush* BorderImage = FJointEditorStyle::Get().GetBrush("JointUI.Border.Round");
	
	FLinearColor NormalColor;
	FLinearColor HoverColor;
	FLinearColor OutlineNormalColor;
	FLinearColor OutlineHoverColor;

	const FLinearColor HSV = Color.LinearRGBToHSV();
	const float Value = HSV.B;
	const FLinearColor OffsetColor = FLinearColor::White * (1 - Value) * 0.001;

	NormalColor = Color;
	HoverColor = Color;
	OutlineNormalColor = Color * 1.25 + OffsetColor;
	OutlineHoverColor = Color * 2.5 + OffsetColor * 15;
	
	this->ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(SJointOutlineBorder)
		.RenderTransformPivot(FVector2D(0.5))
		.NormalColor(NormalColor)
		.HoverColor(HoverColor)
		.OutlineNormalColor(OutlineNormalColor)
		.OutlineHoverColor(OutlineHoverColor)
		.UnHoverAnimationSpeed(9)
		.HoverAnimationSpeed(9)
		.InnerBorderImage(BorderImage)
		.OuterBorderImage(BorderImage)
		.ContentPadding(FMargin(0))
		[
			SNew(SHorizontalBox)
			.ToolTipText(InCreateData->Action->GetTooltipDescription())
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Small)
			[
				GetIcon(IconBrush)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Small)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.Text(InCreateData->Action->GetMenuDescription())
				.HighlightText(InArgs._HighlightText)
			]
		]
	];	
}

void SJointGraphActionWidget_NodePreset::Construct(const FArguments& InArgs, const FCreateWidgetForActionData* InCreateData)
{
	ActionPtr = InCreateData->Action;
	MouseButtonDownDelegate = InCreateData->MouseButtonDownDelegate;
	NodePresetPtr = InArgs._NodePreset;
	
	CreateActionWidget(InArgs, InCreateData);
}

FReply SJointGraphActionWidget_NodePreset::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseButtonDownDelegate.Execute(ActionPtr))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedRef<SWidget> SJointGraphActionWidget_NodePreset::GetIcon(const FSlateBrush* IconBrush)
{
	if (IconBrush && IconBrush->DrawAs != ESlateBrushDrawType::NoDrawType && !IconBrush->ImageSize.IsNearlyZero())
	{
		const uint16 IconSizeY = 14;
		uint16 IconSizeX = IconBrush->ImageSize.X * IconSizeY / IconBrush->ImageSize.Y;

		return SNew(SBox)
			.WidthOverride(IconSizeX)
			.HeightOverride(IconSizeY)
			[
				SNew(SImage)
				.Image(IconBrush)
			];
	}
	else
	{
		return SNew(SBox)
			.WidthOverride(8)
			.HeightOverride(8);
	}
}



#undef LOCTEXT_NAMESPACE