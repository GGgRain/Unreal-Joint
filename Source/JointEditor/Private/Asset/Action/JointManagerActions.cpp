//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointManagerActions.h"
#include "JointManager.h"
#include "JointEditorToolkit.h"


FJointManagerActions::FJointManagerActions(EAssetTypeCategories::Type InAssetCategory) :
	Category(InAssetCategory)
{
}

bool FJointManagerActions::CanFilter()
{
	return true;
}


void FJointManagerActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto Assets = GetTypedWeakObjectPtrs<UJointManager>(InObjects);
}


uint32 FJointManagerActions::GetCategories()
{
	return Category;
}


FText FJointManagerActions::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_JointManager", "Joint Manager");
}


UClass* FJointManagerActions::GetSupportedClass() const
{
	return UJointManager::StaticClass();
}


FColor FJointManagerActions::GetTypeColor() const
{
	return FColor::Turquoise;
}


bool FJointManagerActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}


#include "JointEditor.h"

void FJointManagerActions::StoreEditorModuleClassCache()
{
	FJointEditorModule& EditorModule = FModuleManager::GetModuleChecked<FJointEditorModule>(TEXT("JointEditor"));

	//Store the class caches here. This is due to the synchronization issue.
	EditorModule.StoreClassCaches();
}

void FJointManagerActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric: EToolkitMode::Standalone;

	//Store the editor's class caches here. This is due to the synchronization issue.
	//It's a little confusing but don't change the location where we call this function at. It's kinda traditional thing for the UE's editor modules.
	//Joint 2.3.0 : made it store the caches prior to the editor initialization.
	StoreEditorModuleClassCache();
	
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		const auto Asset = Cast<UJointManager>(*ObjIt);

		if (Asset == nullptr) continue;
		
		const TSharedRef<FJointEditorToolkit> EditorToolkit = MakeShareable(new FJointEditorToolkit());
		EditorToolkit->InitJointEditor(Mode, EditWithinLevelEditor, Asset);
	}

}
