//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorWidget/SJointFragmentPalette.h"

#include "JointAdvancedWidgets.h"
#include "JointEdGraphSchema.h"
#include "JointEditorLogChannels.h"
#include "JointEditorToolkit.h"
#include "EditorWidget/SJointGraphEditorActionMenu.h"

#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "EditorTools/SJointNotificationWidget.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Markdown/SJointMDSlate_Admonitions.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Misc/EngineVersionComparison.h"
#include "Node/JointFragment.h"

#define LOCTEXT_NAMESPACE "SJointFragmentPalette"

void SJointFragmentPalette::Construct(const FArguments& InArgs)
{
	ToolKitPtr = InArgs._ToolKitPtr;

	SetCanTick(false);

	RebuildWidget();
}

void SJointFragmentPalette::OnFragmentActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedAction,
                                                        ESelectInfo::Type InSelectionType)
{
	if (!(InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress) || SelectedAction.Num() == 0) return;

	if (!ToolKitPtr.IsValid() || !ToolKitPtr.Pin()->GetFocusedJointGraph()) return;

	const FGraphPanelSelectionSet Selections = ToolKitPtr.Pin()->GetSelectedNodes();

	if (Selections.Num() == 0)
	{
		FJointEdUtils::FireNotification(
			LOCTEXT("Notification_Title_CannotAddFragment", "Cannot Add Fragment"),
			LOCTEXT("Notification_Sub_CannotAddFragment", "Nodes must be selected in the graph to attach the fragment to. Please select one or more nodes and try again."),
			EJointMDAdmonitionType::Error
		);
		
		return;
	}

	for (int32 ActionIndex = 0; ActionIndex < SelectedAction.Num(); ActionIndex++)
	{
		TSharedPtr<FEdGraphSchemaAction> CurrentAction = SelectedAction[ActionIndex];

		if (!CurrentAction.IsValid()) continue;

		StaticCastSharedPtr<FJointSchemaAction_NewSubNode>(CurrentAction)->NodesToAttachTo = Selections.Array();

#if UE_VERSION_OLDER_THAN(5, 6, 0)
		CurrentAction->PerformAction(ToolKitPtr.Pin()->GetFocusedJointGraph(), nullptr, FVector2D::ZeroVector);
#else
		CurrentAction->PerformAction(ToolKitPtr.Pin()->GetFocusedJointGraph(), nullptr, FVector2f::ZeroVector);
#endif
	}
}

void SJointFragmentPalette::CollectFragmentActions(FGraphActionListBuilderBase& GraphActionListBuilderBase)
{
	if (!ToolKitPtr.IsValid() || !ToolKitPtr.Pin()->GetFocusedJointGraph()) return;
	
	//We want to provide only the fragments when we are in the node preset editing mode.
	//(We don't support nested presets and nested links)
	if (ToolKitPtr.Pin()->IsInNodePresetEditingMode())
	{
		FGraphContextMenuBuilder ContextMenuBuilder(ActionMenu->GraphObj);
		for (UJointEdGraphNode* GraphNode : ActionMenu->GraphNodes)
		{
			if(!(GraphNode != nullptr && GraphNode->IsValidLowLevel())) continue;
		
			ContextMenuBuilder.SelectedObjects.Add((UObject*)GraphNode);
		}
	
		// If we were dragging from a pin, add that information to the context
		ContextMenuBuilder.FromPin = ActionMenu->DraggedFromPins.Num() > 0 ? ActionMenu->DraggedFromPins[0] : nullptr;
	
		// Determine all possible actions
		if(ActionMenu->GraphObj && ActionMenu->GraphObj->GetSchema())
		{
			if (const UJointEdGraphSchema* MySchema = Cast<const UJointEdGraphSchema>(ActionMenu->GraphObj->GetSchema()))
			{
				MySchema->ImplementAddFragmentActions(ContextMenuBuilder);
			}
		}

		// Copy the added options back to the main list
		GraphActionListBuilderBase.Append(ContextMenuBuilder);
	}else
	{
		ActionMenu->CollectActionsFromJointGraphSchema(GraphActionListBuilderBase);
	}
}

TSharedRef<SWidget> SJointFragmentPalette::OnCreateWidgetForAction(FCreateWidgetForActionData* CreateWidgetForActionData)
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
				FJointEdNodeSetting& EdNodeSetting = CDO->EdNodeSetting;
				const FLinearColor& NodeIconicColor = EdNodeSetting.NodeIconicColor;
				const FSlateBrush* IconicNodeImageBrush = EdNodeSetting.IconicNodeImageBrush.DrawAs != ESlateBrushDrawType::NoDrawType ? &EdNodeSetting.IconicNodeImageBrush : FJointEditorStyle::Get().GetBrush("ClassIcon.JointFragment");
				
				return SNew(SJointGraphActionWidget, CreateWidgetForActionData)
					.IconBrush(IconicNodeImageBrush)
					.IconColor(NodeIconicColor);
			}
			
			FLinearColor DefaultNodeIconicColor = FLinearColor::White;
			const FSlateBrush* DefaultIconicNodeImageBrush = FJointEditorStyle::Get().GetBrush("ClassIcon.JointFragment");
			
			return SNew(SJointGraphActionWidget, CreateWidgetForActionData)
				.IconBrush(DefaultIconicNodeImageBrush)
				.IconColor(DefaultNodeIconicColor);
		}
	}
	
	
	return ActionMenu->CreateDefaultGraphActionWidgetForAction(CreateWidgetForActionData);
}

void SJointFragmentPalette::RebuildWidget()
{
	ChildSlot.DetachWidget();

	if (ToolKitPtr.Pin() == nullptr)
	{
		UE_LOG(LogJointEditor, Log, TEXT("Failed to find a valid editor toolkit for the fragment palette."));

		return;
	}

	ChildSlot
	[
		SAssignNew(ActionMenu, SJointGraphEditorActionMenu)
		.GraphObj(ToolKitPtr.Pin()->GetFocusedJointGraph())
		.bUseCustomActionSelected(true)
		.AutoExpandActionMenu(true)
		.OnActionSelected(this, &SJointFragmentPalette::OnFragmentActionSelected)
		.OnCollectAllActions(this, &SJointFragmentPalette::CollectFragmentActions)
		.OnCreateWidgetForAction(this, &SJointFragmentPalette::OnCreateWidgetForAction)
	];
}



#undef LOCTEXT_NAMESPACE