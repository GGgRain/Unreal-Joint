//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Markdown/SJointMDSlate_Admonitions.h"

#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "Widgets/Images/SImage.h"


void SJointMDSlate_Admonitions::Construct(const FArguments& InArgs)
{
	AdmonitionType = InArgs._AdmonitionType;
	
	FLinearColor BorderColor = FLinearColor::White;
	switch (AdmonitionType)
	{
	case EJointMDAdmonitionType::Info:
		BorderColor = FLinearColor(1.0f, 1.0f, 1.0f);
		break;
	case EJointMDAdmonitionType::Mention:
		BorderColor = FLinearColor(0.3f, 0.7f, 1.0f);
		break;
	case EJointMDAdmonitionType::Note:
		BorderColor = FLinearColor(0.1f, 1.0f, 0.5f);
		break;
	case EJointMDAdmonitionType::Warning:
		BorderColor = FLinearColor(1.0f, 0.6f, 0.0f);
		break;
	case EJointMDAdmonitionType::Caution:
		BorderColor = FLinearColor(1.0f, 0.4f, 0.0f);
		break;
	case EJointMDAdmonitionType::Error:
		BorderColor = FLinearColor(1.0f, 0.0f, 0.0f);
		break;
	case EJointMDAdmonitionType::Important:
		BorderColor = FLinearColor(0.3f, 0.7f, 1.0f);
		break;
	default:
		break;
	}
	
	// icon
	
	const FSlateBrush* IconBrush = nullptr;
	
	switch (AdmonitionType)
	{
	case EJointMDAdmonitionType::Info:
		IconBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Info");
		break;
	case EJointMDAdmonitionType::Mention:
		IconBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Info");
		break;
	case EJointMDAdmonitionType::Note:
		IconBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Info");
		break;
	case EJointMDAdmonitionType::Warning:
		IconBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Warning");
		break;
	case EJointMDAdmonitionType::Caution:
		IconBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Warning");
		break;
	case EJointMDAdmonitionType::Error:
		IconBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Error");
		break;
	case EJointMDAdmonitionType::Important:
		IconBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Error");
		break;
	default: 
		IconBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Info");
		break;
	}
	
	const FVector2D IconSize = FVector2D(16, 16);
	
	FText HeaderText = InArgs._CustomHeaderText.IsEmpty() ? FText::FromString(TEXT("")) : InArgs._CustomHeaderText;
	
	if (HeaderText.IsEmpty() && 
		(InArgs._AdmonitionStyleType == EJointMDAdmonitionStyleType::Vertical_Verbose ||
		 InArgs._AdmonitionStyleType == EJointMDAdmonitionStyleType::Horizontal_Verbose))
	{
		switch (AdmonitionType)
		{
		case EJointMDAdmonitionType::Info:
			HeaderText = FText::FromString(TEXT("Info"));
			break;
		case EJointMDAdmonitionType::Mention:
			HeaderText = FText::FromString(TEXT("Mention"));
			break;
		case EJointMDAdmonitionType::Note:
			HeaderText = FText::FromString(TEXT("Note"));
			break;
		case EJointMDAdmonitionType::Warning:
			HeaderText = FText::FromString(TEXT("Warning"));
			break;
		case EJointMDAdmonitionType::Caution:
			HeaderText = FText::FromString(TEXT("Caution"));
			break;
		case EJointMDAdmonitionType::Important:
			HeaderText = FText::FromString(TEXT("Important"));
			break;
		case EJointMDAdmonitionType::Error:
			HeaderText = FText::FromString(TEXT("Error"));
			break;
		}
	}
	
	
	TSharedPtr<SWidget> ContentWidget = nullptr;
	
	if (InArgs._bUseDescriptionText)
	{
		ContentWidget = 
			SNew(STextBlock)
			.TextStyle(&FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Regular.h3"))
			.Text(InArgs._DescriptionText)
			.AutoWrapText(InArgs._bAutoWrapDescriptionText);
	}
	else
	{
		ContentWidget = InArgs._Content.Widget;
	}
	
	
	
	switch (InArgs._AdmonitionStyleType)
	{
		case EJointMDAdmonitionStyleType::Vertical_Verbose:
			this->ChildSlot
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OutlineNormalColor(BorderColor + FLinearColor(0.0f, 0.0f, 0.0f))
				.OutlineHoverColor(BorderColor + FLinearColor(0.2f, 0.2f, 0.2f))
				.NormalColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.75f))
				.HoverColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.5f))
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot() // icon
					.AutoHeight()
					.HAlign(HAlign_Left)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SImage)
							.Image(IconBrush)
							.DesiredSizeOverride(IconSize)	
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Tiny)
						[
							SNew(STextBlock)
							.TextStyle(&FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Black.h4"))
							.Text(HeaderText)
						]
					]
					+ SVerticalBox::Slot() // content
					.FillHeight(1.0f)
					.HAlign(HAlign_Fill)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						ContentWidget.ToSharedRef()
					]
				]
			];
		break;
	case EJointMDAdmonitionStyleType::Vertical_NoHeaderText:
		break;
	case EJointMDAdmonitionStyleType::Vertical_NoIcon:
		break;
	case EJointMDAdmonitionStyleType::Horizontal_Verbose:
		this->ChildSlot
		[
			SNew(SJointOutlineBorder)
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(BorderColor)
			.OutlineHoverColor(BorderColor + FLinearColor(0.2f, 0.2f, 0.2f))
			.NormalColor(FJointEditorStyle::Color_Normal)
			.ContentPadding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot() // icon
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SImage)
					.Image(IconBrush)
					.DesiredSizeOverride(IconSize)
				]
				+ SHorizontalBox::Slot() // content
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					ContentWidget.ToSharedRef()
				]
			]
		];
		break;
	case EJointMDAdmonitionStyleType::Horizontal_NoHeaderText:
		this->ChildSlot
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OutlineNormalColor(BorderColor)
				.OutlineHoverColor(BorderColor + FLinearColor(0.2f, 0.2f, 0.2f))
				.NormalColor(FJointEditorStyle::Color_Normal)
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot() // content
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						ContentWidget.ToSharedRef()
					]
				]
			];
		break;
	case EJointMDAdmonitionStyleType::Horizontal_NoIcon:
		this->ChildSlot
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OutlineNormalColor(BorderColor)
				.OutlineHoverColor(BorderColor + FLinearColor(0.2f, 0.2f, 0.2f))
				.NormalColor(FJointEditorStyle::Color_Normal)
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot() // content
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						ContentWidget.ToSharedRef()
					]
				]
			];
		break;
	case EJointMDAdmonitionStyleType::OnlyContent:
		this->ChildSlot
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OutlineNormalColor(BorderColor)
				.OutlineHoverColor(BorderColor + FLinearColor(0.2f, 0.2f, 0.2f))
				.NormalColor(FJointEditorStyle::Color_Normal)
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					ContentWidget.ToSharedRef()
				]
			];
		break;
	}
	
}
