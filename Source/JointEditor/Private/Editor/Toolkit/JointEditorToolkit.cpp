//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Toolkit/JointEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"
#include "Editor.h"
#include "EditorStyleSet.h"

#include "PropertyEditorModule.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include "UObject/NameTypes.h"
#include "Widgets/Docking/SDockTab.h"

#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

#include "Graph/JointEdGraph.h"
#include "Graph/JointEdGraphSchema.h"

#include "JointManager.h"
#include "GraphEditorActions.h"
#include "EditorWidget/SJointGraphEditorImpl.h"
#include "Framework/Commands/GenericCommands.h"

#include "JointEditorToolbar.h"
#include "Toolkit/JointEditorCommands.h"


#include "AssetToolsModule.h"
#include "AssetViewUtils.h"
#include "JointActor.h"
#include "EdGraphToken.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "JointEditorStyle.h"

#include "Node/JointEdGraphNode.h"
#include "EdGraphUtilities.h"
#include "IMessageLogListing.h"
#include "MessageLogModule.h"
#include "ScopedTransaction.h"
#include "SGraphPanel.h"
#include "Debug/JointDebugger.h"

#include "Framework/Docking/TabManager.h"

#include "EditorWidget/SJointList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GraphNode/SJointGraphNodeBase.h"
#include "Misc/UObjectToken.h"
#include "SearchTree/Slate/SJointManagerViewer.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Misc/EngineVersionComparison.h"

//this is for the window os related features like copy-pasting the node object from a place to somewhere elses or storing the node object data as a text.
//and in ue5, the codes related with the hololens has been deprecicated, and it's features had been merged into the "GenericPlatform/GenericPlatformApplicationMisc.h"

#include "JointEdGraphNode_Foundation.h"
#include "JointEditorNodePickingManager.h"
#include "VoltDecl.h"
#include "EditorWidget/JointToolkitToastMessages.h"
#include "EditorWidget/SJointFragmentPalette.h"
#include "Module/Volt_ASM_Delay.h"
#include "Module/Volt_ASM_InterpRenderOpacity.h"
#include "Module/Volt_ASM_InterpWidgetTransform.h"
#include "Module/Volt_ASM_Sequence.h"
#include "Module/Volt_ASM_Simultaneous.h"
#include "Node/Derived/JN_Foundation.h"
#include "Windows/WindowsPlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "JointEditorToolkit"

namespace EditorTapIDs
{
	static const FName AppIdentifier("JointEditorToolkit_019");
	static const FName DetailsID("Joint_DetailsID");
	static const FName PaletteID("Joint_PaletteID");
	static const FName SearchReplaceID("Joint_SearchID");
	static const FName CompileResultID("Joint_CompileResultID");
	static const FName ContentBrowserID("Joint_ContentBrowserID");
	static const FName EditorPreferenceID("Joint_EditorPreferenceID");
	static const FName GraphID("Joint_GraphID");
}


DEFINE_LOG_CATEGORY_STATIC(LogJointEditorToolkit, Log, All);

FJointEditorToolkit::FJointEditorToolkit()
{
	FEditorDelegates::BeginPIE.AddRaw(this, &FJointEditorToolkit::OnBeginPIE);
	FEditorDelegates::EndPIE.AddRaw(this, &FJointEditorToolkit::OnEndPIE);

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}
}

FJointEditorToolkit::~FJointEditorToolkit()
{
	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);

	if (UJointEdGraph* CastedGraph = GetJointGraph())
	{
		CastedGraph->RemoveOnGraphRequestUpdateHandler(GraphRequestUpdateDele.GetHandle());
	}

	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

FGraphPanelSelectionSet FJointEditorToolkit::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;

	if (GraphEditorPtr.IsValid())
	{
		CurrentSelection = GraphEditorPtr->GetSelectedNodes();
	}

	return CurrentSelection;
}


void FJointEditorToolkit::FixupPastedNodes(const TSet<UEdGraphNode*>& NewPastedGraphNodes,
                                           const TMap<FGuid, FGuid>& NewToOldNodeMapping)
{
	for (UEdGraphNode* NewPastedGraphNode : NewPastedGraphNodes)
	{
		UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(NewPastedGraphNode);

		UObject*& Instance = CastedGraphNode->NodeInstance;

		if (CastedGraphNode == nullptr || Instance == nullptr) continue;

		Instance->Rename(
			*MakeUniqueObjectName(Instance->GetOuter(), Instance->GetClass(), Instance->GetFName()).ToString());
	}
}

void FJointEditorToolkit::InitializeDetailView()
{
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsViewPtr = PropertyModule.CreateDetailView(Args);
	DetailsViewPtr->SetObject(GetJointManager());
}

void FJointEditorToolkit::InitializeContentBrowser()
{
	ContentBrowserPtr = SNew(SJointList).OnAssetDoubleClicked(
		this, &FJointEditorToolkit::OnContentBrowserAssetDoubleClicked);
}

void FJointEditorToolkit::InitializeEditorPreferenceView()
{
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	EditorPreferenceViewPtr = PropertyModule.CreateDetailView(Args);
	EditorPreferenceViewPtr->SetObject(UJointEditorSettings::Get());
}

void FJointEditorToolkit::InitializePaletteView()
{
	JointFragmentPalettePtr = SNew(SJointFragmentPalette)
		.ToolKitPtr(SharedThis(this));
}

void FJointEditorToolkit::InitializeManagerViewer()
{
	ManagerViewerPtr = SNew(SJointManagerViewer)
		.ToolKitPtr(SharedThis(this))
		.BuilderArgs(FJointPropertyTreeBuilderArgs(true, true, true))
		.JointManagers({GetJointManager()});
}

TSharedRef<FTabManager::FLayout> GetStandaloneEditorDefaultLayout()
{
	return FTabManager::NewLayout("Standalone_JointEditor_Layout_v8")
		->AddArea(FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
			)
#endif
			->Split(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->SetSizeCoefficient(0.75f)
				->Split(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(EditorTapIDs::SearchReplaceID, ETabState::ClosedTab)
					// Provide the location of DocumentManager, chart
				)
				->Split(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.4)
					->Split(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.75f)
						->AddTab(EditorTapIDs::GraphID, ETabState::OpenedTab)
					)
					->Split(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.25f)
						->AddTab(EditorTapIDs::CompileResultID, ETabState::ClosedTab)

					)
				)
				->Split(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.25)
					->Split(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.75f)
						->AddTab(EditorTapIDs::DetailsID, ETabState::OpenedTab)
					)
					->Split(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.25f)
						->AddTab(EditorTapIDs::PaletteID, ETabState::OpenedTab)
						->AddTab(EditorTapIDs::ContentBrowserID, ETabState::OpenedTab)
						->AddTab(EditorTapIDs::EditorPreferenceID, ETabState::OpenedTab)
						->SetForegroundTab(EditorTapIDs::PaletteID)
					)

				)
			)

		);
}

void FJointEditorToolkit::InitJointEditor(const EToolkitMode::Type Mode,
                                          const TSharedPtr<class IToolkitHost>& InitToolkitHost,
                                          UJointManager* InJointManager)
{
	JointManager = InJointManager;

	FGraphEditorCommands::Register();
	FGenericCommands::Register();

	AllocateNodePickerIfNeeded();

	BindGraphEditorCommands();
	BindDebuggerCommands();
	
	InitializeGraph();

	//Initialize editor slates.
	InitializeGraphEditor();
	InitializeDetailView();
	InitializePaletteView();
	InitializeContentBrowser();
	InitializeEditorPreferenceView();
	InitializeManagerViewer();
	InitializeCompileResult();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = GetStandaloneEditorDefaultLayout();

	InitAssetEditor(Mode, InitToolkitHost, EditorTapIDs::AppIdentifier, StandaloneDefaultLayout, true, true,
	                JointManager.Get());

	ExtendToolbar();
	RegenerateMenusAndToolbars();
	SetCurrentMode("StandardMode");

	if (GetJointManager() != nullptr && !GetJointManager()->IsAsset())
	{
		PopulateTransientEditingWarningToastMessage();
	}
	// Create a command, pay attention to setcurrentmode ()
}

void FJointEditorToolkit::CleanUp()
{

	if(UJointEdGraph* Graph = GetJointGraph())
	{
		Graph->OnClosed();
	};
	
	JointManager.Reset();

	EditorPreferenceViewPtr.Reset();
	ContentBrowserPtr.Reset();
	GraphEditorPtr.Reset();
	DetailsViewPtr.Reset();
	ManagerViewerPtr.Reset();
	JointFragmentPalettePtr.Reset();
	Toolbar.Reset();

	if(GraphToastMessageHub)
	{
		GraphToastMessageHub->ReleaseVoltAnimationManager();
		GraphToastMessageHub.Reset();	
	}
}

void FJointEditorToolkit::OnClose()
{
	//Clean Up;

	CleanUp();
	
	FWorkflowCentricApplication::OnClose();
}


void FJointEditorToolkit::ExtendToolbar()
{
	const TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	//Assign this extender to the toolkit.
	AddToolbarExtender(ToolbarExtender);

	Toolbar = MakeShareable(new FJointEditorToolbar(SharedThis(this)));
	Toolbar->AddJointToolbar(ToolbarExtender);
	Toolbar->AddEditorModuleToolbar(ToolbarExtender);
	Toolbar->AddDebuggerToolbar(ToolbarExtender);
}


void FJointEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	const auto& LocalCategories = InTabManager->GetLocalWorkspaceMenuRoot()->GetChildItems();

	if (LocalCategories.Num() <= 0)
	{
		TSharedRef<FWorkspaceItem> Group = FWorkspaceItem::NewGroup(LOCTEXT("JointTabGroupName", "Joint Editor"));
		InTabManager->GetLocalWorkspaceMenuRoot()->AddItem(Group);
	}

	AssetEditorTabsCategory = LocalCategories.Num() > 0
		                          ? LocalCategories[0]
		                          : InTabManager->GetLocalWorkspaceMenuRoot();

	InTabManager->RegisterTabSpawner(EditorTapIDs::EditorPreferenceID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_EditorPreference,
	                                                       EditorTapIDs::EditorPreferenceID))
		.SetDisplayName(LOCTEXT("EditorPreferenceTabLabel", "Joint Editor Preference"))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Details"))
		.SetGroup(AssetEditorTabsCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner(EditorTapIDs::ContentBrowserID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_ContentBrowser,
	                                                       EditorTapIDs::ContentBrowserID))
		.SetDisplayName(LOCTEXT("ContentBrowserTabLabel", "Joint Browser"))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Viewports"))
		.SetGroup(AssetEditorTabsCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner(EditorTapIDs::PaletteID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_Palettes,
	                                                       EditorTapIDs::PaletteID))
		.SetDisplayName(LOCTEXT("PaletteTabLabel", "Fragment Palette"))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.MeshPaintMode"))
		.SetGroup(AssetEditorTabsCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner(EditorTapIDs::DetailsID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_Details,
	                                                       EditorTapIDs::DetailsID))
		.SetDisplayName(LOCTEXT("DetailsTabLabel", "Details"))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Details"))
		.SetGroup(AssetEditorTabsCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner(EditorTapIDs::GraphID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_Graph,
	                                                       EditorTapIDs::GraphID))
		.SetDisplayName(LOCTEXT("EditorTapIDsTabName", "Graph"))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Viewports"))
		.SetGroup(AssetEditorTabsCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(EditorTapIDs::SearchReplaceID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_Search,
	                                                       EditorTapIDs::SearchReplaceID))
		.SetDisplayName(LOCTEXT("SearchTabLabel", "Search"))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Outliner"))
		.SetGroup(AssetEditorTabsCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner(EditorTapIDs::CompileResultID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_CompileResult,
	                                                       EditorTapIDs::CompileResultID))
		.SetDisplayName(LOCTEXT("CompileResultTabLabel", "Compile Result"))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "Kismet.Tabs.CompilerResults"))
		.SetGroup(AssetEditorTabsCategory.ToSharedRef());
}


void FJointEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(EditorTapIDs::ContentBrowserID);
	InTabManager->UnregisterTabSpawner(EditorTapIDs::EditorPreferenceID);
	InTabManager->UnregisterTabSpawner(EditorTapIDs::PaletteID);
	InTabManager->UnregisterTabSpawner(EditorTapIDs::DetailsID);
	InTabManager->UnregisterTabSpawner(EditorTapIDs::GraphID);
	InTabManager->UnregisterTabSpawner(EditorTapIDs::SearchReplaceID);
	InTabManager->UnregisterTabSpawner(EditorTapIDs::CompileResultID);
}

TSharedRef<SDockTab> FJointEditorToolkit::SpawnTab_EditorPreference(const FSpawnTabArgs& Args,
                                                                    FName TabIdentifier) const
{
	TSharedRef<SDockTab> EditorPreferenceTabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointEditorPreferenceTabTitle", "Joint Editor Preference"));

	TSharedPtr<SWidget> TabWidget = SNullWidget::NullWidget;

	if (TabIdentifier == EditorTapIDs::EditorPreferenceID && EditorPreferenceViewPtr.IsValid())
	{
		EditorPreferenceTabPtr->SetContent(EditorPreferenceViewPtr.ToSharedRef());
	}

	return EditorPreferenceTabPtr;
}

TSharedRef<SDockTab> FJointEditorToolkit::SpawnTab_ContentBrowser(const FSpawnTabArgs& Args, FName TabIdentifier)
{
	TSharedRef<SDockTab> ContentBrowserTabPtr = SNew(SDockTab)
		.Label(LOCTEXT("JointContentBrowserTitle", "Joint Browser"));

	if (ContentBrowserPtr.IsValid()) { ContentBrowserTabPtr->SetContent(ContentBrowserPtr.ToSharedRef()); }

	return ContentBrowserTabPtr;
}


TSharedRef<SDockTab> FJointEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args, FName TabIdentifier)
{
	TSharedRef<SDockTab> DetailsTabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointDetailsTabTitle", "Details"));

	if (TabIdentifier == EditorTapIDs::DetailsID && DetailsViewPtr.IsValid())
	{
		DetailsTabPtr->SetContent(DetailsViewPtr.ToSharedRef());
	}

	return DetailsTabPtr;
}

TSharedRef<SDockTab> FJointEditorToolkit::SpawnTab_Palettes(const FSpawnTabArgs& Args, FName TabIdentifier)
{
	TSharedRef<SDockTab> JointFragmentPaletteTabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointFragmentPaletteTabTitle", "Fragment Palette"));

	if (TabIdentifier == EditorTapIDs::PaletteID && JointFragmentPalettePtr.IsValid())
	{
		JointFragmentPaletteTabPtr->SetContent(JointFragmentPalettePtr.ToSharedRef());
	}

	return JointFragmentPaletteTabPtr;
}

TSharedRef<SDockTab> FJointEditorToolkit::SpawnTab_Graph(const FSpawnTabArgs& Args, FName TabIdentifier)
{
	TSharedRef<SDockTab> GraphEditorTabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointGraphCanvasTitle", "Graph"));

	if (GraphEditorPtr.IsValid()) { GraphEditorTabPtr->SetContent(GraphEditorPtr.ToSharedRef()); }

	return GraphEditorTabPtr;
}

#include "IPropertyTable.h"

TSharedRef<SDockTab> FJointEditorToolkit::SpawnTab_Search(const FSpawnTabArgs& Args, FName TabIdentifier)
{
	TSharedRef<SDockTab> SearchTabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointSearchTabTitle", "Search & Replace"));

	if (ManagerViewerPtr.IsValid()) { SearchTabPtr->SetContent(ManagerViewerPtr.ToSharedRef()); }

	return SearchTabPtr;
}

TSharedRef<SDockTab> FJointEditorToolkit::SpawnTab_CompileResult(const FSpawnTabArgs& Args, FName TabIdentifier)
{
	TSharedRef<SDockTab> CompileResultTabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointCompileResultTabTitle", "Compile Result"));

	if (GetJointGraph() && GetJointGraph()->CompileResultPtr.IsValid())
	{
		if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
		{
			FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");

			CompileResultTabPtr->SetContent(
				MessageLogModule.CreateLogListingWidget(GetJointGraph()->CompileResultPtr.ToSharedRef()));
		}
	}

	return CompileResultTabPtr;
}

void FJointEditorToolkit::RequestManagerViewerRefresh()
{
	if (ManagerViewerPtr.IsValid()) ManagerViewerPtr->RequestTreeRebuild();
}


FName FJointEditorToolkit::GetToolkitFName() const { return FName("Joint Editor"); }

FText FJointEditorToolkit::GetBaseToolkitName() const { return LOCTEXT("JointEditor", "Joint Editor"); }

FString FJointEditorToolkit::GetWorldCentricTabPrefix() const { return FString(); }

FLinearColor FJointEditorToolkit::GetWorldCentricTabColorScale() const { return FLinearColor(); }

void FJointEditorToolkit::PostUndo(bool bSuccess)
{
	RequestManagerViewerRefresh();

	// Clear selection, to avoid holding refs to nodes that go away
	if (TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr)
	{
		CurrentGraphEditor->ClearSelectionSet();
		CurrentGraphEditor->NotifyGraphChanged();
	}

	FSlateApplication::Get().DismissAllMenus();
}

void FJointEditorToolkit::PostRedo(bool bSuccess)
{
	RequestManagerViewerRefresh();

	// Clear selection, to avoid holding refs to nodes that go away
	if (TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr)
	{
		CurrentGraphEditor->ClearSelectionSet();
		CurrentGraphEditor->NotifyGraphChanged();
	}

	FSlateApplication::Get().DismissAllMenus();
}

void FJointEditorToolkit::OnCompileJoint()
{
	if (UJointEdGraph* Graph = GetJointGraph()) Graph->CompileJointGraph();

	//Make it display the tab whenever users manually pressed the button.
	if (TabManager)
	{
		TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(EditorTapIDs::CompileResultID);

		if (Tab.IsValid())
		{
			Tab->FlashTab();
		}
	}
}

bool FJointEditorToolkit::CanCompileJoint()
{
	return !UJointDebugger::IsPIESimulating();
}

void FJointEditorToolkit::OnCompileJointFinished(
	const UJointEdGraph::FJointGraphCompileInfo& CompileInfo) const
{
	if (GetJointGraph() && GetJointGraph()->CompileResultPtr)
	{
		TWeakPtr<class IMessageLogListing> WeakCompileResultPtr = GetJointGraph()->CompileResultPtr;

		if (GetJointManager() == nullptr)
		{
			const TSharedRef<FTokenizedMessage> Token = FTokenizedMessage::Create(
				EMessageSeverity::Error,
				LOCTEXT("JointCompileLogMsg_Error_NoJointManager",
				        "No valid Joint manager. This editor might be opened compulsively or refer to a PIE transient Joint manager.")
			);

			WeakCompileResultPtr.Pin()->AddMessage(Token);

			return;
		}


		const int NumError = WeakCompileResultPtr.Pin()->NumMessages(EMessageSeverity::Error);
		const int NumWarning = WeakCompileResultPtr.Pin()->NumMessages(EMessageSeverity::Warning);
		const int NumInfo = WeakCompileResultPtr.Pin()->NumMessages(EMessageSeverity::Info);
		const int NumPerformanceWarning = WeakCompileResultPtr.Pin()->NumMessages(EMessageSeverity::PerformanceWarning);


		GetJointManager()->Status =
			NumError
				? EBlueprintStatus::BS_Error
				: NumWarning
				? EBlueprintStatus::BS_UpToDateWithWarnings
				: NumInfo
				? EBlueprintStatus::BS_UpToDate
				: NumPerformanceWarning
				? EBlueprintStatus::BS_UpToDateWithWarnings
				: EBlueprintStatus::BS_UpToDate;


		TSharedRef<FTokenizedMessage> Token = FTokenizedMessage::Create(
			EMessageSeverity::Info,
			FText::Format(LOCTEXT("CompileFinished",
			                      "Compilation finished. [{0}] {1} Fatal Issue(s) {2} Warning(s) {3} Info. (Compiled through {4} nodes total, {5}ms elapsed on the compilation.)")
			              , FText::FromString(GetJointManager()->GetPathName())
			              , NumError

			              , NumWarning + NumPerformanceWarning
			              , NumInfo
			              , CompileInfo.NodeCount
			              , CompileInfo.ElapsedTime)
		);

		WeakCompileResultPtr.Pin()->AddMessage(Token);


		if (GetJointManager()->Status != EBlueprintStatus::BS_UpToDate)
		{
			if (TabManager)
			{
				TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(EditorTapIDs::CompileResultID);

				if (Tab.IsValid()) Tab->FlashTab();
			}
		}
	}
}

void FJointEditorToolkit::OnCompileResultTokenClicked(const TSharedRef<IMessageToken>& MessageToken)
{
	if (MessageToken->GetType() == EMessageToken::Object)
	{
		const TSharedRef<FUObjectToken> UObjectToken = StaticCastSharedRef<FUObjectToken>(MessageToken);
		if (UObjectToken->GetObject().IsValid())
		{
			JumpToHyperlink(UObjectToken->GetObject().Get());
		}
	}
	else if (MessageToken->GetType() == EMessageToken::EdGraph)
	{
		const TSharedRef<FEdGraphToken> EdGraphToken = StaticCastSharedRef<FEdGraphToken>(MessageToken);
		const UEdGraphPin* PinBeingReferenced = EdGraphToken->GetPin();
		const UObject* ObjectBeingReferenced = EdGraphToken->GetGraphObject();
		if (PinBeingReferenced)
		{
			//JumpToPin(PinBeingReferenced);
		}
		else if (ObjectBeingReferenced)
		{
			//JumpToHyperlink(ObjectBeingReferenced);
		}
	}
}

void FJointEditorToolkit::FeedEditorSlateToEachTab() const
{
	const TSharedPtr<SDockTab> DetailTab = TabManager->TryInvokeTab(EditorTapIDs::DetailsID);

	if (DetailTab.IsValid())
	{
		DetailTab->ClearContent();
		DetailTab->SetContent(DetailsViewPtr.ToSharedRef());
	}

	const TSharedPtr<SDockTab> GraphTab = TabManager->TryInvokeTab(EditorTapIDs::GraphID);

	if (GraphTab.IsValid())
	{
		GraphTab->ClearContent();
		GraphTab->SetContent(GraphEditorPtr.ToSharedRef());
	}

	const TSharedPtr<SDockTab> SearchTab = TabManager->TryInvokeTab(EditorTapIDs::SearchReplaceID);

	if (SearchTab.IsValid())
	{
		SearchTab->ClearContent();
		SearchTab->SetContent(ManagerViewerPtr.ToSharedRef());
	}

	const TSharedPtr<SDockTab> ContentBrowserTab = TabManager->TryInvokeTab(EditorTapIDs::ContentBrowserID);

	if (ContentBrowserTab.IsValid())
	{
		ContentBrowserTab->ClearContent();
		ContentBrowserTab->SetContent(ContentBrowserPtr.ToSharedRef());
	}

	const TSharedPtr<SDockTab> EditorPreferenceTab = TabManager->TryInvokeTab(EditorTapIDs::EditorPreferenceID);

	if (EditorPreferenceTab.IsValid())
	{
		EditorPreferenceTab->ClearContent();
		EditorPreferenceTab->SetContent(EditorPreferenceViewPtr.ToSharedRef());
	}
}

void FJointEditorToolkit::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}


void FJointEditorToolkit::SetJointManagerBeingEdited(UJointManager* NewManager)
{
	if ((NewManager != GetJointManager()) && (NewManager != nullptr))
	{
		UJointManager* OldManager = GetJointManager();
		JointManager = NewManager;

		// Let the editor know that are editing something different
		RemoveEditingObject(OldManager);
		AddEditingObject(NewManager);

		InitializeGraphEditor();
		InitializeDetailView();
		InitializeContentBrowser();
		InitializeEditorPreferenceView();
		InitializeManagerViewer();
		InitializeCompileResult();

		FeedEditorSlateToEachTab();
	}
}

void FJointEditorToolkit::BindGraphEditorCommands()
{
	// Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
	// however it should be safe, since commands are being used only within this editor
	// if it ever crashes, this function will have to go away and be reimplemented in each derived class


	ToolkitCommands->MapAction(FGenericCommands::Get().SelectAll
	                           , FExecuteAction::CreateRaw(this, &FJointEditorToolkit::SelectAllNodes)
	                           , FCanExecuteAction::CreateRaw(this, &FJointEditorToolkit::CanSelectAllNodes)
	);

	ToolkitCommands->MapAction(FGenericCommands::Get().Delete
	                           , FExecuteAction::CreateRaw(
		                           this, &FJointEditorToolkit::DeleteSelectedNodes)
	                           , FCanExecuteAction::CreateRaw(this, &FJointEditorToolkit::CanDeleteNodes)
	);

	ToolkitCommands->MapAction(FGenericCommands::Get().Copy
	                           , FExecuteAction::CreateRaw(
		                           this, &FJointEditorToolkit::CopySelectedNodes)
	                           , FCanExecuteAction::CreateRaw(this, &FJointEditorToolkit::CanCopyNodes)
	);

	ToolkitCommands->MapAction(FGenericCommands::Get().Cut
	                           , FExecuteAction::CreateRaw(
		                           this, &FJointEditorToolkit::CutSelectedNodes)
	                           , FCanExecuteAction::CreateRaw(this, &FJointEditorToolkit::CanCutNodes)
	);

	ToolkitCommands->MapAction(FGenericCommands::Get().Paste
	                           , FExecuteAction::CreateRaw(this, &FJointEditorToolkit::PasteNodes)
	                           , FCanExecuteAction::CreateRaw(this, &FJointEditorToolkit::CanPasteNodes)
	);

	ToolkitCommands->MapAction(FGenericCommands::Get().Duplicate
	                           , FExecuteAction::CreateRaw(this, &FJointEditorToolkit::DuplicateNodes)
	                           , FCanExecuteAction::CreateRaw(this, &FJointEditorToolkit::CanDuplicateNodes)
	);

	ToolkitCommands->MapAction(FGenericCommands::Get().Rename
	                           , FExecuteAction::CreateRaw(this, &FJointEditorToolkit::RenameNodes)
	                           , FCanExecuteAction::CreateRaw(this, &FJointEditorToolkit::CanRenameNodes)
	);

	// Debug actions
	ToolkitCommands->MapAction(FGraphEditorCommands::Get().AddBreakpoint,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnAddBreakpoint),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanAddBreakpoint),
	                           FIsActionChecked(),
	                           FIsActionButtonVisible::CreateSP(this, &FJointEditorToolkit::CanAddBreakpoint)
	);

	ToolkitCommands->MapAction(FGraphEditorCommands::Get().RemoveBreakpoint,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnRemoveBreakpoint),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanRemoveBreakpoint),
	                           FIsActionChecked(),
	                           FIsActionButtonVisible::CreateSP(this, &FJointEditorToolkit::CanRemoveBreakpoint)
	);

	ToolkitCommands->MapAction(FGraphEditorCommands::Get().EnableBreakpoint,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnEnableBreakpoint),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanEnableBreakpoint),
	                           FIsActionChecked(),
	                           FIsActionButtonVisible::CreateSP(this, &FJointEditorToolkit::CanEnableBreakpoint)
	);

	ToolkitCommands->MapAction(FGraphEditorCommands::Get().DisableBreakpoint,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnDisableBreakpoint),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanDisableBreakpoint),
	                           FIsActionChecked(),
	                           FIsActionButtonVisible::CreateSP(this, &FJointEditorToolkit::CanDisableBreakpoint)
	);

	ToolkitCommands->MapAction(FGraphEditorCommands::Get().ToggleBreakpoint,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnToggleBreakpoint),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanToggleBreakpoint),
	                           FIsActionChecked(),
	                           FIsActionButtonVisible::CreateSP(this, &FJointEditorToolkit::CanToggleBreakpoint)
	);

	ToolkitCommands->MapAction(FGraphEditorCommands::Get().CreateComment,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnCreateComment),
	                           FCanExecuteAction(),
	                           FIsActionChecked()
	);


	const FJointEditorCommands& Commands = FJointEditorCommands::Get();

	ToolkitCommands->MapAction(Commands.CompileJoint,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnCompileJoint),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanCompileJoint)
	);

	ToolkitCommands->MapAction(
		Commands.OpenSearchTab
		, FExecuteAction::CreateSP(this, &FJointEditorToolkit::OpenSearchTab)
		, FCanExecuteAction());

	ToolkitCommands->MapAction(
		Commands.OpenReplaceTab
		, FExecuteAction::CreateSP(this, &FJointEditorToolkit::OpenReplaceTab)
		, FCanExecuteAction());

	if(GetNodePickingManager().IsValid())
	{
		ToolkitCommands->MapAction(
			Commands.EscapeNodePickingMode
			, FExecuteAction::CreateSP( GetNodePickingManager().ToSharedRef(), &FJointEditorNodePickingManager::EndNodePicking)
			, FCanExecuteAction());
	}


	ToolkitCommands->MapAction(
		Commands.SetShowNormalConnection
		, FExecuteAction::CreateSP(this, &FJointEditorToolkit::ToggleShowNormalConnection)
		, FCanExecuteAction()
		, FIsActionChecked::CreateSP(this, &FJointEditorToolkit::IsShowNormalConnectionChecked));

	ToolkitCommands->MapAction(
		Commands.SetShowRecursiveConnection
		, FExecuteAction::CreateSP(this, &FJointEditorToolkit::ToggleShowRecursiveConnection)
		, FCanExecuteAction()
		, FIsActionChecked::CreateSP(this, &FJointEditorToolkit::IsShowRecursiveConnectionChecked));


	ToolkitCommands->MapAction(Commands.JumpToSelection,
						   FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnJumpToSelection),
						   FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanJumpToSelection),
						   FIsActionChecked()
	);

	
	ToolkitCommands->MapAction(Commands.RemoveAllBreakpoints,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnRemoveAllBreakpoints),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanRemoveAllBreakpoints)
	);

	ToolkitCommands->MapAction(Commands.EnableAllBreakpoints,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnEnableAllBreakpoints),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanEnableAllBreakpoints)
	);


	ToolkitCommands->MapAction(Commands.DisableAllBreakpoints,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnDisableAllBreakpoints),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanDisableAllBreakpoints)
	);


	ToolkitCommands->MapAction(Commands.CreateFoundation,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnCreateFoundation),
	                           FCanExecuteAction(),
	                           FIsActionChecked()
	);


	ToolkitCommands->MapAction(Commands.ToggleDebuggerExecution,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnToggleDebuggerExecution),
	                           FCanExecuteAction(),
	                           FIsActionChecked::CreateSP(
		                           this, &FJointEditorToolkit::GetCheckedToggleDebuggerExecution)
	);

	ToolkitCommands->MapAction(Commands.ShowIndividualVisibilityButtonForSimpleDisplayProperty,
	                           FExecuteAction::CreateSP(
		                           this, &FJointEditorToolkit::OnToggleVisibilityChangeModeForSimpleDisplayProperty),
	                           FCanExecuteAction(),
	                           FIsActionChecked::CreateSP(
		                           this,
		                           &FJointEditorToolkit::GetCheckedToggleVisibilityChangeModeForSimpleDisplayProperty));
}

void FJointEditorToolkit::BindDebuggerCommands()
{
	const FJointDebuggerCommands& Commands = FJointDebuggerCommands::Get();
	UJointDebugger* DebuggerOb = UJointDebugger::Get();

	ToolkitCommands->MapAction(
		Commands.ForwardInto,
		FExecuteAction::CreateUObject(DebuggerOb, &UJointDebugger::StepForwardInto),
		FCanExecuteAction::CreateUObject(DebuggerOb, &UJointDebugger::CanStepForwardInto),
		FIsActionChecked(),
		//TODO: Change it to visible only when the breakpoint is hit.
		FIsActionChecked::CreateStatic(&UJointDebugger::IsPIESimulating));

	ToolkitCommands->MapAction(
		Commands.ForwardOver,
		FExecuteAction::CreateUObject(DebuggerOb, &UJointDebugger::StepForwardOver),
		FCanExecuteAction::CreateUObject(DebuggerOb, &UJointDebugger::CanStepForwardOver),
		FIsActionChecked(),
		//TODO: Change it to visible only when the breakpoint is hit.
		FIsActionChecked::CreateStatic(&UJointDebugger::IsPIESimulating));

	ToolkitCommands->MapAction(
		Commands.StepOut,
		FExecuteAction::CreateUObject(DebuggerOb, &UJointDebugger::StepOut),
		FCanExecuteAction::CreateUObject(DebuggerOb, &UJointDebugger::CanStepOut),
		FIsActionChecked(),
		//TODO: Change it to visible only when the breakpoint is hit.
		FIsActionChecked::CreateStatic(&UJointDebugger::IsPIESimulating));

	ToolkitCommands->MapAction(
		Commands.PausePlaySession,
		FExecuteAction::CreateStatic(&UJointDebugger::PausePlaySession),
		FCanExecuteAction::CreateStatic(&UJointDebugger::IsPlaySessionRunning),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic(&UJointDebugger::IsPlaySessionRunning));

	ToolkitCommands->MapAction(
		Commands.ResumePlaySession,
		FExecuteAction::CreateStatic(&UJointDebugger::ResumePlaySession),
		FCanExecuteAction::CreateStatic(&UJointDebugger::IsPlaySessionPaused),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic(&UJointDebugger::IsPlaySessionPaused));

	ToolkitCommands->MapAction(
		Commands.StopPlaySession,
		FExecuteAction::CreateStatic(&UJointDebugger::StopPlaySession),
		FCanExecuteAction::CreateStatic(&UJointDebugger::IsPIESimulating),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic(&UJointDebugger::IsPIESimulating));
}

void FJointEditorToolkit::CreateNewGraphForEditingJointManagerIfNeeded() const
{
	if (GetJointManager() == nullptr) return;

	if (GetJointManager()->JointGraph != nullptr) return;

	UJointEdGraph* NewJointGraph = nullptr;

	NewJointGraph = Cast<UJointEdGraph>(FBlueprintEditorUtils::CreateNewGraph(
		GetJointManager()
		, FName("JointGraph")
		, UJointEdGraph::StaticClass()
		, UJointEdGraphSchema::StaticClass()));


	//Feed the newly created Joint graph to the Joint manager.
	GetJointManager()->JointGraph = NewJointGraph;

	NewJointGraph->JointManager = GetJointManager();

	NewJointGraph->GetSchema()->CreateDefaultNodesForGraph(*NewJointGraph);

	//Notify and initialize the graph.
	NewJointGraph->OnCreated();
}

void FJointEditorToolkit::InitializeGraph()
{
	UJointEdGraph* Graph = GetJointGraph();

	if (Graph != nullptr)
	{
		Graph->OnLoaded();
	}
	else
	{
		CreateNewGraphForEditingJointManagerIfNeeded();
	}

	if (UJointEdGraph* NewGraph = GetJointGraph())
	{
		NewGraph->Toolkit = SharedThis(this);
		NewGraph->Initialize();
	}
}

void FJointEditorToolkit::InitializeGraphEditor()
{
	if (!GetJointManager()) return;

	SJointGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FJointEditorToolkit::OnNodeTitleCommitted);
	InEvents.OnVerifyTextCommit = FOnNodeVerifyTextCommit::CreateSP(
		this, &FJointEditorToolkit::OnVerifyNodeTitleChanged);

	InEvents.OnSelectionChanged = SJointGraphEditor::FOnSelectionChanged::CreateSP(
		this, &FJointEditorToolkit::OnGraphSelectedNodesChanged);

	GraphRequestUpdateDele = FOnGraphRequestUpdate::FDelegate::CreateSP(
		this, &FJointEditorToolkit::OnGraphRequestUpdate);

	if (UJointEdGraph* CastedGraph = GetJointGraph())
	{
		CastedGraph->AddOnGraphRequestUpdateHandler(GraphRequestUpdateDele);
	}

	GraphToastMessageHub = SNew(SJointToolkitToastMessageHub);

	//Feed the toolkit to the graph.
	if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(GetJointManager()->JointGraph))
	{
		CastedGraph->Toolkit = SharedThis(this);
	}

	GraphEditorPtr = SNew(SJointGraphEditor)
		.TitleBar(GraphToastMessageHub)
		.AdditionalCommands(ToolkitCommands)
		.IsEditable(GetJointManager()->IsAsset())
		.GraphToEdit(GetJointManager()->JointGraph)
		.GraphEvents(InEvents);
}

void FJointEditorToolkit::InitializeCompileResult()
{
	if (GetJointGraph() && !GetJointGraph()->CompileResultPtr)
	{
		GetJointGraph()->InitializeCompileResultIfNeeded();
	}

	if (GetJointGraph() && GetJointGraph()->CompileResultPtr)
	{
		GetJointGraph()->OnCompileFinished.Unbind();
		GetJointGraph()->CompileResultPtr->OnMessageTokenClicked().Clear();

		GetJointGraph()->OnCompileFinished.BindSP(this, &FJointEditorToolkit::OnCompileJointFinished);
		GetJointGraph()->CompileResultPtr->OnMessageTokenClicked().AddSP(
			this, &FJointEditorToolkit::OnCompileResultTokenClicked);

		GetJointGraph()->CompileJointGraph();
	}
}

void FJointEditorToolkit::OnGraphRequestUpdate()
{
	RequestManagerViewerRefresh();
}

UJointEdGraph* FJointEditorToolkit::GetJointGraph() const
{
	if (GetJointManager() == nullptr) return nullptr;

	if (GetJointManager()->JointGraph == nullptr) return nullptr;

	return Cast<UJointEdGraph>(GetJointManager()->JointGraph);
}

UJointManager* FJointEditorToolkit::GetJointManager() const
{
	return JointManager.Get();
}

void FJointEditorToolkit::OpenSearchTab() const
{
	if (!ManagerViewerPtr.IsValid()) return;

	if (TSharedPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(EditorTapIDs::SearchReplaceID))
	{
		if (ManagerViewerPtr->GetMode() != EJointManagerViewerMode::Search)
		{
			//If the tab is already opened and not showing the search mode, change it to search mode.
			ManagerViewerPtr->SetMode(EJointManagerViewerMode::Search);
		}
		else
		{
			//If the tab is already opened and showing the search mode, close it.
			FoundTab->RequestCloseTab();
		}
	}
	else
	{
		//If the tab is not live, then open it.
		TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(EditorTapIDs::SearchReplaceID);

		ManagerViewerPtr->SetMode(EJointManagerViewerMode::Search);
	}
}


void FJointEditorToolkit::OpenReplaceTab() const
{
	if (!ManagerViewerPtr.IsValid()) return;

	if (TSharedPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(EditorTapIDs::SearchReplaceID))
	{
		if (ManagerViewerPtr->GetMode() != EJointManagerViewerMode::Replace)
		{
			//If the tab is already opened and not showing the search mode, change it to search mode.
			ManagerViewerPtr->SetMode(EJointManagerViewerMode::Replace);
		}
		else
		{
			//If the tab is already opened and showing the search mode, close it.
			FoundTab->RequestCloseTab();
		}
	}
	else
	{
		//If the tab is not live, then open it.
		TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(EditorTapIDs::SearchReplaceID);

		ManagerViewerPtr->SetMode(EJointManagerViewerMode::Replace);
	}
}


void FJointEditorToolkit::OnBeginPIE(bool bArg)
{
}

void FJointEditorToolkit::OnEndPIE(bool bArg)
{
	if (GetJointGraph() == nullptr) return;

	TSet<TSoftObjectPtr<UJointEdGraphNode>> GraphNodes = GetJointGraph()->GetCacheJointGraphNodes();

	for (TSoftObjectPtr<UJointEdGraphNode> CachedJointGraphNode : GraphNodes)
	{
		if (CachedJointGraphNode == nullptr) continue;

		if (!CachedJointGraphNode->GetGraphNodeSlate().IsValid()) continue;

		CachedJointGraphNode->GetGraphNodeSlate().Pin()->ResetNodeBodyColorAnimation();
	}
}


void FJointEditorToolkit::PopulateNodePickingToastMessage()
{
	ClearNodePickingToastMessage();

	if (GraphToastMessageHub.IsValid())
	{
		TWeakPtr<SJointToolkitToastMessage> Message = GraphToastMessageHub->FindToasterMessage(
			NodePickingToastMessageGuid);

		if (Message.IsValid())
		{
			Message.Pin()->PlayAppearAnimation();
		}
		else
		{
			NodePickingToastMessageGuid = GraphToastMessageHub->AddToasterMessage(
				SNew(SJointToolkitToastMessage)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Border)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
					.Padding(FJointEditorStyle::Margin_Border)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Border)
							[
								SNew(SBox)
								.WidthOverride(24)
								.HeightOverride(24)
								[
									SNew(SImage)
									.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.EyeDropper"))
								]
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Border)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("PickingEnabledTitle", "Node Picking Enabled"))
								.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Border)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PickingEnabled", "Click the node to select. Press ESC to escape."))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
						]
					]
				]
			);
		}
	}
}


void FJointEditorToolkit::PopulateTransientEditingWarningToastMessage()
{
	ClearTransientEditingWarningToastMessage();

	if (GraphToastMessageHub.IsValid())
	{
		TWeakPtr<SJointToolkitToastMessage> Message = GraphToastMessageHub->FindToasterMessage(
			TransientEditingToastMessageGuid);

		if (Message.IsValid())
		{
			Message.Pin()->PlayAppearAnimation();
		}
		else
		{
			TransientEditingToastMessageGuid = GraphToastMessageHub->AddToasterMessage(
				SNew(SJointToolkitToastMessage)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Border)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
					.Padding(FJointEditorStyle::Margin_Border)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Border)
							[
								SNew(SBox)
								.WidthOverride(24)
								.HeightOverride(24)
								[
									SNew(SImage)
									.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Star"))
								]
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Border)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("PickingEnabledTitle",
								              "You are editing a transient & PIE duplicated object."))
								.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Border)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PickingEnabled",
							              "Any modification on this graph will not be applied to the original asset."))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
						]
					]
				]
			);
		}
	}
}

void FJointEditorToolkit::PopulateVisibilityChangeModeForSimpleDisplayPropertyToastMessage()
{
	ClearVisibilityChangeModeForSimpleDisplayPropertyToastMessage();

	if (GraphToastMessageHub.IsValid())
	{
		TWeakPtr<SJointToolkitToastMessage> Message = GraphToastMessageHub->FindToasterMessage(
			VisibilityChangeModeForSimpleDisplayPropertyToastMessageGuid);

		if (Message.IsValid())
		{
			Message.Pin()->PlayAppearAnimation();
		}
		else
		{
			VisibilityChangeModeForSimpleDisplayPropertyToastMessageGuid = GraphToastMessageHub->AddToasterMessage(
				SNew(SJointToolkitToastMessage)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Border)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
					.Padding(FJointEditorStyle::Margin_Border)
					[
						SNew(SBorder)
						.Padding(FJointEditorStyle::Margin_Border)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
						.Padding(FJointEditorStyle::Margin_Border)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.Padding(FJointEditorStyle::Margin_Border)
								[
									SNew(SBox)
									.WidthOverride(24)
									.HeightOverride(24)
									[
										SNew(SImage)
										.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Edit"))
									]
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.Padding(FJointEditorStyle::Margin_Border)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("PickingEnabledTitle",
									              "Modifying Simple Display Property Visibility"))
									.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Border)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("PickingEnabled",
								              "Press the eye buttons to change their visibility. Press \'X\' to end."))
								.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
							]
						]
					]
				]
			);
		}
	}
}

void FJointEditorToolkit::PopulateNodePickerCopyToastMessage()
{
	ClearNodePickerCopyToastMessage();

	if (GraphToastMessageHub.IsValid())
	{
		NodePickerCopyToastMessageGuid = GraphToastMessageHub->AddToasterMessage(
			SNew(SJointToolkitToastMessage)
			.RemoveAnimationDuration(1.6)
			.Duration(0.8)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Border)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Border)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Border)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
					.Padding(FJointEditorStyle::Margin_Border)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Border)
							[
								SNew(SBox)
								.WidthOverride(24)
								.HeightOverride(24)
								[
									SNew(SImage)
									.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Edit"))
								]
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Border)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("CopyTitle",
								              "Node Pointer Copied!"))
								.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Border)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CopyTitleEnabled",
							              "Press paste button on others to put this there"))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
						]
					]
				]
			]
		);
	}
}

void FJointEditorToolkit::PopulateNodePickerPastedToastMessage()
{
	ClearNodePickerPastedToastMessage();

	if (GraphToastMessageHub.IsValid())
	{
		NodePickerPasteToastMessageGuid = GraphToastMessageHub->AddToasterMessage(
			SNew(SJointToolkitToastMessage)
			.RemoveAnimationDuration(1.6)
			.Duration(0.8)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Border)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Border)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Border)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
					.Padding(FJointEditorStyle::Margin_Border)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Border)
							[
								SNew(SBox)
								.WidthOverride(24)
								.HeightOverride(24)
								[
									SNew(SImage)
									.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Star"))
								]
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Border)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("PasteTitle",
								              "Node Pointer Pasted!"))
								.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
							]
						]
					]
				]
			]
		);
	}
}

void FJointEditorToolkit::ClearNodePickingToastMessage() const
{
	if (GraphToastMessageHub.IsValid()) GraphToastMessageHub->RemoveToasterMessage(NodePickingToastMessageGuid);
}

void FJointEditorToolkit::ClearTransientEditingWarningToastMessage() const
{
	if (GraphToastMessageHub.IsValid()) GraphToastMessageHub->RemoveToasterMessage(TransientEditingToastMessageGuid);
}

void FJointEditorToolkit::ClearVisibilityChangeModeForSimpleDisplayPropertyToastMessage() const
{
	if (GraphToastMessageHub.IsValid())
		GraphToastMessageHub->RemoveToasterMessage(
			VisibilityChangeModeForSimpleDisplayPropertyToastMessageGuid);
}

void FJointEditorToolkit::ClearNodePickerCopyToastMessage() const
{
	if (GraphToastMessageHub.IsValid()) GraphToastMessageHub->RemoveToasterMessage(NodePickerCopyToastMessageGuid,true);
}

void FJointEditorToolkit::ClearNodePickerPastedToastMessage() const
{
	if (GraphToastMessageHub.IsValid()) GraphToastMessageHub->RemoveToasterMessage(NodePickerPasteToastMessageGuid,true);
}

void FJointEditorToolkit::OnContentBrowserAssetDoubleClicked(const FAssetData& AssetData)
{
	if (AssetData.GetAsset()) AssetViewUtils::OpenEditorForAsset(AssetData.GetAsset());
}

TSharedPtr<FJointEditorNodePickingManager> FJointEditorToolkit::GetNodePickingManager() const
{
	return NodePickingManager;
}

void FJointEditorToolkit::AllocateNodePickerIfNeeded()
{
	NodePickingManager = MakeShared<FJointEditorNodePickingManager>(SharedThis(this));
}

void FJointEditorToolkit::StartHighlightingNode(UJointEdGraphNode* NodeToHighlight, bool bBlinkForOnce)
{
	if (NodeToHighlight == nullptr) return;

	if (!NodeToHighlight->GetGraphNodeSlate().IsValid()) return;

	NodeToHighlight->GetGraphNodeSlate().Pin()->PlayHighlightAnimation(bBlinkForOnce);
}

void FJointEditorToolkit::StopHighlightingNode(UJointEdGraphNode* NodeToHighlight)
{
	if (NodeToHighlight == nullptr) return;

	if (!NodeToHighlight->GetGraphNodeSlate().IsValid()) return;

	NodeToHighlight->GetGraphNodeSlate().Pin()->StopHighlightAnimation();
}

void FJointEditorToolkit::JumpToNode(UEdGraphNode* Node, const bool bRequestRename)
{
	if (!GraphEditorPtr.IsValid()) return;

	const UEdGraphNode* FinalCenterTo = Node;

	//if the node was UJointEdGraphNode type, then make it sure to be centered to its parentmost node.
	if (Cast<UJointEdGraphNode>(Node))
	{
		StartHighlightingNode(Cast<UJointEdGraphNode>(Node), true);

		FinalCenterTo = Cast<UJointEdGraphNode>(Node)->GetParentmostNode();
	}

	GraphEditorPtr->JumpToNode(FinalCenterTo, false, false);
}


void FJointEditorToolkit::JumpToHyperlink(UObject* ObjectReference, bool bRequestRename)
{
	if (UEdGraphNode* GraphNodeNode = Cast<UEdGraphNode>(ObjectReference))
	{
		JumpToNode(GraphNodeNode, false);
	}
	else if (UJointNodeBase* Node = Cast<UJointNodeBase>(ObjectReference))
	{
		JumpToNode(GetJointGraph()->FindGraphNodeForNodeInstance(Node), false);
	}
	else if (const UBlueprintGeneratedClass* Class = Cast<const UBlueprintGeneratedClass>(ObjectReference))
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Class->ClassGeneratedBy);
	}
	else if ((ObjectReference != nullptr) && ObjectReference->IsAsset())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(const_cast<UObject*>(ObjectReference));
	}
	else
	{
		UE_LOG(LogBlueprint, Warning, TEXT("Unknown type of hyperlinked object (%s), cannot focus it"),
		       *GetNameSafe(ObjectReference));
	}
}

void FJointEditorToolkit::OnClassListUpdated()
{
	if (!GraphEditorPtr.IsValid()) return;

	/*UJointEdGraph* MyGraph = Cast<UJointEdGraph>(GraphEditorPtr->GetCurrentGraph());
	if (MyGraph)
	{
		const bool bUpdated = MyGraph->UpdateUnknownNodeClasses();
		if (bUpdated)
		{
			FGraphPanelSelectionSet CurrentSelection = GetSelectedNodes();
			OnSelectedNodesChanged(CurrentSelection);

			MyGraph->UpdateAsset();
		}
	}*/
}


void FJointEditorToolkit::DeleteSelectedNodes()
{
	const TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr;

	if (!CurrentGraphEditor.IsValid()) return;

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

	UJointEdGraph* InGraph = GetJointGraph();

	if (!InGraph) return;

	InGraph->Modify();

	InGraph->LockUpdates();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (!*NodeIt) continue;

		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
		{
			if (!Node->CanUserDeleteNode()) continue;

			Node->Modify();

			if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node))
			{
				if (CastedNode->GetCastedNodeInstance()) CastedNode->GetCastedNodeInstance()->Modify();

				UJointEdGraphNode* SavedParentNode = CastedNode->GetParentmostNode();

				if (SavedParentNode)
				{
					SavedParentNode->Modify();

					if (SavedParentNode->GetCastedNodeInstance())
					{
						SavedParentNode->GetCastedNodeInstance()->Modify();
					}
					
					for (UJointEdGraphNode* SubNodes : SavedParentNode->GetAllSubNodesInHierarchy())
					{
						if (SubNodes)
						{
							SubNodes->Modify();

							if (SubNodes->GetCastedNodeInstance())
							{
								SubNodes->GetCastedNodeInstance()->Modify();
							}
						}
					}
				}


				Node->DestroyNode();

				if (SavedParentNode)
				{
					SavedParentNode->RequestUpdateSlate();
				}
			}
			else
			{
				Node->DestroyNode();
			}
		}
	}

	InGraph->UnlockUpdates();

	InGraph->NotifyGraphChanged();

	RequestManagerViewerRefresh();
}

void FJointEditorToolkit::DeleteSelectedDuplicatableNodes()
{
	TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr;
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

bool FJointEditorToolkit::CanSelectAllNodes() const
{
	return true;
}

bool FJointEditorToolkit::CanDeleteNodes() const
{
	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

bool FJointEditorToolkit::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

bool FJointEditorToolkit::CanCopyNodes() const
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}

bool FJointEditorToolkit::CanPasteNodes() const
{
	TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr;
	if (!CurrentGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

bool FJointEditorToolkit::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void FJointEditorToolkit::RenameNodes()
{
	TSharedPtr<SJointGraphEditor> FocusedGraphEd = GraphEditorPtr;
	if (FocusedGraphEd.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt); SelectedNode != nullptr &&
				SelectedNode->GetCanRenameNode())
			{
				SelectedNode->RequestRenameOnGraphNodeSlate();
				break;
			}
		}
	}
}

bool FJointEditorToolkit::CanRenameNodes() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	const UEdGraphNode* SelectedNode = (SelectedNodes.Num() == 1)
		                                   ? Cast<UEdGraphNode>(*SelectedNodes.CreateConstIterator())
		                                   : nullptr;

	if (SelectedNode)
	{
		return SelectedNode->GetCanRenameNode();
	}

	return false;
}

void FJointEditorToolkit::OnCreateComment()
{
	TSharedPtr<SJointGraphEditor> GraphEditor = GraphEditorPtr;
	if (GraphEditor.IsValid())
	{
		if (UEdGraph* Graph = GraphEditor->GetCurrentGraph())
		{
			if (const UEdGraphSchema* Schema = Graph->GetSchema())
			{
				FJointSchemaAction_AddComment CommentAction;
				CommentAction.PerformAction(Graph, nullptr, GraphEditor->GetPasteLocation());
			}
		}
	}
}

void FJointEditorToolkit::OnCreateFoundation()
{
	TSharedPtr<SJointGraphEditor> GraphEditor = GraphEditorPtr;
	if (GraphEditor.IsValid())
	{
		if (UEdGraph* Graph = GraphEditor->GetCurrentGraph())
		{
			if (const UEdGraphSchema* Schema = Graph->GetSchema())
			{
				FJointSchemaAction_NewNode Action;

				Action.PerformAction_Command(Graph, UJointEdGraphNode_Foundation::StaticClass(),
				                             UJN_Foundation::StaticClass(), GraphEditor->GetPasteLocation());
			}
		}
	}
}

bool FJointEditorToolkit::CanJumpToSelection()
{
	const FGraphPanelSelectionSet& Selection = GetSelectedNodes();
	
	return !Selection.IsEmpty();
}

void FJointEditorToolkit::OnJumpToSelection()
{
	const FGraphPanelSelectionSet& Selection = GetSelectedNodes();
	
	for (UObject* Object : Selection)
	{
		if(!Object) continue;

		JumpToHyperlink(Object,false);

		return;
	}
}

void FJointEditorToolkit::ToggleShowNormalConnection()
{
	UJointEditorSettings::Get()->bDrawNormalConnection = !UJointEditorSettings::Get()->bDrawNormalConnection;
}

bool FJointEditorToolkit::IsShowNormalConnectionChecked() const
{
	return UJointEditorSettings::Get()->bDrawNormalConnection;
}

void FJointEditorToolkit::ToggleShowRecursiveConnection()
{
	UJointEditorSettings::Get()->bDrawRecursiveConnection = !UJointEditorSettings::Get()->
		bDrawRecursiveConnection;
}

bool FJointEditorToolkit::IsShowRecursiveConnectionChecked() const
{
	return UJointEditorSettings::Get()->bDrawRecursiveConnection;
}


void StoreNodeAndChildrenSubNodes(UObject* SelectedNode, TSet<UObject*>& NodesToCopy)
{
	UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedNode);

	if (Node == nullptr) return;

	NodesToCopy.Add(Node);

	UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node);

	if (CastedNode == nullptr) return;

	CastedNode->CachedParentGuidForCopyPaste = (CastedNode->ParentNode != nullptr)
		                                           ? CastedNode->ParentNode->NodeGuid
		                                           : CastedNode->CachedParentGuidForCopyPaste;

	//Copy all selected sub nodes.
	for (UJointEdGraphNode* SubNode : CastedNode->SubNodes)
	{
		if (SubNode == nullptr) continue;


		NodesToCopy.Add(SubNode);

		StoreNodeAndChildrenSubNodes(SubNode, NodesToCopy);
	}
}

void FJointEditorToolkit::CopySelectedNodes()
{
	//Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	//Target nodes to copy.
	TSet<UObject*> NodesToCopy;

	//Ensure to be clean
	NodesToCopy.Empty();

	for (UObject* SelectedNode : SelectedNodes)
	{
		if (SelectedNode == nullptr) continue;

		if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(SelectedNode))
		{
			if (!GraphNode->CanDuplicateNode()) continue;

			StoreNodeAndChildrenSubNodes(SelectedNode, NodesToCopy);
		}
	}

	for (UObject* NodeToCopy : NodesToCopy)
	{
		if (NodeToCopy == nullptr) continue;

		if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(NodeToCopy)) CastedNode->PrepareForCopying();
	}

	FString ExportedText;

	FEdGraphUtilities::ExportNodesToText(NodesToCopy, ExportedText);

	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	for (UObject* NodeToCopy : NodesToCopy)
	{
		if (NodeToCopy == nullptr) continue;

		if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(NodeToCopy)) CastedNode->PostCopyNode();
	}
}


void FJointEditorToolkit::SelectAllNodes()
{
	if (TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr)
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}

void FJointEditorToolkit::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();

	RequestManagerViewerRefresh();
}

void FJointEditorToolkit::PasteNodes()
{
	if (const TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr)
	{
		PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
	}

	RequestManagerViewerRefresh();
}

void FJointEditorToolkit::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();

	RequestManagerViewerRefresh();
}

bool WasPastedJointEdNodeSubNode(const UJointEdGraphNode* NodeToCheckOut)
{
	if (NodeToCheckOut == nullptr) return false;

	return NodeToCheckOut->CachedParentGuidForCopyPaste.IsValid();
}

void FJointEditorToolkit::PasteNodesHere(const FVector2D& Location)
{
	// Undo/Redo support
	TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr;

	if (!CurrentGraphEditor.IsValid()) return;

	const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());

	UJointEdGraph* CastedGraph = GetJointGraph();

	if (CastedGraph == nullptr) return;

	CastedGraph->Modify();

	//Lock the update to prevent unnecessary updates between the paste action.
	CastedGraph->LockUpdates();

	//Get the currently selected node on the graph and store it to use it when if we need to attach some nodes that will be imported right after this.
	//But if there is any node that can be standalone on the graph among the imported nodes, we will not implement those dependent nodes.
	//This is for the fragment copy paste between nodes.


	//Only can work with a single object. Not considering paste action for the multiple nodes at once, because it will be confusing and annoying sometimes (ex, Users didn't know that they selected unwanted node to paste to.)

	bool bHasMultipleNodesSelected = false;

	UJointEdGraphNode* AttachTargetNode = nullptr;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	for (UObject* SelectedNode : SelectedNodes)
	{
		if (SelectedNode == nullptr) continue;

		if (UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(SelectedNode))
		{
			if (AttachTargetNode == nullptr)
			{
				AttachTargetNode = Node;
			}
			else
			{
				bHasMultipleNodesSelected = true;
			}
		}
	}


	//Abort if we have multiple valid nodes.
	if (bHasMultipleNodesSelected)
	{
		return;
	}


	// Clear the selection set (newly pasted stuff will be selected)
	CurrentGraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;

	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;

	FEdGraphUtilities::ImportNodesFromText(CastedGraph, TextToImport, /*out*/ PastedNodes);

	//Reallocate graph nodes' outer to the Joint manager to prevent possible issues with ambiguous path search resolve
	//CastedGraph->ReallocateGraphNodesToJointManager();
	//CastedGraph->UpdateSubNodeChains();

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	// Number of nodes used to calculate AvgNodePosition
	int32 AvgCount = 0;

	//Calculate the average position on the graph to place our new nodes at.

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* EdNode = *It;

		if (EdNode == nullptr)
		{
			It.RemoveCurrent();

			continue;
		}

		//If it is UJointEdGraphNode type, then check whether it is a fragment to decide whether to add it on the avg point calculation.
		//If it was a graph node but not a UJointEdGraphNode type node, then just add it to the calculation.
		if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(EdNode); CastedNode
			                                                                     ? !WasPastedJointEdNodeSubNode(
				                                                                     CastedNode)
			                                                                     : true)
		{
			AvgNodePosition.X += EdNode->NodePosX;
			AvgNodePosition.Y += EdNode->NodePosY;
			++AvgCount;
		}
	}

	if (AvgCount > 0)
	{
		float InvNumNodes = 1.0f / static_cast<float>(AvgCount);
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}


	TMap<FGuid/*New*/, FGuid/*Old*/> NewToOldNodeMapping;

	TMap<FGuid, UJointEdGraphNode*> OldGuidToNewParentMap; //Node for the new parent.

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* EdNode = *It;

		if (EdNode == nullptr) continue;

		UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(EdNode);

		//If it was not a sub node, then implement it as a parent node.
		if (CastedNode ? !WasPastedJointEdNodeSubNode(CastedNode) : true)
		{
			EdNode->NodePosX = (EdNode->NodePosX - AvgNodePosition.X) + Location.X;
			EdNode->NodePosY = (EdNode->NodePosY - AvgNodePosition.Y) + Location.Y;

			EdNode->SnapToGrid(UJointEditorSettings::GetJointGridSnapSize());
		}

		const FGuid OldGuid = EdNode->NodeGuid;

		// Give new node a different Guid from the old one
		EdNode->CreateNewGuid();

		const FGuid NewGuid = EdNode->NodeGuid;

		NewToOldNodeMapping.Add(NewGuid, OldGuid);

		if (CastedNode)
		{
			//Clear Out the sub nodes. we are going to attach it right away after this.
			CastedNode->ParentNode = nullptr;
			CastedNode->SubNodes.Empty();
			//CastedNode->RemoveAllSubNodes();

			OldGuidToNewParentMap.Add(OldGuid, CastedNode);
		}
	}

	//Modify the object, so it can be stored in the transaction buffer.

	if (AttachTargetNode)
	{
		AttachTargetNode->Modify();

		if (AttachTargetNode->GetCastedNodeInstance()) AttachTargetNode->GetCastedNodeInstance()->Modify();
	}

	for (TTuple<FGuid, UJointEdGraphNode*> GuidToNewParentMap : OldGuidToNewParentMap)
	{
		if (!GuidToNewParentMap.Value) continue;

		GuidToNewParentMap.Value->Modify();

		if (!GuidToNewParentMap.Value->GetCastedNodeInstance()) continue;

		GuidToNewParentMap.Value->GetCastedNodeInstance()->Modify();
	}

	//Reattach sub nodes to the nodes.
	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* EdNode = *It;

		if (EdNode == nullptr) continue;

		UJointEdGraphNode* CastedPastedNode = Cast<UJointEdGraphNode>(EdNode);

		if (CastedPastedNode == nullptr) continue;

		//Execute only when this node is sub node.
		if (WasPastedJointEdNodeSubNode(CastedPastedNode))
		{
			CastedPastedNode->NodePosX = 0;
			CastedPastedNode->NodePosY = 0;

			//Remove sub node from graph, it will be referenced from parent node
			CastedPastedNode->DestroyNode();

			CastedPastedNode->ParentNode = OldGuidToNewParentMap.
				FindRef(CastedPastedNode->CachedParentGuidForCopyPaste);

			if (CastedPastedNode->ParentNode)
			{
				CastedPastedNode->ParentNode->AddSubNode(CastedPastedNode);
			}
			else if (!bHasMultipleNodesSelected && AttachTargetNode)
			{
				CastedPastedNode->ParentNode = AttachTargetNode;
				AttachTargetNode->AddSubNode(CastedPastedNode);
			}
		}
	}

	FixupPastedNodes(PastedNodes, NewToOldNodeMapping);

	if (CastedGraph)
	{
		CastedGraph->UpdateClassData();
		CastedGraph->OnNodesPasted(TextToImport);
		CastedGraph->UnlockUpdates();
		CastedGraph->NotifyGraphChanged();
	}

	// Update UI
	CurrentGraphEditor->NotifyGraphChanged();

	UObject* GraphOwner = CastedGraph->GetOuter();
	if (GraphOwner)
	{
		GraphOwner->PostEditChange();
		GraphOwner->MarkPackageDirty();
	}


	TSet<class UObject*> Selection;

	// Select the newly pasted stuff
	for (UEdGraphNode* PastedNode : PastedNodes)
	{
		if (!PastedNode) continue;

		Selection.Add(PastedNode);
	}

	SelectProvidedObjectOnGraph(Selection);
}

void FJointEditorToolkit::OnGraphSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	if (GetNodePickingManager().IsValid() && GetNodePickingManager()->IsInNodePicking())
	{
		for (UObject* Selection : NewSelection)
		{
			if (Selection == nullptr) continue;

			UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(Selection);

			if (CastedGraphNode == nullptr) continue;

			GetNodePickingManager()->PerformNodePicking(CastedGraphNode->GetCastedNodeInstance(), CastedGraphNode);

			break;
		}
	}
	else
	{
		SelectProvidedObjectOnDetail(NewSelection);
		NotifySelectionChangeToNodeSlates(NewSelection);
	}
}

void FJointEditorToolkit::SelectProvidedObjectOnDetail(const TSet<UObject*>& NewSelection)
{
	TArray<UObject*> Selection = NewSelection.Array();

	if (DetailsViewPtr.IsValid())
	{
		if (Selection.Num() != 0)
		{
			DetailsViewPtr->SetObjects(Selection);
		}
		else
		{
			DetailsViewPtr->SetObject(GetJointManager());
		}
	}
}


void FJointEditorToolkit::SelectProvidedObjectOnGraph(TSet<UObject*> NewSelection)
{
	TArray<UObject*> Selection = NewSelection.Array();

	if (GraphEditorPtr.IsValid() && GraphEditorPtr->GetGraphPanel() != nullptr)
	{
		GraphEditorPtr->GetGraphPanel()->SelectionManager.SetSelectionSet(NewSelection);
	}
}


void FJointEditorToolkit::OnEnableBreakpoint()
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TArray<FJointNodeDebugData>* DebugData =
		UJointDebugger::GetDebugDataFromJointManager(GetJointManager());

	if (DebugData != nullptr)
	{
		for (FJointNodeDebugData& Data : *DebugData)
		{
			if (UObject* GraphNode = *SelectedNodes.Find(Data.Node))
			{
				if (UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(GraphNode))
				{
					if (Data.bHasBreakpoint == true)
					{
						Data.bIsBreakpointEnabled = true;
						Data.Node->NotifyDebugDataChangedToGraphNodeWidget(&Data);
					}
				}
			}
		}
	}

	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}

bool FJointEditorToolkit::CanEnableBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TArray<FJointNodeDebugData>* DebugData =
		UJointDebugger::GetDebugDataFromJointManager(GetJointManager());

	if (DebugData != nullptr)
	{
		for (FJointNodeDebugData& Data : *DebugData)
		{
			if (SelectedNodes.Contains(Data.Node) && Data.bHasBreakpoint == true && Data.bIsBreakpointEnabled == false)
				return true;
		}
	}

	return false;
}

bool FJointEditorToolkit::CanDisableBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TArray<FJointNodeDebugData>* DebugData =
		UJointDebugger::GetDebugDataFromJointManager(GetJointManager());

	if (DebugData != nullptr)
	{
		for (FJointNodeDebugData& Data : *DebugData)
		{
			if (SelectedNodes.Contains(Data.Node) && Data.bHasBreakpoint == true && Data.bIsBreakpointEnabled == true)
				return true;
		}
	}
	return false;
}

void FJointEditorToolkit::OnDisableBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TArray<FJointNodeDebugData>* DebugData =
		UJointDebugger::GetDebugDataFromJointManager(GetJointManager());

	if (DebugData != nullptr)
	{
		for (FJointNodeDebugData& Data : *DebugData)
		{
			if (Data.bHasBreakpoint == true)
			{
				Data.bIsBreakpointEnabled = false;
				Data.Node->NotifyDebugDataChangedToGraphNodeWidget(&Data);
			}
		}
	}

	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}


void FJointEditorToolkit::OnAddBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TArray<FJointNodeDebugData>* DebugDataArr = UJointDebugger::GetDebugDataFromJointManager(
		GetJointManager());

	//Revert
	if (DebugDataArr == nullptr) return;

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);

		if (SelectedNode)
		{
			FJointNodeDebugData* Data = UJointDebugger::GetDebugDataFor(SelectedNode);

			if (Data != nullptr)
			{
				Data->bHasBreakpoint = true;
				Data->bIsBreakpointEnabled = false;
				Data->Node->NotifyDebugDataChangedToGraphNodeWidget(Data);
			}
			else
			{
				FJointNodeDebugData NewDebugData;

				NewDebugData.Node = SelectedNode;
				NewDebugData.bHasBreakpoint = true;
				NewDebugData.bIsBreakpointEnabled = true;

				DebugDataArr->Add(NewDebugData);
				NewDebugData.Node->NotifyDebugDataChangedToGraphNodeWidget(Data);
			}
		}
	}

	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}

bool FJointEditorToolkit::CanAddBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);

		if (SelectedNode)
		{
			if (!SelectedNode->CanHaveBreakpoint()) return false;

			FJointNodeDebugData* Data = UJointDebugger::GetDebugDataFor(SelectedNode);

			if (Data == nullptr) return true;
			if (Data->bHasBreakpoint == false) return true;
		}
	}

	return false;
}

void FJointEditorToolkit::OnRemoveBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TArray<FJointNodeDebugData>* DebugDataArr = UJointDebugger::GetDebugDataFromJointManager(
		GetJointManager());

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);

		if (SelectedNode)
		{
			FJointNodeDebugData* Data = UJointDebugger::GetDebugDataFor(SelectedNode);

			if (Data != nullptr)
			{
				Data->bHasBreakpoint = false;
				Data->bIsBreakpointEnabled = false;
				Data->Node->NotifyDebugDataChangedToGraphNodeWidget(Data);
			}
		}
	}

	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}

bool FJointEditorToolkit::CanRemoveBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);

		if (SelectedNode)
		{
			FJointNodeDebugData* Data = UJointDebugger::GetDebugDataFor(SelectedNode);

			if (Data != nullptr)
			{
				if (Data->bHasBreakpoint == true) return true;
			}
		}
	}

	return false;
}

void FJointEditorToolkit::OnRemoveAllBreakpoints()
{
	if (UJointEdGraph* CastedGraph = GetJointGraph())
	{
		for (FJointNodeDebugData& DebugData : CastedGraph->DebugData)
		{
			DebugData.bHasBreakpoint = false;
			DebugData.Node->NotifyDebugDataChangedToGraphNodeWidget(&DebugData);
		}
	}
}

bool FJointEditorToolkit::CanRemoveAllBreakpoints() const
{
	if (UJointEdGraph* CastedGraph = GetJointGraph())
	{
		return !CastedGraph->DebugData.IsEmpty();
	}

	return false;
}

void FJointEditorToolkit::OnEnableAllBreakpoints()
{
	if (UJointEdGraph* CastedGraph = GetJointGraph())
	{
		for (FJointNodeDebugData& DebugData : CastedGraph->DebugData)
		{
			DebugData.bIsBreakpointEnabled = true;
			DebugData.Node->NotifyDebugDataChangedToGraphNodeWidget(&DebugData);
		}
	}
}

bool FJointEditorToolkit::CanEnableAllBreakpoints() const
{
	if (UJointEdGraph* CastedGraph = GetJointGraph())
	{
		for (const FJointNodeDebugData& DebugData : CastedGraph->DebugData)
		{
			if (!DebugData.bIsBreakpointEnabled) return true;
		}

		return false;
	}

	return false;
}

void FJointEditorToolkit::OnDisableAllBreakpoints()
{
	if (UJointEdGraph* CastedGraph = GetJointGraph())
	{
		for (FJointNodeDebugData& DebugData : CastedGraph->DebugData)
		{
			DebugData.bIsBreakpointEnabled = false;
			DebugData.Node->NotifyDebugDataChangedToGraphNodeWidget(&DebugData);
		}
	}
}

bool FJointEditorToolkit::CanDisableAllBreakpoints() const
{
	if (UJointEdGraph* CastedGraph = GetJointGraph())
	{
		for (const FJointNodeDebugData& DebugData : CastedGraph->DebugData)
		{
			if (DebugData.bIsBreakpointEnabled) return true;
		}

		return false;
	}

	return false;
}

void FJointEditorToolkit::OnToggleDebuggerExecution()
{
	UJointEditorSettings::Get()->bDebuggerEnabled = !UJointEditorSettings::Get()->bDebuggerEnabled;
}

bool FJointEditorToolkit::GetCheckedToggleDebuggerExecution() const
{
	return UJointEditorSettings::Get()->bDebuggerEnabled;
}

void FJointEditorToolkit::OnToggleVisibilityChangeModeForSimpleDisplayProperty()
{
	bIsOnVisibilityChangeModeForSimpleDisplayProperty = !bIsOnVisibilityChangeModeForSimpleDisplayProperty;

	if (bIsOnVisibilityChangeModeForSimpleDisplayProperty)
	{
		PopulateVisibilityChangeModeForSimpleDisplayPropertyToastMessage();
	}
	else
	{
		ClearVisibilityChangeModeForSimpleDisplayPropertyToastMessage();
	}
}

bool FJointEditorToolkit::GetCheckedToggleVisibilityChangeModeForSimpleDisplayProperty() const
{
	return bIsOnVisibilityChangeModeForSimpleDisplayProperty;
}

void FJointEditorToolkit::OnToggleBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	if (UJointManager* OriginalJointManager = UJointDebugger::GetOriginalJointManager(GetJointManager()))
	{
		TArray<FJointNodeDebugData>* DebugData = UJointDebugger::GetDebugDataFromJointManager(OriginalJointManager);

		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);

			if (FJointNodeDebugData* Data = UJointDebugger::GetDebugDataFor(SelectedNode); Data != nullptr)
			{
				if (Data->bHasBreakpoint) //do disable
				{
					Data->bHasBreakpoint = false;
					Data->bIsBreakpointEnabled = false;
					Data->Node->NotifyDebugDataChangedToGraphNodeWidget(Data);
				}
				else //do enable
				{
					Data->bHasBreakpoint = true;
					Data->bIsBreakpointEnabled = true;
					Data->Node->NotifyDebugDataChangedToGraphNodeWidget(Data);
				}
			}
			else
			{
				//enable. add a new one for this.
				FJointNodeDebugData NewDebugData;

				NewDebugData.Node = UJointDebugger::GetOriginalJointGraphNodeFromJointGraphNode(SelectedNode);
				NewDebugData.bHasBreakpoint = true;
				NewDebugData.bIsBreakpointEnabled = true;

				DebugData->Add(NewDebugData);

				NewDebugData.Node->NotifyDebugDataChangedToGraphNodeWidget(&NewDebugData);
			}
		}

		UJointDebugger::NotifyDebugDataChanged(OriginalJointManager);
	}
}

bool FJointEditorToolkit::CanToggleBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);
		if (SelectedNode)
		{
			if (!SelectedNode->CanHaveBreakpoint()) return false;

			return true;
		}
	}

	return false;
}


void FJointEditorToolkit::NotifySelectionChangeToNodeSlates(const TSet<class UObject*>& NewSelection) const
{
	if (JointManager == nullptr) return;

	if (UJointEdGraph* CastedGraph = GetJointGraph())
	{
		TSet<TSoftObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCacheJointGraphNodes();

		for (TSoftObjectPtr<UJointEdGraphNode> GraphNode : GraphNodes)
		{
			if (!GraphNode.IsValid()) continue;

			const TSharedPtr<SJointGraphNodeBase> NodeSlate = GraphNode->GetGraphNodeSlate().Pin();

			if (!NodeSlate.IsValid()) continue;

			NodeSlate->OnGraphSelectionChanged(NewSelection);
		}
	}

	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}

/*
	
void FJointEditorToolkit::ClearSelectionOnGraph()
{
	TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr;
	
	if (!CurrentGraphEditor.IsValid()) return;
	
	CurrentGraphEditor->ClearSelectionSet();
}

void FJointEditorToolkit::ChangeSelectionOfNodeOnGraph(UEdGraphNode* Node, bool bSelected)
{
	TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr;
	
	if (!CurrentGraphEditor.IsValid()) return;
	
	if(!Node) return;

	CurrentGraphEditor->SetNodeSelection(Node, bSelected);
}

void FJointEditorToolkit::ChangeSelectionsOfNodeOnGraph(TArray<UEdGraphNode*> Nodes, bool bSelected)
{
	TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GraphEditorPtr;
	
	if (!CurrentGraphEditor.IsValid()) return;

	for (UEdGraphNode* Node : Nodes) {

		if(!Node) continue;
		
		CurrentGraphEditor->SetNodeSelection(Node, bSelected);
		
	}	
}
*/

void FJointEditorToolkit::OnDebuggingJointInstanceChanged(TWeakObjectPtr<AJointActor> WeakObject)
{
}

void FJointEditorToolkit::SetDebuggingJointInstance(TWeakObjectPtr<AJointActor> InDebuggingJointInstance)
{
	if (InDebuggingJointInstance == nullptr) return;

	DebuggingJointInstance = InDebuggingJointInstance;

	OnDebuggingJointInstanceChanged(InDebuggingJointInstance);
}

TWeakObjectPtr<AJointActor> FJointEditorToolkit::GetDebuggingJointInstance()
{
	return DebuggingJointInstance;
}

void FJointEditorToolkit::HandleNewAssetActionClassPicked(UClass* InClass) const
{
	if (InClass == nullptr) return;

	FString ClassName = FBlueprintEditorUtils::GetClassNameWithoutSuffix(InClass);

	FString PathName;

	if (GetJointManager() != nullptr)
	{
		PathName = GetJointManager()->GetOutermost()->GetPathName();
		PathName = FPaths::GetPath(PathName);
		PathName /= ClassName;
	}
	else
	{
		PathName = FPaths::ProjectContentDir();
		PathName = FPaths::GetPath(PathName);
		PathName /= ClassName;
	}

	FString Name;
	FString PackageName;
	const FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(PathName, TEXT("_New"), PackageName, Name);

	UPackage* Package = CreatePackage(*PackageName);
	if (ensure(Package))
	{
		// Create and init a new Blueprint
		if (UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(InClass
		                                                                , Package
		                                                                , FName(*Name)
		                                                                , BPTYPE_Normal
		                                                                , UBlueprint::StaticClass()
		                                                                , UBlueprintGeneratedClass::StaticClass()))
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(NewBP);

			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(NewBP);

			// Mark the package dirty...
			Package->MarkPackageDirty();
		}
	}


	FSlateApplication::Get().DismissAllMenus();
}

void FJointEditorToolkit::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo,
                                               UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		static const FText TranscationTitle = FText::FromString(FString(TEXT("Rename Node")));
		const FScopedTransaction Transaction(TranscationTitle);
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

bool FJointEditorToolkit::OnVerifyNodeTitleChanged(const FText& InText, UEdGraphNode* NodeBeingEdited,
                                                   FText& OutErrorMessage)
{
	if (NodeBeingEdited)
	{
		if (Cast<UJointEdGraphNode>(NodeBeingEdited))
		{
			return FName(InText.ToString()).IsValidObjectName(OutErrorMessage);
		}
		else
		{
			return true;
		}
	}

	return false;
}

void FJointEditorToolkit::OnDebuggerActorSelected(TWeakObjectPtr<AJointActor> InstanceToDebug)
{
	//Feed it to the debugger.
	if (InstanceToDebug != nullptr)
	{
		FJointEditorToolkit* Toolkit = nullptr;

		UJointDebugger::Get()->AssignInstanceToLookUp(InstanceToDebug.Get());

		FindOrOpenEditorInstanceFor(InstanceToDebug->GetJointManager(), true);
	}
}

FJointEditorToolkit* FJointEditorToolkit::FindOrOpenEditorInstanceFor(UObject* ObjectRelatedTo,
                                                                      const bool& bOpenIfNotPresent)
{

	if (ObjectRelatedTo != nullptr)
	{
		if (UJointManager* Manager = Cast<UJointManager>(ObjectRelatedTo))
		{
			IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->
				FindEditorForAsset(Manager, true);
			
			if(EditorInstance != nullptr) return static_cast<FJointEditorToolkit*>(EditorInstance);

			if(bOpenIfNotPresent)
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Manager);

				EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Manager, true);
			
				return (EditorInstance != nullptr) ? static_cast<FJointEditorToolkit*>(EditorInstance) : nullptr;
			}

		}
		
		if (UJointEdGraph* Graph = Cast<UJointEdGraph>(ObjectRelatedTo))
		{
			if (UJointManager* FoundManager = Graph->GetJointManager())
			{
				IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->
					FindEditorForAsset(FoundManager, true);

				if(EditorInstance != nullptr) return static_cast<FJointEditorToolkit*>(EditorInstance);

				if(bOpenIfNotPresent)
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(FoundManager);

					EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(FoundManager, true);
			
					return (EditorInstance != nullptr) ? static_cast<FJointEditorToolkit*>(EditorInstance) : nullptr;
				}
			}
		}
		if (UJointEdGraphNode* EdNode = Cast<UJointEdGraphNode>(ObjectRelatedTo))
		{
			if (UJointManager* FoundManager = EdNode->GetJointManager())
			{
				IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->
					FindEditorForAsset(FoundManager, true);

				if(EditorInstance != nullptr) return static_cast<FJointEditorToolkit*>(EditorInstance);

				if(bOpenIfNotPresent)
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(FoundManager);

					EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(FoundManager, true);
			
					return (EditorInstance != nullptr) ? static_cast<FJointEditorToolkit*>(EditorInstance) : nullptr;
				}
			}
			
		}
		if (UJointNodeBase* NodeBase = Cast<UJointNodeBase>(ObjectRelatedTo))
		{
			if (UJointManager* FoundManager = NodeBase->GetJointManager())
			{
				IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->
					FindEditorForAsset(FoundManager, true);

				if(EditorInstance != nullptr) return static_cast<FJointEditorToolkit*>(EditorInstance);

				if(bOpenIfNotPresent)
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(FoundManager);

					EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(FoundManager, true);
			
					return (EditorInstance != nullptr) ? static_cast<FJointEditorToolkit*>(EditorInstance) : nullptr;
				}
			}
		}
	}

	return nullptr;
}


TSharedRef<class SWidget> FJointEditorToolkit::OnGetDebuggerActorsMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	if (UJointDebugger* DebuggerSingleton = UJointDebugger::Get())
	{
		TArray<AJointActor*> MatchingInstances;

		//If it already has the debugging asset, then use that instance to getter the Joint manager because the toolkit's GetJointManager() will return the instance of the duplicated, transient version of it.
		DebuggerSingleton->GetMatchingInstances(GetDebuggingJointInstance().IsValid()
			                                        ? GetDebuggingJointInstance().Get()->OriginalJointManager
			                                        : GetJointManager(), MatchingInstances);

		// Fill the combo menu with presets of common screen resolutions

		for (AJointActor* MatchingInstance : MatchingInstances)
		{
			if (MatchingInstance == nullptr) continue;

			const FText InstanceDescription = DebuggerSingleton->GetInstanceDescription(MatchingInstance);

			TWeakObjectPtr<AJointActor> InstancePtr = MatchingInstance;

			FUIAction ItemAction(
				FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnDebuggerActorSelected, InstancePtr));
			MenuBuilder.AddMenuEntry(
				InstanceDescription,
				TAttribute<FText>(),
				FSlateIcon(),
				ItemAction);
		}


		// Failsafe when no actor match
		if (MatchingInstances.IsEmpty())
		{
			const FText ActorDesc = LOCTEXT("NoMatchForDebug", "Can't find matching actors");
			TWeakObjectPtr<AJointActor> InstancePtr;

			FUIAction ItemAction(
				FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnDebuggerActorSelected, InstancePtr));
			MenuBuilder.AddMenuEntry(ActorDesc, TAttribute<FText>(), FSlateIcon(), ItemAction);
		}
	}

	return MenuBuilder.MakeWidget();
}

FText FJointEditorToolkit::GetDebuggerActorDesc() const
{
	return UJointDebugger::Get() != nullptr
		       ? UJointDebugger::Get()->GetInstanceDescription(DebuggingJointInstance.Get())
		       : FText::GetEmpty();
}


#undef LOCTEXT_NAMESPACE
