//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "SearchTree/Slate/SJointTreeFilter.h"

#include "JointEditorStyle.h"
#include "Filter/JointTreeFilter.h"
#include "Filter/JointTreeFilterItem.h"

#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

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
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Frame)
		[
			SNew(SButton)
			.ContentPadding(FJointEditorStyle::Margin_Border)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round")
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(this, &SJointTreeFilter::OnAddNewFilterButtonClicked)
			[
				SNew(STextBlock)
				.Visibility(EVisibility::HitTestInvisible)
				.Text(LOCTEXT("AddNewFilter", "Add New Filter"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Frame)
		[
			SAssignNew(FilterWrapBox, SWrapBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.Orientation(Orient_Horizontal)
			.UseAllottedSize(true)
			.InnerSlotPadding(FVector2D(4,4))
		]
	];
}

void SJointTreeFilter::UpdateFilterBox()
{
	if (FilterWrapBox.IsValid() && Filter.IsValid())
	{
		FilterWrapBox->ClearChildren();

		for (const TSharedPtr<FJointTreeFilterItem>& JointTreeFilterItem : Filter->GetItems())
		{
			if(!JointTreeFilterItem->GetFilterWidget().IsValid()) continue;
			
			FilterWrapBox->AddSlot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					JointTreeFilterItem->GetFilterWidget().ToSharedRef()
				];
		}
	}
}

FReply SJointTreeFilter::OnAddNewFilterButtonClicked() const
{
	if (Filter.IsValid()) Filter->AddItem(MakeShareable(new FJointTreeFilterItem("New_Tag")));

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
