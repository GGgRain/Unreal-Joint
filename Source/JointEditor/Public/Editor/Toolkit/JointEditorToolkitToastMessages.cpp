#include <JointEditorToolkitToastMessages.h>

#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "EditorWidget/JointToolkitToastMessages.h"

#define LOCTEXT_NAMESPACE "JointEditorToolkitToastMessages"

void JointEditorToolkitToastMessages::PopulateNodePickingToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr)
{
	PopulateToastMessage(
		ToolkitPtr,
		EJointEditorToastMessage::NodePicking,
		SNew(SJointToolkitToastMessage)
		[
			SNew(SBorder)
			.Padding(FJointEditorStyle::Margin_Normal)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal * 2)
			[
				SNew(SVerticalBox)
				.Clipping(EWidgetClipping::ClipToBounds)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SBox)
						.WidthOverride(24)
						.HeightOverride(24)
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.EyeDropper"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Clipping(EWidgetClipping::ClipToBounds)
						.Text(LOCTEXT("PickingEnabledTitle", "Node Picking Enabled"))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(STextBlock)
					.Clipping(EWidgetClipping::ClipToBounds)
					.Text(LOCTEXT("PickingEnabled", "Click the node to select. Press ESC to escape."))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
				]
			]
		]
	);
}

void JointEditorToolkitToastMessages::PopulateQuickNodePickingToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr)
{
	PopulateToastMessage(
		ToolkitPtr,
		EJointEditorToastMessage::QuickNodePicking,
		SNew(SJointToolkitToastMessage)
		[
			SNew(SBorder)
			.Padding(FJointEditorStyle::Margin_Normal)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal * 2)
			[
				SNew(SVerticalBox)
				.Clipping(EWidgetClipping::ClipToBounds)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SBox)
						.WidthOverride(24)
						.HeightOverride(24)
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.EyeDropper"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Clipping(EWidgetClipping::ClipToBounds)
						.Text(LOCTEXT("PickingEnabledTitle", "Quick Node Picking Enabled"))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(STextBlock)
					.Clipping(EWidgetClipping::ClipToBounds)
					.Text(LOCTEXT("PickingEnabled", "Click the node to pick the reference of. Press ESC to escape.\nOnce you pick a node, paste it on your node pointers."))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
				]
			]
		]
	);
}

void JointEditorToolkitToastMessages::PopulateTransientEditingWarningToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr)
{
	PopulateToastMessage(
		ToolkitPtr,
		EJointEditorToastMessage::TransientEditingWarning,
		SNew(SJointToolkitToastMessage)
		[
			SNew(SBorder)
			.Padding(FJointEditorStyle::Margin_Normal)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal * 2)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SBox)
						.WidthOverride(24)
						.HeightOverride(24)
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Star"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Clipping(EWidgetClipping::ClipToBounds)
						.Text(LOCTEXT("PickingEnabledTitle",
						              "You are editing a transient & PIE duplicated object."))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(STextBlock)
					.Clipping(EWidgetClipping::ClipToBounds)
					.Text(LOCTEXT("PickingEnabled",
					              "Any modification on this graph will not be applied to the original asset."))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
				]
			]
		]
	);
}

void JointEditorToolkitToastMessages::PopulateNonStandaloneAssetEditingWarningToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr)
{
	PopulateToastMessage(
		ToolkitPtr,
		EJointEditorToastMessage::NonStandaloneAssetEditingWarning,
		SNew(SJointToolkitToastMessage)
		[
			SNew(SBorder)
			.Padding(FJointEditorStyle::Margin_Normal)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal * 2)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SBox)
						.WidthOverride(24)
						.HeightOverride(24)
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Star"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Clipping(EWidgetClipping::ClipToBounds)
						.Text(LOCTEXT("NonStandaloneEditTitle", "You are editing a non-standalone asset."))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(STextBlock)
					.Clipping(EWidgetClipping::ClipToBounds)
					.Text(LOCTEXT("NonStandaloneEditDesc",
					              "This Joint manager must be used with internal usages only, and should not be referenced by the other parts of the system.\nJoint Node Preset uses this type of Joint Manager, so if you are creating or editing a Joint Node Preset, this is expected and you can ignore this message."))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
				]
			]
		]
	);
}

void JointEditorToolkitToastMessages::PopulateVisibilityChangeModeForSimpleDisplayPropertyToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr)
{
	PopulateToastMessage(
		ToolkitPtr,
		EJointEditorToastMessage::VisibilityChangeModeForSimpleDisplayProperty,
		SNew(SJointToolkitToastMessage)
		[
			SNew(SBorder)
			.Padding(FJointEditorStyle::Margin_Normal)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SVerticalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						.Clipping(EWidgetClipping::ClipToBounds)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SBox)
							.WidthOverride(24)
							.HeightOverride(24)
							[
								SNew(SImage)
								.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Edit"))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Clipping(EWidgetClipping::ClipToBounds)
							.Text(LOCTEXT("PickingEnabledTitle",
							              "Modifying Simple Display Property Visibility"))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Clipping(EWidgetClipping::ClipToBounds)
						.Text(LOCTEXT("PickingEnabled",
						              "Press the eye buttons to change their visibility. Press \'X\' to end."))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
					]
				]
			]
		]
	);
}

void JointEditorToolkitToastMessages::PopulateNodePickerCopyToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr)
{
	PopulateToastMessage(
		ToolkitPtr,
		EJointEditorToastMessage::NodePickerCopy,

		SNew(SJointToolkitToastMessage)
		.SizeDecreaseInterpolationSpeed(2)
		.RemoveAnimationDuration(0.5)
		.Duration(1.5)
		[
			SNew(SBorder)
			.Padding(FJointEditorStyle::Margin_Normal)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SVerticalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SBox)
							.WidthOverride(24)
							.HeightOverride(24)
							[
								SNew(SImage)
								.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Edit"))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Clipping(EWidgetClipping::ClipToBounds)
							.Text(LOCTEXT("CopyTitle",
							              "Node Pointer Copied!"))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Clipping(EWidgetClipping::ClipToBounds)
						.Text(LOCTEXT("CopyTitleEnabled",
						              "Press paste button on others to put this there"))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
					]
				]
			]
		]

	);
}

void JointEditorToolkitToastMessages::PopulateNodePickerPastedToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr)
{
	PopulateToastMessage(
		ToolkitPtr,
		EJointEditorToastMessage::NodePickerPasted,
		SNew(SJointToolkitToastMessage)
		.SizeDecreaseInterpolationSpeed(2)
		.RemoveAnimationDuration(0.5)
		.Duration(1.5)
		[
			SNew(SBorder)
			.Padding(FJointEditorStyle::Margin_Normal)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SVerticalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SBox)
							.WidthOverride(24)
							.HeightOverride(24)
							[
								SNew(SImage)
								.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Star"))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Clipping(EWidgetClipping::ClipToBounds)
							.Text(LOCTEXT("PasteTitle",
							              "Node Pointer Pasted!"))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
						]
					]
				]
			]
		]
	);
}

void JointEditorToolkitToastMessages::PopulateNeedReopeningToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr)
{
	PopulateToastMessage(
		ToolkitPtr,
		EJointEditorToastMessage::NeedReopening,
		SNew(SJointToolkitToastMessage)
		.Duration(-1)
		[
			SNew(SBorder)
			.Padding(FJointEditorStyle::Margin_Normal)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Node_Invalid)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SVerticalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SBox)
							.WidthOverride(24)
							.HeightOverride(24)
							[
								SNew(SImage)
								.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Edit"))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Clipping(EWidgetClipping::ClipToBounds)
							.Text(LOCTEXT("ReopeningRequest",
							              "Graph Editor Reported Invalidated Slate Reference Error.\nPlease reopen the editor to solve the issue."))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
						]
					]
				]
			]
		]
	);
}

void JointEditorToolkitToastMessages::PopulateNodePresetEditingToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr)
{
	PopulateToastMessage(
		ToolkitPtr,
		EJointEditorToastMessage::NodePresetEditing,
		SNew(SJointToolkitToastMessage)
		.Duration(-1)
		[
			SNew(SBorder)
			.Padding(FJointEditorStyle::Margin_Normal)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SVerticalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SBox)
							.WidthOverride(24)
							.HeightOverride(24)
							[
								SNew(SImage)
								.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Edit"))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Clipping(EWidgetClipping::ClipToBounds)
							.Text(LOCTEXT("NodePresetEditing",
							              "You are editing a Joint Node Preset's Internal Joint Manager."))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
						]
					]
					]
				]
			]);
}

void JointEditorToolkitToastMessages::PopulateToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr, const FString& MessageKey, TSharedRef<SJointToolkitToastMessage> ToastMessage)
{
	if (ToolkitPtr.Pin() == nullptr) return;

	FGuid MessageGuid;

	if (ToolkitPtr.Pin()->MessageGuids.Contains(MessageKey))
	{
		MessageGuid = ToolkitPtr.Pin()->MessageGuids[MessageKey];

		ClearToastMessage(ToolkitPtr, MessageGuid);
	}

	TWeakPtr<SJointToolkitToastMessage> Message = ToolkitPtr.Pin()->GetOrCreateGraphToastMessageHub()->FindToasterMessage(MessageGuid);

	if (Message.IsValid())
	{
		Message.Pin()->PlayAppearAnimation();
	}
	else
	{
		MessageGuid = ToolkitPtr.Pin()->GetOrCreateGraphToastMessageHub()->AddToasterMessage(
			ToastMessage
		);

		ToolkitPtr.Pin()->MessageGuids.Add(MessageKey, MessageGuid);
	}
}

void JointEditorToolkitToastMessages::ClearToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr, const FGuid& MessageGuid)
{
	if (ToolkitPtr.Pin() == nullptr) return;

	if (ToolkitPtr.Pin()->GetOrCreateGraphToastMessageHub()->HasToasterMessage(MessageGuid))
	{
		ToolkitPtr.Pin()->GetOrCreateGraphToastMessageHub()->RemoveToasterMessage(MessageGuid);
	}
}

void JointEditorToolkitToastMessages::ClearToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr, const FString& MessageKey)
{
	if (ToolkitPtr.Pin() == nullptr) return;

	if (ToolkitPtr.Pin()->MessageGuids.Contains(MessageKey))
	{
		ClearToastMessage(ToolkitPtr, ToolkitPtr.Pin()->MessageGuids[MessageKey]);

		ToolkitPtr.Pin()->MessageGuids.Remove(MessageKey);
	}
}

#undef LOCTEXT_NAMESPACE
