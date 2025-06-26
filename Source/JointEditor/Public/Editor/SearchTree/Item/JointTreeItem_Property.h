//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointTreeItem.h"


class JOINTEDITOR_API FJointTreeItem_Property : public FJointTreeItem, public FGCObject
{
public:
	Joint_PROPERTY_TREE_ITEM_TYPE(FJointTreeItem_Property, FJointTreeItem)

	FJointTreeItem_Property(FProperty* InProperty, TWeakObjectPtr<UObject> InObject, const TSharedRef<class SJointTree>& InTree);

public:
	virtual void GenerateWidgetForNameColumn(TSharedPtr<SHorizontalBox> Box, const TAttribute<FText>& FilterText,
	                                         FIsSelected InIsSelected) override;
	virtual TSharedRef<SWidget> GenerateWidgetForDataColumn(const TAttribute<FText>& FilterText,
	                                                        const FName& DataColumnName,
	                                                        FIsSelected InIsSelected) override;
	//virtual TSharedRef< SWidget > GenerateInlineEditWidget(const TAttribute<FText>& FilterText, FIsSelected InIsSelected) override;
	virtual FName GetRowItemName() const override;
	virtual UObject* GetObject() const override;

	virtual void AllocateItemTags() override;
	virtual TSet<TSharedPtr<IJointTreeItemTag>> GetItemTags() override;

public:
	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

public:
	void OnTextCommitted(const FText& Text, ETextCommit::Type Arg);
	FReply OnMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent);
	virtual void OnItemDoubleClicked() override;

public:
	void CacheAdditionalRowSearchString();
	
	virtual const FString GetFilterString() override;

public:
	/**
	 * The property this item indicate.
	 */
	FProperty* Property = nullptr;

	/**
	 * The outer object of this item's property.
	 */
	TWeakObjectPtr<UObject> PropertyOuter = nullptr;

public:
	/**
	 * An additional string that will be included for this item for the search and filter.
	 */
	FString AdditionalRowSearchString;

public:
	TSet<TSharedPtr<IJointTreeItemTag>> ItemTags;
};
