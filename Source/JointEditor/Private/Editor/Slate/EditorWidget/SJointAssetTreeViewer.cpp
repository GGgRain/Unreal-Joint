//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorWidget/SJointAssetTreeViewer.h"
#include "JointManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "SearchTree/Slate/SJointManagerViewer.h"

#include "Misc/EngineVersionComparison.h"

//////////////////////////////////////////////////////////////////////////
// SJointList


void SJointAssetTreeViewer::Construct(const FArguments& InArgs)
{

	SetCanTick(false);
	
	RebuildWidget();

	FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SJointAssetTreeViewer::PropertyChanged);

}

SJointAssetTreeViewer::~SJointAssetTreeViewer()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}


void SJointAssetTreeViewer::GatherJointManagers()
{

	Managers.Empty();

	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	// The asset registry is populated asynchronously at startup, so t

	FARFilter Filter;
	
#if UE_VERSION_OLDER_THAN(5,1,0)
	Filter.ClassNames.Add(UJointManager::StaticClass()->GetFName());
#else
	Filter.ClassPaths.Add(UJointManager::StaticClass()->GetClassPathName());
#endif
	
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	for (FAssetData List : AssetList)
	{
		Managers.Add(Cast<UJointManager>(List.GetAsset()));
	}
}

void SJointAssetTreeViewer::RebuildWidget()
{
	ChildSlot.DetachWidget();
	
	GatherJointManagers();
	
	ChildSlot
	[
		SAssignNew(ManagerTree,SJointManagerViewer)
		.JointManagers(Managers)
	];
}

void SJointAssetTreeViewer::RequestTreeRebuild()
{
	ManagerTree->RequestTreeRebuild();
}

void SJointAssetTreeViewer::PropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{

	if(Managers.Contains(Object) || Managers.Contains(Object->GetOutermostObject()))
	{
		RequestTreeRebuild();
	}
}
