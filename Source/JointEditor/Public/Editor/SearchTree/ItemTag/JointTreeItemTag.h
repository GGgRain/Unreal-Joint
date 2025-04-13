//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IJointTreeItemTag.h"


class JOINTEDITOR_API FJointTreeItemTag : public IJointTreeItemTag
{
public:
	
	FJointTreeItemTag();

	virtual TSharedRef<SWidget> MakeTagWidget() override;
	virtual FText GetFilterText() override;
	virtual TSharedPtr<class IJointTreeFilterItem> GetFilterItem() override;

	
};
