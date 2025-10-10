//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "ItemTag/JointTreeItemTag_Graph.h"

#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"

#include "Filter/JointTreeFilter.h"
#include "Filter/JointTreeFilterItem.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FJointTreeItemTag_Graph"


FJointTreeItemTag_Graph::FJointTreeItemTag_Graph(const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter) : OwnerJointTreeFilter(InOwnerJointTreeFilter)
{
}

FReply FJointTreeItemTag_Graph::AddFilterText()
{
	if(OwnerJointTreeFilter.IsValid()) OwnerJointTreeFilter.Pin()->AddItem(MakeShareable(new FJointTreeFilterItem(GetFilterText().ToString())));
	
	return FReply::Handled();
}

TSharedRef<SWidget> FJointTreeItemTag_Graph::MakeTagWidget()
{
	return SNew(SJointOutlineButton)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
		.NormalColor(FLinearColor(0.2,0.3,0.5))
		.HoverColor(FLinearColor(0.2,0.3,0.5) * 2)
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.OnClicked(this, &FJointTreeItemTag_Graph::AddFilterText)
		[
			SNew(STextBlock)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.Text(LOCTEXT("GraphFilterText","Graph"))
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
			.ToolTipText(FText::Format(LOCTEXT("GraphFilterTextToolTip","Press it to add filter for this tag: '{0}'"), GetFilterText()))
		];
}

FText FJointTreeItemTag_Graph::GetFilterText()
{
	return LOCTEXT("NodeFilterText","Tag:Graph");
}



#undef LOCTEXT_NAMESPACE
