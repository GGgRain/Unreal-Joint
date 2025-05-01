//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "IAssetTypeActions.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/SCompoundWidget.h"


class UJointManager;
class SJointTree;

class JOINTEDITOR_API SJointBulkSearchReplace : public SCompoundWidget
{

	
public:

	SLATE_BEGIN_ARGS(SJointBulkSearchReplace) {}
		SLATE_ARGUMENT(TSharedPtr<class FTabManager>, TabManager)
	SLATE_END_ARGS();


	void Construct(const FArguments& InArgs);

public:

	void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager);

public:

	TSharedRef<SDockTab> SpawnJointListTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnJointTreeTab(const FSpawnTabArgs& Args);

	void InitializeJointList();
	void InitializeJointTree();
	
public:

	void FillWindowMenu(FMenuBuilder& MenuBuilder);
	
	TSharedRef<SWidget> MakeToolbar();

public:

	TSharedPtr<FTabManager> TabManager;

public:

	TSharedPtr<class SJointManagerViewer> JointTree;
	TSharedPtr<class SJointList> JointList;

public:

	TArray<UJointManager*> WatchingManagers;

public:

	//Callbacks
	void OnJointListAssetDoubleClicked(const FAssetData& AssetData);
	
	void OnJointListAssetActivated(const TArray<FAssetData>& AssetDatas, EAssetTypeActivationMethod::Type Arg);

	void OnJointListAssetSelected(const FAssetData& AssetData);
	
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;

};


