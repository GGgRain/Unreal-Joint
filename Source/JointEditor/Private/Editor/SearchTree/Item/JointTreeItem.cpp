//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Item/JointTreeItem.h"

#include "JointEditorStyle.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "ItemTag/IJointTreeItemTag.h"
#include "SearchTree/Slate/SJointTreeRow.h"

//////////////////////////////////////////////////////////////////////////
// SJointList

#define LOCTEXT_NAMESPACE "JointPropertyList"


FJointTreeItem::FJointTreeItem(const TSharedRef<SJointTree>& JointPropertyTreePtr):
	JointPropertyTreePtr(JointPropertyTreePtr)
	, FilterResult(EJointTreeFilterResult::Shown)
{
}

TSharedRef<SWidget> FJointTreeItem::GenerateWidgetForDataColumn(const TAttribute<FText>& InFilterText,
                                                                   const FName& DataColumnName,
                                                                   FIsSelected InIsSelected)
{
	return SNullWidget::NullWidget;
}

bool FJointTreeItem::HasInlineEditor() const
{
	return false;
}

void FJointTreeItem::ToggleInlineEditorExpansion()
{
}

bool FJointTreeItem::IsInlineEditorExpanded() const
{
	return false;
}

FName FJointTreeItem::GetAttachName() const
{
	return GetRowItemName();
}

bool FJointTreeItem::CanRenameItem() const
{
	return false;
}

void FJointTreeItem::RequestRename()
{
}

void FJointTreeItem::OnItemDoubleClicked()
{
}

void FJointTreeItem::HandleDragEnter(const FDragDropEvent& DragDropEvent)
{
}

void FJointTreeItem::HandleDragLeave(const FDragDropEvent& DragDropEvent)
{
}

FReply FJointTreeItem::HandleDrop(const FDragDropEvent& DragDropEvent)
{
	return FReply::Unhandled();
}

TSharedPtr<IJointTreeItem> FJointTreeItem::GetParent() const
{
	return Parent.Pin();
}

void FJointTreeItem::SetParent(TSharedPtr<IJointTreeItem> InParent)
{
	Parent = InParent;
}

TArray<TSharedPtr<IJointTreeItem>>& FJointTreeItem::GetChildren()
{
	return Children;
}

TArray<TSharedPtr<IJointTreeItem>>& FJointTreeItem::GetFilteredChildren()
{
	return FilteredChildren;
}

TSharedPtr<SJointTree> FJointTreeItem::GetJointPropertyTree() const
{
	return JointPropertyTreePtr.Pin();
}

EJointTreeFilterResult FJointTreeItem::GetFilterResult() const
{
	return FilterResult;
}

void FJointTreeItem::SetFilterResult(EJointTreeFilterResult InResult)
{
	FilterResult = InResult;
	//UE_LOG(LogJointEditor, Log, TEXT("SetFilterResult: %d for %s"), (int32)InResult, *GetRowItemName().ToString());
}

FReply FJointTreeItem::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Unhandled();
}

UObject* FJointTreeItem::GetObject() const
{
	return nullptr;
}

bool FJointTreeItem::IsInitiallyExpanded() const
{
	return true;
}

void FJointTreeItem::AllocateItemTags()
{
}

TSet<TSharedPtr<IJointTreeItemTag>> FJointTreeItem::GetItemTags()
{
	return TSet<TSharedPtr<class IJointTreeItemTag>>();
}

TSharedRef<ITableRow> FJointTreeItem::MakeTreeRowWidget(const TSharedRef<STableViewBase>& InOwnerTable,
                                                           const TAttribute<FText>& InFilterText)
{
	return SNew(SJointTreeRow, InOwnerTable)
		.FilterText(InFilterText)
		.Item(SharedThis(this))
		.OnDraggingItem(this, &FJointTreeItem::OnDragDetected);
}

const FString FJointTreeItem::GetFilterString()
{
	FString FilterString;

	for (TSharedPtr<IJointTreeItemTag> JointTreeItemTag : GetItemTags())
	{
		FilterString += " ";
		FilterString += JointTreeItemTag->GetFilterText().ToString().Replace(TEXT(" "), TEXT("_"));
	}

	//for debugging
	FString RowItemName;

	if ( FilterString.IsEmpty())
	{
		RowItemName = "";
	}
	
	return "Name=" + GetRowItemName().ToString().Replace(TEXT(" "), TEXT("_")) + FilterString;
}

TSharedRef<SWidget> FJointTreeItem::MakeItemTagContainerWidget()
{
	TSharedPtr<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

	for (const TSharedPtr<IJointTreeItemTag>& JointTreeItemTag : GetItemTags())
	{
		if(!JointTreeItemTag.IsValid()) continue;
		
		HorizontalBox->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				JointTreeItemTag.Get()->MakeTagWidget()
			];
	}

	return HorizontalBox.ToSharedRef();
}


#undef LOCTEXT_NAMESPACE
