//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Slate/SJointManagement.h"

#include "JointAdvancedWidgets.h"
#include "JointManagementTabs.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/ISlateStyle.h"
#include "SlateOptMacros.h"
#include "Framework/Docking/LayoutService.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"

#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"

#include "Widgets/Layout/SScrollBox.h"

/**
 * @brief Look for the "SNiagaraDebugger.cpp" for the sample codes for the layout in future update.
 */
#define LOCTEXT_NAMESPACE "JointManagement"

//Tab Identification and spawn related namespaces

void SJointManagement::Construct(const FArguments& InArgs)
{
	TabManager = InArgs._TabManager;
	JointManagementTabHandler = InArgs._JointManagementTabHandler;
	
	SetCanTick(false);

	TSharedRef<FTabManager::FStack> Stack = FTabManager::NewStack()->SetSizeCoefficient(0.8f)->SetHideTabWell(false);

	TArray<TSharedPtr<IJointManagementSubTab>> SubTabs = JointManagementTabHandler.Pin()->GetSubTabs();

	bool bAddedTab = false;

	for (TSharedPtr<IJointManagementSubTab> JointManagementSubTab : SubTabs)
	{
		if(!JointManagementSubTab.IsValid()) continue;

		JointManagementSubTab->RegisterTabSpawner(TabManager);

		Stack->AddTab(JointManagementSubTab->GetTabId(),JointManagementSubTab->GetInitialTabState());


		//Set the first tab to be foreground tab.
		if(!bAddedTab)
		{
			Stack->SetForegroundTab(JointManagementSubTab->GetTabId());
			bAddedTab = true;
		}
	}
	
	TSharedPtr<FTabManager::FLayout> DebuggerLayout = FTabManager::NewLayout("JointManagementLayout_V1.1.1")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.3f)
				->Split(
					Stack
				)
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
		, FNewMenuDelegate::CreateSP(this, &SJointManagement::FillWindowMenu)
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


void SJointManagement::FillWindowMenu(FMenuBuilder& MenuBuilder)
{
	if (!TabManager.IsValid()) { return; }

	TabManager->PopulateLocalTabSpawnerMenu(MenuBuilder);
}


TSharedRef<SWidget> SJointManagement::MakeToolbar()
{
	FToolBarBuilder ToolbarBuilder(MakeShareable(new FUICommandList), FMultiBoxCustomization::None);
	
	ToolbarBuilder.BeginSection("Main");

	ToolbarBuilder.EndSection();

	return ToolbarBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
