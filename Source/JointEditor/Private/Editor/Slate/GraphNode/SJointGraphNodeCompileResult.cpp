//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "GraphNode/SJointGraphNodeCompileResult.h"

#include "JointEditorStyle.h"
#include "Components/HorizontalBox.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SJointGraphNodeCompileResult"

void SJointGraphNodeCompileResult::Construct(const FArguments& InArgs)
{
	GraphNodeBase = InArgs._GraphNodeBase.Pin();

	SetVisibility(EVisibility::Collapsed);
	SetCanTick(false);
	RebuildWidget();
}

void SJointGraphNodeCompileResult::RebuildWidget()
{
	this->ChildSlot.DetachWidget();

	ErrorCountToolTip = SNew(SToolTip);
	WarningCountToolTip = SNew(SToolTip);
	InfoCountToolTip = SNew(SToolTip);

	this->ChildSlot
	[

		SNew(SHorizontalBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(ErrorCountButton, SButton)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
			.Visibility(EVisibility::Collapsed)
			.ButtonColorAndOpacity(FLinearColor(0.2, 0.05, 0.05, 0.80))
			.ContentPadding(FJointEditorStyle::Margin_Normal)
			.ToolTip(ErrorCountToolTip)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(0, 0, 2, 0))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.ErrorWithColor"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(ErrorCountTextBlock, STextBlock)
					.Text(LOCTEXT("Invalid", "NOT PROVIDED"))
				]
			]
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(WarningCountButton, SButton)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
			.Visibility(EVisibility::Collapsed)
			.ButtonColorAndOpacity(FLinearColor(0.2, 0.1, 0.05, 0.80))
			.ContentPadding(FJointEditorStyle::Margin_Normal)
			.ToolTip(WarningCountToolTip)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(0, 0, 2, 0))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.WarningWithColor"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(WarningCountTextBlock, STextBlock)
					.Text(LOCTEXT("Invalid", "NOT PROVIDED"))
				]
			]
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(InfoCountButton, SButton)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
			.Visibility(EVisibility::Collapsed)
			.ButtonColorAndOpacity(FLinearColor(0.02, 0.02, 0.02, 0.80))
			.ContentPadding(FJointEditorStyle::Margin_Normal)
			.ToolTip(InfoCountToolTip)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(0, 0, 2, 0))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.InfoWithColor"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(InfoCountTextBlock, STextBlock)
					.Text(LOCTEXT("Invalid", "NOT PROVIDED"))
				]
			]
		]

	];
}

void SJointGraphNodeCompileResult::UpdateWidgetWithCompileResult(
	const TArray<TSharedPtr<FTokenizedMessage>>& CompileResult)
{
	ErrorMessages.Empty();
	WarningMessages.Empty();
	InfoMessages.Empty();

	for (TSharedPtr<FTokenizedMessage> TokenizedMessage : CompileResult)
	{
		const EMessageSeverity::Type& Severity = TokenizedMessage->GetSeverity();

		if (Severity == EMessageSeverity::Error)
		{
			ErrorMessages.Add(TokenizedMessage);
		}
		else if (Severity == EMessageSeverity::Warning)
		{
			WarningMessages.Add(TokenizedMessage);
		}
		else if (Severity == EMessageSeverity::Info)
		{
			InfoMessages.Add(TokenizedMessage);
		}
		else if (Severity == EMessageSeverity::PerformanceWarning)
		{
			WarningMessages.Add(TokenizedMessage);
		}
	}

	const int& NumError = ErrorMessages.Num();
	const int& NumWarning= WarningMessages.Num();
	const int& NumInfo = InfoMessages.Num();

	ErrorCountButton->SetVisibility(NumError ? EVisibility::Visible : EVisibility::Collapsed);
	WarningCountButton->SetVisibility(NumWarning ? EVisibility::Visible : EVisibility::Collapsed);
	InfoCountButton->SetVisibility(NumInfo ? EVisibility::Visible : EVisibility::Collapsed);

	ErrorCountTextBlock->SetText(FText::FromString(FString::FromInt(NumError)));
	WarningCountTextBlock->SetText(FText::FromString(FString::FromInt(NumWarning)));
	InfoCountTextBlock->SetText(FText::FromString(FString::FromInt(NumInfo)));


	ErrorCountToolTip->SetContentWidget(GetErrorToolTip().ToSharedRef());
	WarningCountToolTip->SetContentWidget(GetWarningToolTip().ToSharedRef());
	InfoCountToolTip->SetContentWidget(GetInfoToolTip().ToSharedRef());

	if(NumError + NumWarning + NumInfo)
	{
		SetVisibility(EVisibility::Visible);
	}else
	{
		SetVisibility(EVisibility::Collapsed);
	}

}

TSharedPtr<SWidget> SJointGraphNodeCompileResult::GetErrorToolTip()
{
	TSharedPtr<SVerticalBox> OutVerticalBox = SNew(SVerticalBox);

	for (TWeakPtr<FTokenizedMessage>& Message : ErrorMessages)
	{
		OutVerticalBox->AddSlot()
		[
			SNew(SBorder)
			.Visibility(EVisibility::HitTestInvisible)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.Text(Message.Pin()->ToText())
				.WrapTextAt(1400)
			]
		];
	}

	return OutVerticalBox;
}

TSharedPtr<SWidget> SJointGraphNodeCompileResult::GetWarningToolTip()
{
	TSharedPtr<SVerticalBox> OutVerticalBox = SNew(SVerticalBox);

	for (TWeakPtr<FTokenizedMessage>& Message : WarningMessages)
	{
		OutVerticalBox->AddSlot()
		[
			SNew(SBorder)
			.Visibility(EVisibility::HitTestInvisible)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.Text(Message.Pin()->ToText())
			]
		];
	}

	return OutVerticalBox;

}

TSharedPtr<SWidget> SJointGraphNodeCompileResult::GetInfoToolTip()
{
	TSharedPtr<SVerticalBox> OutVerticalBox = SNew(SVerticalBox);

	for (TWeakPtr<FTokenizedMessage>& Message : InfoMessages)
	{
		OutVerticalBox->AddSlot()
		[
			SNew(SBorder)
			.Visibility(EVisibility::HitTestInvisible)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.Text(Message.Pin()->ToText())
			]
		];
	}

	return OutVerticalBox;
}


#undef LOCTEXT_NAMESPACE
