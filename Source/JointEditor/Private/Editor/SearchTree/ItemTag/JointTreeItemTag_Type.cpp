//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "ItemTag/JointTreeItemTag_Type.h"

#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "Components/HorizontalBox.h"
#include "Filter/JointTreeFilter.h"
#include "Filter/JointTreeFilterItem.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FJointTreeItemTag_Type"


FJointTreeItemTag_Type::FJointTreeItemTag_Type(const FText& InType, const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter): Type(InType), Icon(nullptr), OwnerJointTreeFilter(InOwnerJointTreeFilter)
{
}

FJointTreeItemTag_Type::FJointTreeItemTag_Type(const FSlateColor& InColor, const FSlateBrush* InIcon,
                                                     const FText& InType, const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter) : Type(InType), Color(InColor), Icon(InIcon), OwnerJointTreeFilter(InOwnerJointTreeFilter)
{
}

FReply FJointTreeItemTag_Type::AddFilterText()
{
	if(OwnerJointTreeFilter.IsValid())
	{
		OwnerJointTreeFilter.Pin()->AddItem(MakeShareable(new FJointTreeFilterItem(GetFilterText().ToString())));
	}
	
	return FReply::Handled();
}

TSharedRef<SWidget> FJointTreeItemTag_Type::MakeTagWidget()
{
	if (Icon == nullptr)
	{
		return SNew(SJointOutlineButton)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
			.NormalColor(FLinearColor(0.02, 0.02, 0.03))
			.HoverColor(FLinearColor(0.02, 0.02, 0.03) * 2)
			.ContentPadding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(this, &FJointTreeItemTag_Type::AddFilterText)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FMargin(2,0))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Type)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
					.ToolTipText(FText::Format(LOCTEXT("NodeFilterTextToolTip","Press it to add filter for this tag: '{0}'"), GetFilterText()))
				]
			];
	}
	else
	{
		return SNew(SJointOutlineButton)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
			.NormalColor(FLinearColor(0.02, 0.02, 0.03))
			.HoverColor(FLinearColor(0.02, 0.02, 0.03) * 2)
			.ContentPadding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(this, &FJointTreeItemTag_Type::AddFilterText)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FMargin(2,0))
				[
					SNew(SBox)
					.WidthOverride(16)
					.HeightOverride(16)
					[
						SNew(SImage)
						.Image(Icon)
						.ColorAndOpacity(Color)
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FMargin(2,0))
				[
					SNew(STextBlock)
					.Text(Type)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
					.ToolTipText(FText::Format(LOCTEXT("NodeFilterTextToolTip","Press it to add filter for this tag: '{0}'"), GetFilterText()))
				]
			];
	}
}

FText FJointTreeItemTag_Type::GetFilterText()
{
	return FText::FromString("Tag:" + Type.ToString());
}



#undef LOCTEXT_NAMESPACE
