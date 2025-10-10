//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "ItemTag/JointTreeItemTag_ManagerFragment.h"

#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "Filter/JointTreeFilter.h"
#include "Filter/JointTreeFilterItem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FJointTreeItemTag_ManagerFragment"


FJointTreeItemTag_ManagerFragment::FJointTreeItemTag_ManagerFragment(const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter) : OwnerJointTreeFilter(InOwnerJointTreeFilter)
{
}
FReply FJointTreeItemTag_ManagerFragment::AddFilterText()
{

	if(OwnerJointTreeFilter.IsValid())
	{
		OwnerJointTreeFilter.Pin()->AddItem(MakeShareable(new FJointTreeFilterItem(GetFilterText().ToString())));
	}
	
	return FReply::Handled();
}

TSharedRef<SWidget> FJointTreeItemTag_ManagerFragment::MakeTagWidget()
{
	return SNew(SJointOutlineButton)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
		.NormalColor(FLinearColor(0.02, 0.1, 0.1))
		.HoverColor(FLinearColor(0.02, 0.1, 0.1) * 2)
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.OnClicked(this, &FJointTreeItemTag_ManagerFragment::AddFilterText)
		[
			SNew(STextBlock)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.Text(LOCTEXT("ManagerFragmentText", "Manager Fragment"))
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
			.ToolTipText(FText::Format(LOCTEXT("NodeFilterTextToolTip","Press it to add filter for this tag: '{0}'"), GetFilterText()))
		];
}

FText FJointTreeItemTag_ManagerFragment::GetFilterText()
{
	return LOCTEXT("ManagerFragmentText", "Tag:Manager Fragment");
}


#undef LOCTEXT_NAMESPACE
