//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "SearchTree/Filter/JointTreeFilter.h"

#include "Filter/JointTreeFilterItem.h"

FJointTreeFilter::FJointTreeFilter(const TSharedPtr<SJointTree>& InJointTree) : JointTree(InJointTree)
{
}

void FJointTreeFilter::AddItem(const TSharedPtr<FJointTreeFilterItem>& Item)
{
	if (const TSharedPtr<FJointTreeFilterItem>& FoundItem = FindEqualItem(Item); !FoundItem)
	{
		Items.Add(Item);

		Item->SetOwnerTreeFilter(this->AsShared());

		Item->OnJointFilterItemDataChanged.AddSP(this, &FJointTreeFilter::OnItemUpdated);

		OnItemUpdated();
	}
}

void FJointTreeFilter::RemoveItem(const TSharedPtr<FJointTreeFilterItem>& Item)
{
	if (const TSharedPtr<FJointTreeFilterItem>& FoundItem = FindEqualItem(Item))
	{
		Items.Remove(FoundItem);

		Item->SetOwnerTreeFilter(nullptr);

		FoundItem->OnJointFilterItemDataChanged.RemoveAll(this);

		OnItemUpdated();
	}
}

TSet<TSharedPtr<FJointTreeFilterItem>> FJointTreeFilter::GetItems()
{
	return Items;
}

void FJointTreeFilter::ClearItems()
{
	for (TSharedPtr<FJointTreeFilterItem>
	     JointTreeFilterItem : Items)
	{
		JointTreeFilterItem->SetOwnerTreeFilter(nullptr);

		JointTreeFilterItem->OnJointFilterItemDataChanged.RemoveAll(this);
	}

	Items.Empty();

	OnItemUpdated();
}

TSharedPtr<FJointTreeFilterItem> FJointTreeFilter::FindEqualItem(
	const TSharedPtr<FJointTreeFilterItem>& Item)
{
	for (TSharedPtr<FJointTreeFilterItem> JointTreeFilterItem : Items)
	{
		if (JointTreeFilterItem.IsValid() && JointTreeFilterItem->IsEqual(Item)) return JointTreeFilterItem;
	}

	return nullptr;
}

TSharedPtr<FJointTreeFilterItem> FJointTreeFilter::FindOrAddItem(const TSharedPtr<FJointTreeFilterItem>& Item)
{
	if (const TSharedPtr<FJointTreeFilterItem>& FoundItem = FindEqualItem(Item)) return FoundItem;
	
	AddItem(Item);

	return nullptr;
}

const FText FJointTreeFilter::ExtractFilterItems()
{
	FString FilterString;
	FString FilterContentString;

	if (Items.IsEmpty()) return FText::GetEmpty();

	for (TSharedPtr<FJointTreeFilterItem>& JointTreeFilterItem : Items)
	{
		if (!JointTreeFilterItem->GetIsEnabled()) continue;

		if (!FilterContentString.IsEmpty()) FilterContentString += " || ";
		FilterContentString += JointTreeFilterItem->ExtractFilterItem();
	}

	if (!FilterContentString.IsEmpty()) FilterString = "(" + FilterContentString + ")";

	return FText::FromString(FilterString);
}

void FJointTreeFilter::OnItemUpdated()
{
	if (OnJointFilterChanged.IsBound()) OnJointFilterChanged.Broadcast();
}

const FText FJointTreeFilter::ReplaceInqueryableCharacters(const FText& InText)
{
	return FText::FromString(InText.ToString().Replace(TEXT("!"), TEXT("$")));
}

const FString FJointTreeFilter::ReplaceInqueryableCharacters(const FString& InString)
{
	return InString.Replace(TEXT("!"), TEXT("$"));
}
