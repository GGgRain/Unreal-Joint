//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "ItemTag/JointTreeItemTag_Node.h"

#include "JointEditorStyle.h"

#include "Filter/JointTreeFilter.h"
#include "Filter/JointTreeFilterItem.h"

#define LOCTEXT_NAMESPACE "FJointTreeItemTag_Node"


FJointTreeItemTag_Node::FJointTreeItemTag_Node(const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter) : OwnerJointTreeFilter(InOwnerJointTreeFilter)
{
}

FReply FJointTreeItemTag_Node::AddFilterText()
{
	if(OwnerJointTreeFilter.IsValid()) OwnerJointTreeFilter.Pin()->AddItem(MakeShareable(new FJointTreeFilterItem(GetFilterText().ToString())));
	
	return FReply::Handled();
}

TSharedRef<SWidget> FJointTreeItemTag_Node::MakeTagWidget()
{
	return
		SNew(SButton)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
		.ButtonColorAndOpacity(FLinearColor(0.4,0.02,0.02))
		.ContentPadding(FJointEditorStyle::Margin_Tag)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.OnClicked(this, &FJointTreeItemTag_Node::AddFilterText)
		[
			SNew(STextBlock)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.Text(LOCTEXT("NodeFilterText","Node"))
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
			.ToolTipText(FText::Format(LOCTEXT("NodeFilterTextToolTip","Press it to add filter for this tag: '{0}'"), GetFilterText()))
		];
}

FText FJointTreeItemTag_Node::GetFilterText()
{
	return LOCTEXT("NodeFilterText","Tag:Node");
}



#undef LOCTEXT_NAMESPACE
