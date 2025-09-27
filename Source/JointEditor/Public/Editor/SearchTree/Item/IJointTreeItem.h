//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/SlateDelegates.h"

class SJointTree;
class IJointTreeItemTag;
class SHorizontalBox;
class STableViewBase;
class ITableRow;
class UJointManager;

enum class JOINTEDITOR_API EJointTreeFilterResult : int32
{
	/** Hide the item */
	Hidden = 0,

	/** Show the item because child items were shown */
	ShownDescendant = 1,

	/** Show the item */
	Shown = 2,

	/** Show the item and highlight search text */
	ShownHighlighted = 3,
};

#define Joint_PROPERTY_TREE_BASE_ITEM_TYPE(TYPE) \
public:\
static const FName& GetTypeId() { static FName Type(TEXT(#TYPE)); return Type; } \
virtual bool IsOfTypeByName(const FName& Type) const { return TYPE::GetTypeId() == Type; } \
virtual FName GetTypeName() const { return TYPE::GetTypeId(); }


class JOINTEDITOR_API IJointTreeItem : public TSharedFromThis<IJointTreeItem>
{
public:
	Joint_PROPERTY_TREE_BASE_ITEM_TYPE(IJointTreeItem)

	/** Check if this item can cast safely to the specified template type */
	template <typename TType>
	bool IsOfType() const
	{
		return IsOfTypeByName(TType::GetTypeId());
	}

	virtual ~IJointTreeItem()
	{
	};

	/** Builds the table row widget to display this info */
	virtual TSharedRef<ITableRow> MakeTreeRowWidget(const TSharedRef<STableViewBase>& InOwnerTable,
	                                                const TAttribute<FText>& InFilterText) = 0;

	/** Builds the slate widget for the name column */
	virtual void GenerateWidgetForNameColumn(TSharedPtr<SHorizontalBox> Box, const TAttribute<FText>& FilterText,
	                                         FIsSelected InIsSelected) = 0;

	/** Builds the slate widget for the data column */
	virtual TSharedRef<SWidget> GenerateWidgetForDataColumn(const TAttribute<FText>& FilterText,
	                                                        const FName& DataColumnName, FIsSelected InIsSelected) = 0;

public:
	//Inline Editor

	/** Builds the slate widget for any inline data editing */
	//virtual TSharedRef< SWidget > GenerateInlineEditWidget(const TAttribute<FText>& FilterText, FIsSelected InIsSelected) = 0;

	/** @return true if the item has an in-line editor widget */
	virtual bool HasInlineEditor() const = 0;

	/** Toggle the expansion state of the inline editor */
	virtual void ToggleInlineEditorExpansion() = 0;

	/** Get the expansion state of the inline editor */
	virtual bool IsInlineEditorExpanded() const = 0;

public:
	/** Get whether this item begins expanded or not */
	virtual bool IsInitiallyExpanded() const = 0;

public:
	//Row Rename (for some of the default inline editing)

	/** Get the name of the item that this row represents */
	virtual FName GetRowItemName() const = 0;

	/** Return the name used to attach to this item */
	virtual FName GetAttachName() const = 0;

	/** @return true if this item can be renamed */
	virtual bool CanRenameItem() const = 0;

	/** Requests a rename on the the tree row item */
	virtual void RequestRename() = 0;

public:
	//Widget Action

	/** Handler for when the user double clicks on this item in the tree */
	virtual void OnItemDoubleClicked() = 0;

	/** Handle a drag and drop enter event */
	virtual void HandleDragEnter(const FDragDropEvent& DragDropEvent) = 0;

	/** Handle a drag and drop leave event */
	virtual void HandleDragLeave(const FDragDropEvent& DragDropEvent) = 0;

	/** Handle a drag and drop drop event */
	virtual FReply HandleDrop(const FDragDropEvent& DragDropEvent) = 0;

public:
	/** Get this item's parent  */
	virtual TSharedPtr<IJointTreeItem> GetParent() const = 0;

	/** Set this item's parent */
	virtual void SetParent(TSharedPtr<IJointTreeItem> InParent) = 0;

	/** The array of children for this item */
	virtual TArray<TSharedPtr<IJointTreeItem>>& GetChildren() = 0;

	/** The filtered array of children for this item */
	virtual TArray<TSharedPtr<IJointTreeItem>>& GetFilteredChildren() = 0;

public:
	//Object Reference


	/** The owning skeleton tree */
	virtual TSharedPtr<SJointTree> GetJointPropertyTree() const = 0;

	/** Get the editable skeleton the tree represents */
	//virtual UJointManager* GetJointManager() const = 0;

	/** Get the object represented by this item, if any */
	virtual UObject* GetObject() const = 0;

public:
	//Filter Action

	/** Get the current filter result */
	virtual EJointTreeFilterResult GetFilterResult() const = 0;

	/** Set the current filter result */
	virtual void SetFilterResult(EJointTreeFilterResult InResult) = 0;

public:

	virtual const FString GetFilterString() = 0;

public:
	/**
	 * Allocate tags for the item. This is useful when you have to display custom data for the item.
	 */
	virtual void AllocateItemTags() = 0;

	virtual TSet<TSharedPtr<class IJointTreeItemTag>> GetItemTags() = 0;
};


#define Joint_PROPERTY_TREE_ITEM_TYPE(TYPE, BASE) \
public:\
static const FName& GetTypeId() { static FName Type(TEXT(#TYPE)); return Type; } \
virtual bool IsOfTypeByName(const FName& Type) const override { return GetTypeId() == Type || BASE::IsOfTypeByName(Type); } \
virtual FName GetTypeName() const { return TYPE::GetTypeId(); }
