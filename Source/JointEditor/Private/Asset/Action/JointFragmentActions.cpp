//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointFragmentActions.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Node/JointFragment.h"

FJointFragmentActions::FJointFragmentActions(EAssetTypeCategories::Type InAssetCategory) :
	Category(InAssetCategory)
{
}



bool FJointFragmentActions::CanFilter()
{
	return false;
}

void FJointFragmentActions::BuildBackendFilter(FARFilter& InFilter)
{
	InFilter.bRecursiveClasses = true;
	InFilter.bRecursivePaths = true;

	FAssetTypeActions_Base::BuildBackendFilter(InFilter);
	/*
	TArray<FString> Paths;

	FName BaseClassName = UJointFragment::StaticClass()->GetFName();
	
	static const FName GeneratedClassTag = TEXT("GeneratedClass");
	static const FName ClassFlagsTag = TEXT("ClassFlags");
	
	FARFilter Filter;
	
	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked< FAssetRegistryModule >(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	// The asset registry is populated asynchronously at startup, so there's no guarantee it has finished.
	// This simple approach just runs a synchronous scan on the entire content directory.
	// Better solutions would be to specify only the path to where the relevant blueprints are,
	// or to register a callback with the asset registry to be notified of when it's finished populating.
	TArray< FString > ContentPaths;
	ContentPaths.Add(TEXT("/Game"));
	AssetRegistry.ScanPathsSynchronous(ContentPaths);

	// Use the asset registry to get the set of all class names deriving from Base
	TSet< FName > DerivedNames;
	{
		TArray< FName > BaseNames;
		BaseNames.Add(BaseClassName);

		TSet< FName > Excluded;
		
		AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
	}

	Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray< FAssetData > AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);
	
	// Iterate over retrieved blueprint assets
	for(auto const& Asset : AssetList)
	{
		// Get the the class this blueprint generates (this is stored as a full path)
		auto GeneratedClassPathPtr = Asset.TagsAndValues.FindTag(GeneratedClassTag);
		
		if(!GeneratedClassPathPtr.GetValue().IsEmpty())
		{
			// Convert path to just the name part
			const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassPathPtr.AsString());
			const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

			// Check if this class is in the derived set
			if(!DerivedNames.Contains(*ClassName))
			{
				continue;
			}
			
			Paths.Add(GeneratedClassPathPtr.AsString());
		}
	}

	InFilter.ObjectPaths.Append(Paths);
	*/
}

FName FJointFragmentActions::GetFilterName() const
{
	//return FName("ParentClass==JointFragment");

	return FName(FAssetTypeActions_Base::GetFilterName().ToString());
}


void FJointFragmentActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto Assets = GetTypedWeakObjectPtrs<UJointFragment>(InObjects);
}


uint32 FJointFragmentActions::GetCategories()
{
	return Category;
}


FText FJointFragmentActions::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_JointFragment", "Joint Fragment");
}


UClass* FJointFragmentActions::GetSupportedClass() const
{
	return UJointFragment::StaticClass();
}


FColor FJointFragmentActions::GetTypeColor() const
{
	return FColor(71, 212 , 222);
}

bool FJointFragmentActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}
