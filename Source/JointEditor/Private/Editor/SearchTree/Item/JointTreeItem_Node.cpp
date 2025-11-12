//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Item/JointTreeItem_Node.h"

#include "JointAdvancedWidgets.h"
#include "JointEdGraph.h"
#include "JointEdGraphNode_Composite.h"
#include "JointEdGraphNode_Connector.h"
#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "ItemTag/JointTreeItemTag_ManagerFragment.h"
#include "ItemTag/JointTreeItemTag_Node.h"
#include "ItemTag/JointTreeItemTag_Type.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

//////////////////////////////////////////////////////////////////////////
// SJointList

#define LOCTEXT_NAMESPACE "JointPropertyList"


FJointTreeItem_Node::FJointTreeItem_Node(TWeakObjectPtr<UEdGraphNode> InNodePtr,
                                         const TSharedRef<SJointTree>& InTree)
	: FJointTreeItem(InTree)
	  , EditorNodePtr(InNodePtr)
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
		
		if (EditorNodePtr.IsValid())
		{
			return EditorNodePtr->GetNodeTitleColor();
		}
		
		return DefaultColor;
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
					.Text(this, &FJointTreeItem_Node::GetDisplayName)
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
	//it must return a path to the editor node itself.
	if (EditorNodePtr == nullptr) return NAME_None;

	if (UJointEdGraphNode* JointNode = Cast<UJointEdGraphNode>(EditorNodePtr.Get()))
	{
		if (const UJointManager* JointManager = JointNode->GetCastedNodeInstance<UJointManager>())
		{
			//root node of the manager
			return FName(JointNode->GetJointManager()->GetPathName().Replace(TEXT(" "), TEXT("_")) + ".Root");
		}
	}
	
	return FName(EditorNodePtr->GetPathName().Replace(TEXT(" "), TEXT("_")));
}

void FJointTreeItem_Node::OnItemDoubleClicked()
{
	if (GetJointPropertyTree() && EditorNodePtr.Get()) GetJointPropertyTree()->JumpToHyperlink(EditorNodePtr.Get());
}

UObject* FJointTreeItem_Node::GetObject() const
{
	return EditorNodePtr.Get();
}

void FJointTreeItem_Node::AllocateItemTags()
{
	if (GetJointPropertyTree())
	{
		if (!EditorNodePtr.Get()) return;

		if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(EditorNodePtr.Get()))
		{
			ItemTags.Add(MakeShareable( new FJointTreeItemTag_Type( LOCTEXT("CommentTypeTag","Comment"), GetJointPropertyTree()->Filter)));
		}
		else if (UJointEdGraphNode_Connector* Connector = Cast<UJointEdGraphNode_Connector>(EditorNodePtr.Get()))
		{
			ItemTags.Add(MakeShareable( new FJointTreeItemTag_Type( LOCTEXT("ConnectorTypeTag","Connector"), GetJointPropertyTree()->Filter)));	
		}
		else if (UJointEdGraphNode_Composite* Composite = Cast<UJointEdGraphNode_Composite>(EditorNodePtr.Get()))
		{
			ItemTags.Add(MakeShareable( new FJointTreeItemTag_Type( LOCTEXT("CompositeTypeTag","Composite"), GetJointPropertyTree()->Filter)));
		}
		else if (UJointEdGraphNode_Tunnel* Tunnel = Cast<UJointEdGraphNode_Tunnel>(EditorNodePtr.Get()))
		{
			ItemTags.Add(MakeShareable( new FJointTreeItemTag_Type( LOCTEXT("TunnelTypeTag","Tunnel"), GetJointPropertyTree()->Filter)));
		}
		else if (UJointEdGraphNode* JointNode = Cast<UJointEdGraphNode>(EditorNodePtr.Get()))
		{
			if (const UJointNodeBase* NodeInstance = JointNode->GetCastedNodeInstance())
			{

				ItemTags.Add(MakeShareable(new FJointTreeItemTag_Node(GetJointPropertyTree()->Filter)));
				
				if (NodeInstance->GetJointManager() && NodeInstance->GetJointManager()->ManagerFragments.Contains(NodeInstance))
				{
					ItemTags.Add(MakeShareable(new FJointTreeItemTag_ManagerFragment(GetJointPropertyTree()->Filter)));
				}

				ItemTags.Add(MakeShareable(
					new FJointTreeItemTag_Type(
						NodeInstance ? FText::FromString(NodeInstance->GetClass()->GetName()) : FText::GetEmpty()
						, GetJointPropertyTree()->Filter)));	
			}else if (const UJointManager* JointManager = JointNode->GetCastedNodeInstance<UJointManager>())
			{
				//root node of the manager
			}
		}
	}
}

TSet<TSharedPtr<IJointTreeItemTag>> FJointTreeItem_Node::GetItemTags()
{
	return ItemTags;
}

const FString FJointTreeItem_Node::GetFilterString()
{
	return "Name=" + GetRowItemName().ToString().Replace(TEXT(" "), TEXT("_")) + "DisplayName" + EditorNodePtr->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Replace(TEXT(" "), TEXT("_"));
}

FText FJointTreeItem_Node::GetDisplayName() const
{
	if (EditorNodePtr == nullptr) return LOCTEXT("NodeName_Null", "INVALID OBJECT");

	if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(EditorNodePtr))
	{
		if (UJointNodeBase* NodeInstance = CastedNode->GetCastedNodeInstance())
		{
			return FText::FromName(NodeInstance->GetFName());
		}else if (UJointManager* JointManager = CastedNode->GetCastedNodeInstance<UJointManager>())
		{
			return LOCTEXT("NodeName_RootManager", "Root Node & Manager Fragments");
			//root node of the manager
		}
		
		return CastedNode->GetNodeTitle(ENodeTitleType::FullTitle);
	}else if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(EditorNodePtr))
	{
		return CommentNode->GetNodeTitle(ENodeTitleType::FullTitle);
	}
	
	return FText::FromString(EditorNodePtr->GetName());
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
