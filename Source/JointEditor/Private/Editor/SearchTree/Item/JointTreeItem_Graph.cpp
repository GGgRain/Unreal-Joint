//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Item/JointTreeItem_Graph.h"

#include "JointAdvancedWidgets.h"
#include "JointEdGraph.h"
#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "ItemTag/JointTreeItemTag_Graph.h"
#include "ItemTag/JointTreeItemTag_Node.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

//////////////////////////////////////////////////////////////////////////
// SJointList

#define LOCTEXT_NAMESPACE "JointPropertyList"


FJointTreeItem_Graph::FJointTreeItem_Graph(TWeakObjectPtr<UJointEdGraph> InJointEdGraph,
                                         const TSharedRef<SJointTree>& InTree)
	: FJointTreeItem(InTree)
	  , GraphPtr(InJointEdGraph)
{
	AllocateItemTags();
}


FReply FJointTreeItem_Graph::OnMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
{
	OnItemDoubleClicked();

	return FReply::Handled();
}

void FJointTreeItem_Graph::GenerateWidgetForNameColumn(TSharedPtr<SHorizontalBox> Box,
                                                      const TAttribute<FText>& InFilterText,
                                                      FIsSelected InIsSelected)
{

	TAttribute<FText> NodeName_Attr = TAttribute<FText>::CreateLambda([this]
	{
		return GetObject() ? FText::FromName(GetObject()->GetFName()) : LOCTEXT("NodeName_Null", "INVALID OBJECT");
	});

	// Get icon for graph
	FSlateBrush const* IconOut = nullptr;
	FJointEdUtils::GetGraphIconFor(GraphPtr.Get(),IconOut);
	
	Box->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SJointOutlineBorder)
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.NormalColor(FJointEditorStyle::Color_Hover)
			.HoverColor(FJointEditorStyle::Color_Hover + FLinearColor(0.1, 0.1, 0.1, 0.0))
			.ContentPadding(FJointEditorStyle::Margin_Large)
			.OnMouseDoubleClick(this, &FJointTreeItem_Graph::OnMouseDoubleClick)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Large)
				[
					SNew(SImage)
					.Image(IconOut)
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


TSharedRef<SWidget> FJointTreeItem_Graph::GenerateWidgetForDataColumn(const TAttribute<FText>& InFilterText,
                                                                     const FName& DataColumnName,
                                                                     FIsSelected InIsSelected)
{
	return SNullWidget::NullWidget;
}


FName FJointTreeItem_Graph::GetRowItemName() const
{
	return GraphPtr != nullptr ? FName(GraphPtr->GetPathName()) : NAME_None;
}

void FJointTreeItem_Graph::OnItemDoubleClicked()
{
	if (GetJointPropertyTree() && GraphPtr.Get()) GetJointPropertyTree()->JumpToHyperlink(GraphPtr.Get());
}

UObject* FJointTreeItem_Graph::GetObject() const
{
	return GraphPtr.Get();
}


TSet<TSharedPtr<IJointTreeItemTag>> FJointTreeItem_Graph::GetItemTags()
{
	return ItemTags;
}

void FJointTreeItem_Graph::AllocateItemTags()
{
	if (GetJointPropertyTree())
	{
		ItemTags.Add(MakeShareable(new FJointTreeItemTag_Graph(GetJointPropertyTree()->Filter)));
	}
}

FString FJointTreeItem_Graph::GetReferencerName() const
{
	return TEXT("FJointTreeItem_Graph");
}

#include "Misc/EngineVersionComparison.h"

void FJointTreeItem_Graph::AddReferencedObjects(FReferenceCollector& Collector)
{
}


#undef LOCTEXT_NAMESPACE
