//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "SearchTree/Filter/JointTreeFilterItem.h"

#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "Components/HorizontalBox.h"
#include "Filter/JointTreeFilter.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Layout/SBox.h"

void FJointTreeFilterItem::MakeFilterWidget()
{
	TAttribute<FSlateColor> EnabledColor_Attr = TAttribute<FSlateColor>::CreateLambda([this]
	{
		if (this) return GetIsEnabled() ? FLinearColor(0.2, 0.2, 0.3) : FLinearColor(0.05, 0.05, 0.08);

		return FJointEditorStyle::Color_Hover;
	});


	Widget = SNew(SButton)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
		.ButtonColorAndOpacity(EnabledColor_Attr)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.OnClicked(this, &FJointTreeFilterItem::OnItemEnableButtonClicked)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SEditableText)
				.Text(FText::FromString(FilterString))
				.Style(FJointEditorStyle::Get(), "JointUI.EditableText.Tag")
				.OnTextChanged(this, &FJointTreeFilterItem::OnItemFilterTextChanged)
				.OnTextCommitted(this, &FJointTreeFilterItem::OnItemFilterTextCommitted)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SJointOutlineButton)
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.NormalColor(FLinearColor::Transparent)
				.OutlineNormalColor(FLinearColor::Transparent)
				.OnClicked(this, &FJointTreeFilterItem::OnItemRemoveButtonClicked)
				[
					SNew(SBox)
					.WidthOverride(16)
					.HeightOverride(16)
					[
						SNew(SImage)
						.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.X"))
					]
				]
			]
		];
}

FReply FJointTreeFilterItem::OnItemEnableButtonClicked()
{
	bIsEnabled = !bIsEnabled;

	NotifyItemDataChanged();

	return FReply::Handled();
}

FReply FJointTreeFilterItem::OnItemRemoveButtonClicked()
{
	if (GetOwnerTreeFilter().IsValid())
	{
		GetOwnerTreeFilter().Pin()->RemoveItem(AsShared());
	}
	return FReply::Handled();
}

void FJointTreeFilterItem::NotifyItemDataChanged()
{
	if (OnJointFilterItemDataChanged.IsBound()) OnJointFilterItemDataChanged.Broadcast();
}

void FJointTreeFilterItem::OnItemFilterTextCommitted(const FText& Text, ETextCommit::Type Arg)
{
	FilterString = Text.ToString();

	NotifyItemDataChanged();
}

void FJointTreeFilterItem::OnItemFilterTextChanged(const FText& Text)
{
	FilterString = Text.ToString();

	NotifyItemDataChanged();
}

void FJointTreeFilterItem::SetOwnerTreeFilter(const TWeakPtr<FJointTreeFilter>& InOwnerTreeFilter)
{
	OwnerTreeFilter = InOwnerTreeFilter;
}

const TWeakPtr<FJointTreeFilter>& FJointTreeFilterItem::GetOwnerTreeFilter()
{
	return OwnerTreeFilter;
}

void FJointTreeFilterItem::SetIsEnabled(const bool NewIsEnabled)
{
	bIsEnabled = NewIsEnabled;

	NotifyItemDataChanged();
}

FJointTreeFilterItem::FJointTreeFilterItem(): bIsEnabled(true)
{
}

FJointTreeFilterItem::FJointTreeFilterItem(const FString& FilterString) : FilterString(FilterString), bIsEnabled(true)
{
}

TSharedPtr<SWidget> FJointTreeFilterItem::GetFilterWidget()
{
	if (!Widget.IsValid()) MakeFilterWidget();

	return Widget;
}

bool FJointTreeFilterItem::GetIsEnabled() const
{
	return bIsEnabled;
}

bool FJointTreeFilterItem::IsEqual(const TSharedPtr<FJointTreeFilterItem>& Other) const
{
	if (Other.IsValid()) return this->FilterString == Other->FilterString;

	return false;
}

const FString FJointTreeFilterItem::ExtractFilterItem()
{
	return GetIsEnabled() ? FilterString.Replace(TEXT(" "), TEXT("_")) : "";
}

FText FJointTreeFilterItem::ExtractFilterItemText()
{
	return FText::FromString(ExtractFilterItem());
}
