//Copyright 2022~2024 DevGrain. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"

enum class ETextFilterExpressionType : uint8;
class FJointTreeFilter;
class SWidget;

class JOINTEDITOR_API FJointTreeFilterItem : public TSharedFromThis<FJointTreeFilterItem>
{
public:
	
	FJointTreeFilterItem();

	FJointTreeFilterItem(const FString& InFilterString);

private:
	
	FString FilterString;

	FString FilterOperator;
	
	bool bIsEnabled = true;

private:
	
	TSharedPtr<SWidget> Widget;

	TWeakPtr<FJointTreeFilter> OwnerTreeFilter;

public:
	
	TSharedPtr<SWidget> GetFilterWidget();

	void MakeFilterWidget();

public:
	FReply OnItemEnableButtonClicked();

	FReply OnItemRemoveButtonClicked();

	void OnItemFilterTextCommitted(const FText& Text, ETextCommit::Type Arg);

	void OnItemFilterTextChanged(const FText& Text);

public:

	void NotifyItemDataChanged();

public:

	void SetOwnerTreeFilter(const TWeakPtr<FJointTreeFilter>& InOwnerTreeFilter);

	const TWeakPtr<FJointTreeFilter>& GetOwnerTreeFilter();
	
public:

	void SetIsEnabled(const bool NewIsEnabled);

	bool GetIsEnabled() const;

	bool IsEqual(const TSharedPtr<FJointTreeFilterItem>& Other) const;

public:

	const FString ExtractFilterItem();

	FText ExtractFilterItemText();


public:
	DECLARE_MULTICAST_DELEGATE(FOnJointFilterItemDataChanged)

	FOnJointFilterItemDataChanged OnJointFilterItemDataChanged;
};
