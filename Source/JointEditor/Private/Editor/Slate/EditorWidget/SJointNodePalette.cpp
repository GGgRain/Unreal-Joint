//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorWidget/SJointNodePalette.h"

#include "JointAdvancedWidgets.h"
#include "JointEdGraphSchema.h"
#include "JointEditorLogChannels.h"
#include "JointEditorToolkit.h"
#include "EditorWidget/SJointGraphEditorActionMenu.h"

#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "EditorTools/SJointNotificationWidget.h"
#include "EditorWidget/JointGraphEditor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Markdown/SJointMDSlate_Admonitions.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Misc/EngineVersionComparison.h"
#include "Node/JointFragment.h"

#define LOCTEXT_NAMESPACE "SJointFragmentPalette"

void SJointNodePalette::Construct(const FArguments& InArgs)
{
	ToolKitPtr = InArgs._ToolKitPtr;

	SetCanTick(false);

	RebuildWidget();
}
PRAGMA_DISABLE_OPTIMIZATION
void SJointNodePalette::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedAction,
                                                        ESelectInfo::Type InSelectionType)
{
	if (!(InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress) || SelectedAction.Num() == 0) return;

	if (!ToolKitPtr.IsValid() || !ToolKitPtr.Pin()->GetFocusedJointGraph()) return;
	
	for (const TSharedPtr<FEdGraphSchemaAction>& EdGraphSchemaAction : SelectedAction)
	{
		if ( !EdGraphSchemaAction) continue;
		
		if (EdGraphSchemaAction->GetTypeId() == FJointSchemaAction_NewSubNode::StaticGetTypeId())
		{
			TSharedPtr<FJointSchemaAction_NewSubNode> SubNodeAction = StaticCastSharedPtr<FJointSchemaAction_NewSubNode>(EdGraphSchemaAction);

			if (SubNodeAction->NodeTemplate == nullptr) continue;
			
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
			
			SubNodeAction->NodesToAttachTo = Selections.Array();

#if UE_VERSION_OLDER_THAN(5, 6, 0)
			SubNodeAction->PerformAction(ToolKitPtr.Pin()->GetFocusedJointGraph(), nullptr, FVector2D::ZeroVector);
#else
			SubNodeAction->PerformAction(ToolKitPtr.Pin()->GetFocusedJointGraph(), nullptr, FVector2f::ZeroVector);
#endif
		}
		else if (EdGraphSchemaAction->GetTypeId() == FJointSchemaAction_NewNodePreset::StaticGetTypeId())
		{
			TSharedPtr<FJointSchemaAction_NewNodePreset> NodePresetAction = StaticCastSharedPtr<FJointSchemaAction_NewNodePreset>(EdGraphSchemaAction);

			if (NodePresetAction->NodePreset == nullptr) continue;
			
			// get current center location of the graph view to spawn the node preset at
			
#if UE_VERSION_OLDER_THAN(5, 6, 0)
			FVector2D ViewLocation;
			float ZoomAmount;
			FVector2D ViewSize = ToolKitPtr.Pin()->GetFocusedGraphEditor()->GetDesiredSize();
#else
			FVector2f ViewLocation;
			float ZoomAmount;
			FVector2f ViewSize = ToolKitPtr.Pin()->GetFocusedGraphEditor()->GetDesiredSize();
#endif
			
			ToolKitPtr.Pin()->GetFocusedGraphEditor()->GetViewLocation(ViewLocation, ZoomAmount);
			float ZoomOffsetMul = 1 / ZoomAmount;
			ViewLocation = ViewLocation + ViewSize * ZoomOffsetMul * 2.5;
			NodePresetAction->PerformAction(ToolKitPtr.Pin()->GetFocusedJointGraph(), nullptr, ViewLocation);
		}
		else
		{
			continue;
		}
	}
}
PRAGMA_ENABLE_OPTIMIZATION

void SJointNodePalette::RebuildWidget()
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
		.OnActionSelected(this, &SJointNodePalette::OnActionSelected)
	];
}



#undef LOCTEXT_NAMESPACE