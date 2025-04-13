//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointTreeItemTag.h"


class FJointTreeFilter;

class JOINTEDITOR_API FJointTreeItemTag_ManagerFragment : public FJointTreeItemTag
{
public:
	
	FJointTreeItemTag_ManagerFragment(const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter);

public:

	FReply AddFilterText();

public:

	virtual TSharedRef<SWidget> MakeTagWidget() override;
	virtual FText GetFilterText() override;
	
public:

	TWeakPtr<FJointTreeFilter> OwnerJointTreeFilter;
};
