//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointScriptParserActions.h"

#include "JointScriptParser.h"

FJointScriptParserActions::FJointScriptParserActions(EAssetTypeCategories::Type InAssetCategory) :
	Category(InAssetCategory)
{
}

bool FJointScriptParserActions::CanFilter()
{
	return true;
}


void FJointScriptParserActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto Assets = GetTypedWeakObjectPtrs<UJointScriptParser>(InObjects);
}


uint32 FJointScriptParserActions::GetCategories()
{
	return Category;
}


FText FJointScriptParserActions::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_JointScriptParser", "Joint Script Parser");
}


UClass* FJointScriptParserActions::GetSupportedClass() const
{
	return UJointScriptParser::StaticClass();
}


FColor FJointScriptParserActions::GetTypeColor() const
{
	return FColor::Magenta;
}


bool FJointScriptParserActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

