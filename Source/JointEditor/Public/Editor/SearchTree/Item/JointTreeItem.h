//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IJointTreeItem.h"
#include "SearchTree/Slate/SJointTree.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"


class UTexture2D;

//////////////////////////////////////////////////////////////////////////
// SJointList


class JOINTEDITOR_API FJointTreeItem : public IJointTreeItem
{
public:
	Joint_PROPERTY_TREE_ITEM_TYPE(FJointTreeItem, IJointTreeItem)

	FJointTreeItem(const TSharedRef<class SJointTree>& JointPropertyTreePtr);

	/** ISkeletonTreeItem interface */
	virtual TSharedRef<ITableRow> MakeTreeRowWidget(const TSharedRef<STableViewBase>& InOwnerTable,
	                                                const TAttribute<FText>& InFilterText) override;

	virtual TSharedRef<SWidget> GenerateWidgetForDataColumn(const TAttribute<FText>& InFilterText,
	                                                        const FName& DataColumnName,
	                                                        FIsSelected InIsSelected) override;

	virtual bool HasInlineEditor() const override;

	virtual void ToggleInlineEditorExpansion() override;

	virtual bool IsInlineEditorExpanded() const override;
	virtual FName GetAttachName() const override;
	virtual bool CanRenameItem() const override;

	virtual void RequestRename() override;

	virtual void OnItemDoubleClicked() override;

	virtual void HandleDragEnter(const FDragDropEvent& DragDropEvent) override;

	virtual void HandleDragLeave(const FDragDropEvent& DragDropEvent) override;

	virtual FReply HandleDrop(const FDragDropEvent& DragDropEvent) override;
	virtual TSharedPtr<IJointTreeItem> GetParent() const override;
	virtual void SetParent(TSharedPtr<IJointTreeItem> InParent) override;
	virtual TArray<TSharedPtr<IJointTreeItem>>& GetChildren() override;
	virtual TArray<TSharedPtr<IJointTreeItem>>& GetFilteredChildren() override;

	virtual TSharedPtr<SJointTree> GetJointPropertyTree() const override;

	//virtual UJointManager* GetJointManager() const override { return JointPropertyTreePtr.Pin()->Manager;}
	virtual EJointTreeFilterResult GetFilterResult() const override;
	virtual void SetFilterResult(EJointTreeFilterResult InResult) override;

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	virtual UObject* GetObject() const override;
	virtual bool IsInitiallyExpanded() const override;

	virtual void AllocateItemTags() override;

	virtual TSet<TSharedPtr<class IJointTreeItemTag>> GetItemTags() override;

	virtual const FString GetFilterString() override;


public:
	virtual TSharedRef<SWidget> MakeItemTagContainerWidget();

protected:
	/** The parent of this item */
	TWeakPtr<IJointTreeItem> Parent;

	/** The children of this item */
	TArray<TSharedPtr<IJointTreeItem>> Children;

	/** The filtered children of this item */
	TArray<TSharedPtr<IJointTreeItem>> FilteredChildren;

	/** The owning skeleton tree */
	TWeakPtr<class SJointTree> JointPropertyTreePtr;

	/** The current filter result */
	EJointTreeFilterResult FilterResult;
};
