//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorTools/SJointBulkSearchReplace.h"
#include "JointEditorStyle.h"
#include "Builder/JointTreeBuilder.h"
#include "EditorWidget/SJointList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/ISlateStyle.h"
#include "Framework/Docking/LayoutService.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "Slate/SJointManagerViewer.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"


/**
 * @brief Look for the "SNiagaraDebugger.cpp" for the sample codes for the layout in future update.
 */
#define LOCTEXT_NAMESPACE "SJointBulkSearchReplace"

//Tab Identification and spawn related namespaces


namespace BulkSearchReplaceTapIDs
{
	static const FName JointTreeID("JointTreeID");
	static const FName JointListID("JointListID");
}


void SJointBulkSearchReplace::Construct(const FArguments& InArgs)
{
	TabManager = InArgs._TabManager;
	
	SetCanTick(false);


	InitializeJointList();
	InitializeJointTree();

	RegisterTabSpawners(TabManager.ToSharedRef());


	TSharedPtr<FTabManager::FLayout> DebuggerLayout = FTabManager::NewLayout("JointBulkSearchReplace_V1")
		->AddArea(FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.25f)
				->AddTab(BulkSearchReplaceTapIDs::JointListID, ETabState::OpenedTab)
			)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.75f)
				->AddTab(BulkSearchReplaceTapIDs::JointTreeID, ETabState::OpenedTab)
			)
		);

	DebuggerLayout = FLayoutSaveRestore::LoadFromConfig(GEditorLayoutIni, DebuggerLayout.ToSharedRef());

	TSharedRef<SWidget> TabContents = TabManager->RestoreFrom(DebuggerLayout.ToSharedRef(), TSharedPtr<SWindow>()).
		ToSharedRef();

	// create & initialize main menu
	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());

	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("WindowMenuLabel", "Window")
		, FText::GetEmpty()
		, FNewMenuDelegate::CreateSP(this, &SJointBulkSearchReplace::FillWindowMenu)
		, "Window"
	);

	// Tell tab-manager about the multi-box for platforms with a global menu bar

#if UE_VERSION_OLDER_THAN(5,0,0)
	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
#else
	TSharedRef<SWidget> MenuWidget = MenuBarBuilder.MakeWidget();
	MenuWidget->SetClipping(EWidgetClipping::ClipToBoundsWithoutIntersecting);

	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox(), MenuWidget);
#endif
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MenuBarBuilder.MakeWidget()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeToolbar()
		]
		+ SVerticalBox::Slot()
		.Padding(2.0)
		[
			TabContents
		]
	];
}

void SJointBulkSearchReplace::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	//Unregister first
	TabManager->UnregisterTabSpawner(BulkSearchReplaceTapIDs::JointListID);
	TabManager->UnregisterTabSpawner(BulkSearchReplaceTapIDs::JointTreeID);


	TabManager->RegisterTabSpawner(BulkSearchReplaceTapIDs::JointListID,
	                               FOnSpawnTab::CreateSP(this, &SJointBulkSearchReplace::SpawnJointListTab))
		.SetDisplayName(LOCTEXT("JointListTabTitle", "Joint List"))
		.SetTooltipText(LOCTEXT("JointListTabTooltipText", "Open Joint list tab."))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "ContentBrowser.ShowSourcesView"));

	TabManager->RegisterTabSpawner(BulkSearchReplaceTapIDs::JointTreeID,
	                               FOnSpawnTab::CreateSP(this, &SJointBulkSearchReplace::SpawnJointTreeTab))
		.SetDisplayName(LOCTEXT("JointTreeTabTitle", "Joint Tree"))
		.SetTooltipText(LOCTEXT("JointTreeTabTooltipText", "Open Joint tree tab."))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Details"));
}


TSharedRef<SDockTab> SJointBulkSearchReplace::SpawnJointListTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> TabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointListTabTitle", "Joint List"));

	if (JointList.IsValid())
	{
		TabPtr->SetContent(JointList.ToSharedRef());
	}

	return TabPtr;
}

TSharedRef<SDockTab> SJointBulkSearchReplace::SpawnJointTreeTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> TabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointTreeTabTitle", "Joint Tree"));

	if (JointTree.IsValid())
	{
		TabPtr->SetContent(JointTree.ToSharedRef());
	}

	return TabPtr;
}

void SJointBulkSearchReplace::InitializeJointList()
{
	JointList = SNew(SJointList)
		.OnAssetSelected(this, &SJointBulkSearchReplace::OnJointListAssetSelected)
		.OnAssetDoubleClicked(this, &SJointBulkSearchReplace::OnJointListAssetDoubleClicked)
		.OnAssetsActivated(this, &SJointBulkSearchReplace::OnJointListAssetActivated);
}

void SJointBulkSearchReplace::InitializeJointTree()
{
	JointTree = SNew(SJointManagerViewer)
		.bInitialShowOnlyCurrentGraph(false)
		.ShowOnlyCurrentGraphToggleButtonVisibility(EVisibility::Collapsed);
}


void SJointBulkSearchReplace::FillWindowMenu(FMenuBuilder& MenuBuilder)
{
	if (!TabManager.IsValid()) { return; }

#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif //!WITH_EDITOR

	TabManager->PopulateLocalTabSpawnerMenu(MenuBuilder);
}


TSharedRef<SWidget> SJointBulkSearchReplace::MakeToolbar()
{
	FToolBarBuilder ToolbarBuilder(MakeShareable(new FUICommandList), FMultiBoxCustomization::None);
	ToolbarBuilder.BeginSection("Main");

	ToolbarBuilder.EndSection();

	return ToolbarBuilder.MakeWidget();
}

void SJointBulkSearchReplace::OnJointListAssetDoubleClicked(const FAssetData& AssetData)
{
	const TArray<FAssetData>& CurrentAssetData = JointList->GetCurrentSelection();
	
	TArray<TWeakObjectPtr<UJointManager>> Managers;
	
	for (const FAssetData& InAssetData : CurrentAssetData)
	{
		if(InAssetData.GetAsset() && Cast<UJointManager>(InAssetData.GetAsset()))
		{
			Managers.Add(Cast<UJointManager>(InAssetData.GetAsset()));
		}
	}

	JointTree->SetTargetManager(Managers);
}

void SJointBulkSearchReplace::OnJointListAssetActivated(const TArray<FAssetData>& AssetDatas,
	EAssetTypeActivationMethod::Type Arg)
{

	const TArray<FAssetData>& CurrentAssetData = JointList->GetCurrentSelection();
	
	TArray<TWeakObjectPtr<UJointManager>> Managers;
	
	for (const FAssetData& InAssetData : CurrentAssetData)
	{
		if(InAssetData.GetAsset() && Cast<UJointManager>(InAssetData.GetAsset()))
		{
			Managers.Add(Cast<UJointManager>(InAssetData.GetAsset()));
		}
	}

	JointTree->SetTargetManager(Managers);
}

void SJointBulkSearchReplace::OnJointListAssetSelected(const FAssetData& AssetData)
{
	const TArray<FAssetData>& CurrentAssetData = JointList->GetCurrentSelection();
	
	TArray<TWeakObjectPtr<UJointManager>> Managers;
	
	for (const FAssetData& InAssetData : CurrentAssetData)
	{
		if(InAssetData.GetAsset() && Cast<UJointManager>(InAssetData.GetAsset()))
		{
			Managers.Add(Cast<UJointManager>(InAssetData.GetAsset()));
		}
	}

	JointTree->SetTargetManager(Managers);
	JointTree->RequestTreeRebuild();
}

#undef LOCTEXT_NAMESPACE
