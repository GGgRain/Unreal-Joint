//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/SlateDelegates.h"

class SJointTree;

class JOINTEDITOR_API IJointTreeItemTag : public TSharedFromThis<IJointTreeItemTag>
{
public:

	virtual ~IJointTreeItemTag() {};
	
	IJointTreeItemTag() {};
	
	/** Builds the table row widget to display this info */
	virtual TSharedRef<SWidget> MakeTagWidget() = 0;
	
	/**
	 * Allocate tags for the item. This is useful when you have to display custom data for the item.
	 */
	virtual FText GetFilterText() = 0;
	
	/**
	 * Create the filter item that can be used to filter the same type of tag on the filter.
	 * @return Created filter item.
	 */
	virtual TSharedPtr<class IJointTreeFilterItem> GetFilterItem() = 0;

};
