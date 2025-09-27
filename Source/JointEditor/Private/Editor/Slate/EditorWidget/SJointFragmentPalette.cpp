﻿//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorWidget/SJointFragmentPalette.h"

#include "JointEditorToolkit.h"
#include "EditorWidget/SJointGraphEditorActionMenu.h"

#include "JointEditorStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Misc/EngineVersionComparison.h"

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
		FText NotificationText = FText::FromString("Can not add a fragment");
		FText NotificationSubText = FText::FromString(
			"Nodes to add the fragment have not been selected from the graph.");

		FNotificationInfo NotificationInfo(NotificationText);
		NotificationInfo.SubText = NotificationSubText;
		NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.FadeInDuration = 0.3f;
		NotificationInfo.FadeOutDuration = 1.3f;
		NotificationInfo.ExpireDuration = 4.5f;

		FSlateNotificationManager::Get().AddNotification(NotificationInfo);
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
		UE_LOG(LogTemp, Log, TEXT("Failed to find a valid editor reference. can not create a fragment palette."));

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



