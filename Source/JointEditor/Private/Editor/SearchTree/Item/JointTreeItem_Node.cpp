//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Item/JointTreeItem_Node.h"

#include "JointAdvancedWidgets.h"
#include "JointEdGraph.h"
#include "JointEditorStyle.h"
#include "ItemTag/JointTreeItemTag_ManagerFragment.h"
#include "ItemTag/JointTreeItemTag_Node.h"
#include "ItemTag/JointTreeItemTag_Type.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

//////////////////////////////////////////////////////////////////////////
// SJointList

#define LOCTEXT_NAMESPACE "JointPropertyList"


FJointTreeItem_Node::FJointTreeItem_Node(TWeakObjectPtr<UJointNodeBase> InNodePtr,
                                         const TSharedRef<SJointTree>& InTree)
	: FJointTreeItem(InTree)
	  , NodePtr(InNodePtr)
{
	AllocateItemTags();
}


FReply FJointTreeItem_Node::OnMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
{
	OnItemDoubleClicked();

	return FReply::Handled();
}

void FJointTreeItem_Node::GenerateWidgetForNameColumn(TSharedPtr<SHorizontalBox> Box,
                                                      const TAttribute<FText>& InFilterText,
                                                      FIsSelected InIsSelected)
{
	TAttribute<FSlateColor> NodeBorderColor = TAttribute<FSlateColor>::CreateLambda([this]
	{
		constexpr FLinearColor DefaultColor = FLinearColor(0.1, 0.1, 0.1, 1);

		if (NodePtr.Get() && NodePtr->GetJointManager() && NodePtr->GetJointManager()->JointGraph && Cast<
			UJointEdGraph>(NodePtr->GetJointManager()->JointGraph))
		{
			TSet<TSoftObjectPtr<UJointEdGraphNode>> GraphNodes = Cast<UJointEdGraph>(
				NodePtr->GetJointManager()->JointGraph)->GetCachedJointGraphNodes();

			for (TSoftObjectPtr<UJointEdGraphNode> CachedGraphNode : GraphNodes)
			{
				if (!CachedGraphNode.IsValid()) continue;

				if (CachedGraphNode->NodeInstance == NodePtr) return CachedGraphNode->GetNodeTitleColor();
			}
		}
		return DefaultColor;
	});

	TAttribute<FText> NodeName_Attr = TAttribute<FText>::CreateLambda([this]
	{
		return GetObject() ? FText::FromName(GetObject()->GetFName()) : LOCTEXT("NodeName_Null", "INVALID OBJECT");
	});

	Box->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SJointOutlineBorder)
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.NormalColor(FJointEditorStyle::Color_Normal)
			.ContentPadding(FJointEditorStyle::Margin_Normal)
			.OnMouseDoubleClick(this, &FJointTreeItem_Node::OnMouseDoubleClick)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Normal)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(NodeBorderColor)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SInlineEditableTextBlock)
					.Text(NodeName_Attr)
					.HighlightText(InFilterText)
					.IsReadOnly(true)
				]
			]
		];

	Box->AddSlot()
		.AutoWidth()
		.Padding(FMargin(6, 0))
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			MakeItemTagContainerWidget()
		];
}


TSharedRef<SWidget> FJointTreeItem_Node::GenerateWidgetForDataColumn(const TAttribute<FText>& InFilterText,
                                                                     const FName& DataColumnName,
                                                                     FIsSelected InIsSelected)
{
	return SNullWidget::NullWidget;
}


FName FJointTreeItem_Node::GetRowItemName() const
{
	return NodePtr != nullptr ? FName(NodePtr->GetPathName()) : NAME_None;
}

void FJointTreeItem_Node::OnItemDoubleClicked()
{
	if (GetJointPropertyTree() && NodePtr.Get()) GetJointPropertyTree()->JumpToHyperlink(NodePtr.Get());
}

UObject* FJointTreeItem_Node::GetObject() const
{
	return NodePtr.Get();
}

void FJointTreeItem_Node::AllocateItemTags()
{
	if (GetJointPropertyTree())
	{
		ItemTags.Add(MakeShareable(new FJointTreeItemTag_Node(GetJointPropertyTree()->Filter)));

		if (NodePtr.Get() && NodePtr->GetJointManager() && NodePtr->GetJointManager()->ManagerFragments.Contains(NodePtr))
			ItemTags.Add(MakeShareable(new FJointTreeItemTag_ManagerFragment(GetJointPropertyTree()->Filter)));

		ItemTags.Add(MakeShareable(
			new FJointTreeItemTag_Type(
				NodePtr.Get() ? FText::FromString(NodePtr->GetClass()->GetName()) : FText::GetEmpty()
				, GetJointPropertyTree()->Filter)));
	}
}

TSet<TSharedPtr<IJointTreeItemTag>> FJointTreeItem_Node::GetItemTags()
{
	return ItemTags;
}

FString FJointTreeItem_Node::GetReferencerName() const
{
	return TEXT("FJointTreeItem_Node");
}

#include "Misc/EngineVersionComparison.h"

void FJointTreeItem_Node::AddReferencedObjects(FReferenceCollector& Collector)
{
}


#undef LOCTEXT_NAMESPACE
