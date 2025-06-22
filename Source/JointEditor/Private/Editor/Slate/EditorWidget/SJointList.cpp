//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorWidget/SJointList.h"

#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "JointManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"

#include "Misc/EngineVersionComparison.h"


void SJointList::Construct(const FArguments& InArgs)
{
	OnAssetSelected = InArgs._OnAssetSelected;
	OnAssetDoubleClicked = InArgs._OnAssetDoubleClicked;
	OnAssetsActivated = InArgs._OnAssetsActivated;

	SetCanTick(false);
	
	RebuildWidget();
}


#if UE_VERSION_OLDER_THAN(5,0,0)
	void GetAllBlueprintSubclasses(TArray< TAssetSubclassOf< UObject > >& Subclasses, TSubclassOf< UObject > Base, bool bAllowAbstract, FString const& Path)
#else
	void GetAllBlueprintSubclasses(TArray<TSoftClassPtr<UObject>>& Subclasses, TSubclassOf<UObject> Base, bool bAllowAbstract, FString const& Path)
#endif



{
	/*
	For blueprint classes, things are complicated by the fact that the UClass may not have been loaded into memory yet.
	The approach taken here is a bit more complicated than it has to be, but allows us to gather the list of subclasses
	without force loading anything.
	*/

	static const FName GeneratedClassTag = TEXT("GeneratedClass");
	static const FName ClassFlagsTag = TEXT("ClassFlags");

	check(Base);

	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	// The asset registry is populated asynchronously at startup, so there's no guarantee it has finished.
	// This simple approach just runs a synchronous scan on the entire content directory.
	// Better solutions would be to specify only the path to where the relevant blueprints are,
	// or to register a callback with the asset registry to be notified of when it's finished populating.
	TArray<FString> ContentPaths;
	ContentPaths.Add(TEXT("/Game"));
	AssetRegistry.ScanPathsSynchronous(ContentPaths);


	// Set up a filter and then pull asset data for all blueprints in the specified path from the asset registry.
	// Note that this works in packaged builds too. Even though the blueprint itself cannot be loaded, its asset data
	// still exists and is tied to the UBlueprint type.
	FARFilter Filter;


#if UE_VERSION_OLDER_THAN(5,1,0)
	
	FName BaseClassName = Base->GetFName();

	// Use the asset registry to get the set of all class names deriving from Base
	TSet< FName > DerivedNames;
	{
		TArray< FName > BaseNames;
		BaseNames.Add(BaseClassName);

		TSet< FName > Excluded;
		
		AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
	}

	Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	
#else

	FTopLevelAssetPath BaseClassName = FTopLevelAssetPath(Base->GetPathName());

	// Use the asset registry to get the set of all class names deriving from Base
	TSet<FTopLevelAssetPath> DerivedNames;
	{
		TArray<FTopLevelAssetPath> BaseNames;
		BaseNames.Add(BaseClassName);

		TSet<FTopLevelAssetPath> Excluded;

		AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
	}

	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	
#endif


	Filter.bRecursiveClasses = true;
	if (!Path.IsEmpty())
	{
		Filter.PackagePaths.Add(*Path);
	}
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	// Iterate over retrieved blueprint assets
	for (auto const& Asset : AssetList)
	{
		// Get the class this blueprint generates (this is stored as a full path)
		auto GeneratedClassPathPtr = Asset.TagsAndValues.FindTag(GeneratedClassTag);

		if (!GeneratedClassPathPtr.GetValue().IsEmpty())
		{
			// Optionally ignore abstract classes
			// As of 4.12 I do not believe blueprints can be marked as abstract, but this may change so included for completeness.
			if (!bAllowAbstract)
			{
				auto ClassFlagsPtr = Asset.TagsAndValues.FindTag(ClassFlagsTag);

				if (!ClassFlagsPtr.GetValue().IsEmpty())
				{
					auto ClassFlags = FCString::Atoi(*ClassFlagsPtr.AsString());
					if ((ClassFlags & CLASS_Abstract) != 0)
					{
						continue;
					}
				}
			}

#if UE_VERSION_OLDER_THAN(5,1,0)

			// Convert path to just the name part
			const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassPathPtr.AsString());
			const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

			// Check if this class is in the derived set
			if(!DerivedNames.Contains(*ClassName))
			{
				continue;
			}
			
#else

			const FTopLevelAssetPath ClassObjectPath(FPackageName::ExportTextPathToObjectPath(*GeneratedClassPathPtr.GetValue()));

			// Check if this class is in the derived set
			if (!DerivedNames.Contains(ClassObjectPath))
			{
				continue;
			}
#endif


#if UE_VERSION_OLDER_THAN(5,0,0)
			Subclasses.Add(TAssetSubclassOf< UObject >(FStringAssetReference(ClassObjectPath)));
#else
			Subclasses.Add(TSoftClassPtr<UObject>(FSoftObjectPath(ClassObjectPath)));
#endif
		}
	}
}

FString TrimLastDotSection(FString InString)
{
	FString Output = "";

	InString.Split(".", &Output, nullptr);

	return Output;
}

void SJointList::RebuildWidget()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
		TEXT("ContentBrowser"));

	// Configure filter for asset picker
	FAssetPickerConfig Config;

	Config.Filter.bRecursiveClasses = true;
	Config.Filter.bRecursivePaths = true;

#if UE_VERSION_OLDER_THAN(5,1,0)

	Config.Filter.ClassNames.Add(UJointManager::StaticClass()->GetFName());

#else

	Config.Filter.ClassPaths.Add(UJointManager::StaticClass()->GetClassPathName());

#endif
	
	Config.bAddFilterUI = false;
	Config.bCanShowFolders = false;
	Config.bShowTypeInColumnView = false;
	Config.bShowPathInColumnView = true;
	Config.bShowBottomToolbar = true;
	Config.InitialAssetViewType = EAssetViewType::Column;
	
	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);

	Config.OnAssetSelected = OnAssetSelected;
	Config.OnAssetDoubleClicked = OnAssetDoubleClicked;
	Config.OnAssetsActivated = OnAssetsActivated;

	Config.SelectionMode = ESelectionMode::Multi;

	//Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &SJointList::OnGetAssetContextMenu);
	Config.OnAssetTagWantsToBeDisplayed = FOnShouldDisplayAssetTag::CreateSP(
		this, &SJointList::CanShowColumnForAssetRegistryTag);
	Config.bFocusSearchBoxWhenOpened = false;
	

	this->ChildSlot
	[
		ContentBrowserModule.Get().CreateAssetPicker(Config)
	];
}

bool SJointList::CanShowColumnForAssetRegistryTag(FName AssetType, FName TagName) const
{
	return !AssetRegistryTagsToIgnore.Contains(TagName);
}

TArray<FAssetData> SJointList::GetCurrentSelection()
{
	return GetCurrentSelectionDelegate.Execute();
}
