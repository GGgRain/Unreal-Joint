//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointBuildPresetActions.h"
#include "SharedType/JointBuildPreset.h"


FJointBuildPresetActions::FJointBuildPresetActions(EAssetTypeCategories::Type InAssetCategory) :
	Category(InAssetCategory)
{
}

bool FJointBuildPresetActions::CanFilter()
{
	return true;
}


void FJointBuildPresetActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto Assets = GetTypedWeakObjectPtrs<UJointBuildPreset>(InObjects);
}


uint32 FJointBuildPresetActions::GetCategories()
{
	return Category;
}


FText FJointBuildPresetActions::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_JointBuildPreset", "Joint Build Preset");
}


UClass* FJointBuildPresetActions::GetSupportedClass() const
{
	return UJointBuildPreset::StaticClass();
}


FColor FJointBuildPresetActions::GetTypeColor() const
{
	return FColor::Red;
}


bool FJointBuildPresetActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

