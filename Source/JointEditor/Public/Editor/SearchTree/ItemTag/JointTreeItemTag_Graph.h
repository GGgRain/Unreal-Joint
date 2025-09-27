//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointTreeItemTag.h"

class FJointTreeFilter;

class JOINTEDITOR_API FJointTreeItemTag_Graph : public FJointTreeItemTag
{
public:
	
	FJointTreeItemTag_Graph(const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter);

public:

	virtual TSharedRef<SWidget> MakeTagWidget() override;
	virtual FText GetFilterText() override;

public:

	FReply AddFilterText();

public:

	TWeakPtr<FJointTreeFilter> OwnerJointTreeFilter;
};
