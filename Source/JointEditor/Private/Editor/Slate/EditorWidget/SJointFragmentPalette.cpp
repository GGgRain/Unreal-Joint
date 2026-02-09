//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorWidget/SJointFragmentPalette.h"

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
		SNew(SJointGraphEditorActionMenu)
		.GraphObj(ToolKitPtr.Pin()->GetFocusedJointGraph())
		.bUseCustomActionSelected(true)
		.AutoExpandActionMenu(true)
		.OnActionSelected(this, &SJointFragmentPalette::OnFragmentActionSelected)
	];
}



#undef LOCTEXT_NAMESPACE