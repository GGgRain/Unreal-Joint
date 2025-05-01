//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FJointTreeFilter;
class SWrapBox;

class JOINTEDITOR_API SJointTreeFilter : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJointTreeFilter): _Filter(nullptr)
	{
	}

	SLATE_ARGUMENT(TSharedPtr<class FJointTreeFilter>, Filter)
	SLATE_END_ARGS()

public:
	
	void Construct(const FArguments& InArgs);

	void ConstructLayout();

public:

	void UpdateFilterBox();

public:

	FReply OnAddNewFilterButtonClicked() const;

public:
	
	TSharedPtr<FJointTreeFilter> Filter;

public:

	TSharedPtr<SWrapBox> FilterWrapBox;

	TSharedPtr<SWidget> FilterAddButton;

public:

	TSharedPtr<SWidget> PopulateAddButtonIfNeeded();

};
