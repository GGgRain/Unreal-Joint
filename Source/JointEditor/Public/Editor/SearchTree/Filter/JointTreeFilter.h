//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SJointTree;
class FJointTreeFilterItem;

class JOINTEDITOR_API FJointTreeFilter : public TSharedFromThis<FJointTreeFilter>
{

public:

	FJointTreeFilter(const TSharedPtr<SJointTree>& InJointTree);

	virtual ~FJointTreeFilter()
	{
	};

public:
	
	virtual void AddItem(const TSharedPtr<FJointTreeFilterItem>& Item);
	virtual void RemoveItem(const TSharedPtr<FJointTreeFilterItem>& Item);
	virtual TSharedPtr<FJointTreeFilterItem> FindEqualItem(const TSharedPtr<FJointTreeFilterItem>& Item);
	virtual TSharedPtr<FJointTreeFilterItem> FindOrAddItem(const TSharedPtr<FJointTreeFilterItem>& Item);
	virtual TSet<TSharedPtr<FJointTreeFilterItem>> GetItems();
	virtual void ClearItems();


	virtual const FText ExtractFilterItems();

	void OnItemUpdated();

private:
	
	TSet<TSharedPtr<FJointTreeFilterItem>> Items;

public:
	
	TSharedPtr<SJointTree> JointTree;

public:
	
	DECLARE_MULTICAST_DELEGATE(FOnJointFilterChanged)

	FOnJointFilterChanged OnJointFilterChanged;

public:

	static const FText ReplaceInqueryableCharacters(const FText& InText);

	static const FString ReplaceInqueryableCharacters(const FString& InString);

	
};


