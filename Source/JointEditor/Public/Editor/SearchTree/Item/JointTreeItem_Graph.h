//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointTreeItem.h"
#include "Node/JointNodeBase.h"


class UJointEdGraph;

class JOINTEDITOR_API FJointTreeItem_Graph : public FJointTreeItem, public FGCObject
{
public:
	
	Joint_PROPERTY_TREE_ITEM_TYPE(FJointTreeItem_Graph, FJointTreeItem)

	FJointTreeItem_Graph(TWeakObjectPtr<UJointEdGraph> InJointEdGraph, const TSharedRef<SJointTree>& InTree);
	
public:

	virtual void GenerateWidgetForNameColumn(TSharedPtr< SHorizontalBox > Box, const TAttribute<FText>& FilterText, FIsSelected InIsSelected) override;
	virtual TSharedRef< SWidget > GenerateWidgetForDataColumn(const TAttribute<FText>& FilterText, const FName& DataColumnName, FIsSelected InIsSelected) override;

	virtual FName GetRowItemName() const override;
	virtual void OnItemDoubleClicked() override;
	virtual UObject* GetObject() const override;

	virtual TSet<TSharedPtr<IJointTreeItemTag>> GetItemTags() override;

public:

	virtual void AllocateItemTags() override;
	/** FGCObject interface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	virtual FString GetReferencerName() const override;

public:

	FReply OnMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent);

private:
	
	UPROPERTY()
	TWeakObjectPtr<UJointEdGraph> GraphPtr;

	TSet<TSharedPtr<IJointTreeItemTag>> ItemTags;

};
