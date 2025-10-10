//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "SearchTree/Slate/SJointTreeFilter.h"

#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "Filter/JointTreeFilter.h"
#include "Filter/JointTreeFilterItem.h"

#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "SJointTreeFilter"


void SJointTreeFilter::Construct(const FArguments& InArgs)
{
	Filter = InArgs._Filter;

	Filter->OnJointFilterChanged.AddSP(this, &SJointTreeFilter::UpdateFilterBox);

	ConstructLayout();
}

void SJointTreeFilter::ConstructLayout()
{
	this->ChildSlot.DetachWidget();

	this->ChildSlot
	[
		SAssignNew(FilterWrapBox, SWrapBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.Orientation(Orient_Horizontal)
		.UseAllottedSize(true)
		.InnerSlotPadding(FVector2D(4, 4))
	];

	FilterWrapBox->AddSlot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				PopulateAddButtonIfNeeded().ToSharedRef()
			];
}

void SJointTreeFilter::UpdateFilterBox()
{
	if (FilterWrapBox.IsValid() && Filter.IsValid())
	{
		FilterWrapBox->ClearChildren();

		for (const TSharedPtr<FJointTreeFilterItem>& JointTreeFilterItem : Filter->GetItems())
		{
			if (!JointTreeFilterItem->GetFilterWidget().IsValid()) continue;

			FilterWrapBox->AddSlot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					JointTreeFilterItem->GetFilterWidget().ToSharedRef()
				];
		}

		FilterWrapBox->AddSlot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				PopulateAddButtonIfNeeded().ToSharedRef()
			];
	}
}

FReply SJointTreeFilter::OnAddNewFilterButtonClicked() const
{
	if (Filter.IsValid()) Filter->AddItem(MakeShareable(new FJointTreeFilterItem("New_Tag")));

	return FReply::Handled();
}

TSharedPtr<SWidget> SJointTreeFilter::PopulateAddButtonIfNeeded()
{
	if (FilterAddButton.IsValid()) return FilterAddButton;

	FilterAddButton = SNew(SJointOutlineButton)
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.OnClicked(this, &SJointTreeFilter::OnAddNewFilterButtonClicked)
		.NormalColor(FLinearColor::Transparent)
		.OutlineNormalColor(FLinearColor::Transparent)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				SNew(SBox)
				.WidthOverride(16)
				.HeightOverride(16)
				.Visibility(EVisibility::HitTestInvisible)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Plus"))
				]
			]
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AddNewFilter", "Add New Filter.."))
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
			]
		];

	return FilterAddButton;
}

#undef LOCTEXT_NAMESPACE
