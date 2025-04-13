//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointTreeItemTag.h"

class FJointTreeFilter;

class JOINTEDITOR_API FJointTreeItemTag_Node : public FJointTreeItemTag
{
public:
	
	FJointTreeItemTag_Node(const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter);

	virtual TSharedRef<SWidget> MakeTagWidget() override;
	virtual FText GetFilterText() override;

public:

	FReply AddFilterText();

public:

	TWeakPtr<FJointTreeFilter> OwnerJointTreeFilter;
};
