//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointNodePresetActions.h"

#include "JointNodePreset.h"

FJointNodePresetActions::FJointNodePresetActions(EAssetTypeCategories::Type InAssetCategory) :
	Category(InAssetCategory)
{
}

bool FJointNodePresetActions::CanFilter()
{
	return true;
}


void FJointNodePresetActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto Assets = GetTypedWeakObjectPtrs<UJointNodePreset>(InObjects);
}


uint32 FJointNodePresetActions::GetCategories()
{
	return Category;
}


FText FJointNodePresetActions::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_JointNodePreset", "Joint Node Preset");
}


UClass* FJointNodePresetActions::GetSupportedClass() const
{
	return UJointNodePreset::StaticClass();
}


FColor FJointNodePresetActions::GetTypeColor() const
{
	return FColor::Orange;
}


bool FJointNodePresetActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

