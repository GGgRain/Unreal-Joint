//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointTreeItemTag.h"


class FJointTreeFilter;

class JOINTEDITOR_API FJointTreeItemTag_Type : public FJointTreeItemTag
{
public:

	FJointTreeItemTag_Type(const FText& InType, const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter);

	FJointTreeItemTag_Type(const FSlateColor& InColor, const FSlateBrush* InIcon, const FText& InType, const TSharedPtr<FJointTreeFilter>& InOwnerJointTreeFilter);

public:

	virtual TSharedRef<SWidget> MakeTagWidget() override;
	virtual FText GetFilterText() override;
	
public:

	FReply AddFilterText();

public:

	FText Type;
	FSlateColor Color;
	const FSlateBrush* Icon;

public:

	TWeakPtr<FJointTreeFilter> OwnerJointTreeFilter;

};
