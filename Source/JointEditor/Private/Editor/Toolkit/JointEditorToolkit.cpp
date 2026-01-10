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

#include "HAL/PlatformApplicationMisc.h"

#include "JointEdGraphNode_Composite.h"
#include "JointEdGraphNode_Foundation.h"
#include "JointEdGraphNode_Fragment.h"
#include "JointEditorGraphDocument.h"
#include "JointEditorNameValidator.h"
#include "JointEditorNodePickingManager.h"
#include "JointEdUtils.h"

#include "EditorWidget/JointToolkitToastMessages.h"
#include "EditorWidget/SJointEditorOutliner.h"
#include "EditorWidget/SJointFragmentPalette.h"
#include "EditorWidget/SJointGraphPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Node/JointFragment.h"
#include "Node/Derived/JN_Foundation.h"
#include "Widgets/Images/SImage.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

#define LOCTEXT_NAMESPACE "JointEditorToolkit"

DEFINE_LOG_CATEGORY_STATIC(LogJointEditorToolkit, Log, All);

namespace FJointToolkitInlineUtils
{
	void StartModifyingGraphs(const TArray<UJointEdGraph*>& Graphs)
	{
		for (UJointEdGraph* Graph : Graphs)
		{
			if (!Graph) return;
			Graph->Modify();
			Graph->LockUpdates();
		}
	}

	void EndModifyingGraphs(const TArray<UJointEdGraph*>& Graphs)
	{
		for (UJointEdGraph* Graph : Graphs)
		{
			if (!Graph) return;
			Graph->UnlockUpdates();
			Graph->NotifyGraphChanged();
		}
	}	
};


FJointEditorToolkit::FJointEditorToolkit()
{
	FEditorDelegates::BeginPIE.AddRaw(this, &FJointEditorToolkit::OnBeginPIE);
	FEditorDelegates::EndPIE.AddRaw(this, &FJointEditorToolkit::OnEndPIE);

	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor)
	{
		Editor->RegisterForUndo(this);
	}
	
}

FJointEditorToolkit::~FJointEditorToolkit()
{
	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);

	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());
	
	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor)
	{
		Editor->UnregisterForUndo(this);
	}
}

void FJointEditorToolkit::InitJointEditor(const EToolkitMode::Type Mode,
                                          const TSharedPtr<class IToolkitHost>& InitToolkitHost,
                                          UJointManager* InJointManager)
{
	JointManager = InJointManager;

	//StandaloneMode = MakeShareable(new FJointEditorApplicationMode(SharedThis(this)));
	//StandaloneMode->Initialize();
	//AddApplicationMode(EJointEditorModes::StandaloneMode, StandaloneMode.ToSharedRef());

	CreateNewRootGraphForJointManagerIfNeeded();

	GetOrCreateGraphToastMessageHub();
	GetOrCreateNodePickingManager();

	BindGraphEditorCommands();
	BindDebuggerCommands();

	InitializeDocumentManager();

	//Initialize editor slates.

	InitializeDetailView();
	InitializePaletteView();
	InitializeOutliner();
	InitializeContentBrowser();
	InitializeEditorPreferenceView();
	InitializeManagerViewer();
	InitializeCompileResult();

	TSharedPtr<FTabManager::FLayout> TabLayout = FTabManager::NewLayout("Standalone_JointEditor_Layout_v9")
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
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->SetSizeCoefficient(0.75f)
				->Split(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(EJointEditorTapIDs::SearchReplaceID, ETabState::ClosedTab)
					// Provide the location of DocumentManager, chart
				)
				->Split(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.4)
					->Split(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.75f)
						->AddTab(EJointEditorTapIDs::GraphID, ETabState::ClosedTab)
					)
					->Split(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.25f)
						->AddTab(EJointEditorTapIDs::CompileResultID, ETabState::ClosedTab)
					)
				)
				->Split(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.25)
					->Split(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.75f)
						->AddTab(EJointEditorTapIDs::DetailsID, ETabState::OpenedTab)
					)
					->Split(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.2f)
						->AddTab(EJointEditorTapIDs::OutlinerID, ETabState::OpenedTab)
					)
					->Split(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.25f)
						->AddTab(EJointEditorTapIDs::PaletteID, ETabState::OpenedTab)
						->AddTab(EJointEditorTapIDs::ContentBrowserID, ETabState::OpenedTab)
						->AddTab(EJointEditorTapIDs::EditorPreferenceID, ETabState::OpenedTab)
						->SetForegroundTab(EJointEditorTapIDs::PaletteID)
					)

				)
			)

		);

	InitAssetEditor(
		Mode,
		InitToolkitHost,
		EJointEditorTapIDs::AppIdentifier,
		TabLayout.ToSharedRef(),
		true,
		true,
		JointManager.Get());


	RestoreEditedObjectState();

	ExtendToolbar();
	RegenerateMenusAndToolbars();

	//SetCurrentMode(EJointEditorModes::StandaloneMode);

	if (GetJointManager() != nullptr && !GetJointManager()->IsAsset())
	{
		PopulateTransientEditingWarningToastMessage();
	}
	// Create a command, pay attention to setcurrentmode ()
}


void FJointEditorToolkit::CleanUp()
{
	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		Graph->OnClosed();
	};

	JointManager.Reset();

	EditorPreferenceViewPtr.Reset();
	ContentBrowserPtr.Reset();
	DetailsViewPtr.Reset();
	ManagerViewerPtr.Reset();
	JointFragmentPalettePtr.Reset();
	Toolbar.Reset();

	CleanUpGraphToastMessageHub();
}

void FJointEditorToolkit::OnClose()
{
	//Clean Up;

	SaveEditedObjectState();

	CleanUp();

	FWorkflowCentricApplication::OnClose();
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

void FJointEditorToolkit::InitializeOutliner()
{
	JointEditorOutlinerPtr = SNew(SJointEditorOutliner)
		.ToolKitPtr(SharedThis(this));
}

void FJointEditorToolkit::InitializeManagerViewer()
{
	ManagerViewerPtr = SNew(SJointManagerViewer)
		.ToolKitPtr(SharedThis(this))
		.JointManagers({GetJointManager()})
		.ShowOnlyCurrentGraphToggleButtonVisibility(EVisibility::Visible);
}

TSharedRef<SDockTab> FJointEditorToolkit::SpawnTab_EditorPreference(const FSpawnTabArgs& Args,
                                                                    FName TabIdentifier) const
{
	TSharedRef<SDockTab> EditorPreferenceTabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointEditorPreferenceTabTitle", "Joint Editor Preference"));

	TSharedPtr<SWidget> TabWidget = SNullWidget::NullWidget;

	if (TabIdentifier == EJointEditorTapIDs::EditorPreferenceID && EditorPreferenceViewPtr.IsValid())
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

	if (TabIdentifier == EJointEditorTapIDs::DetailsID && DetailsViewPtr.IsValid())
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

	if (TabIdentifier == EJointEditorTapIDs::PaletteID && JointFragmentPalettePtr.IsValid())
	{
		JointFragmentPaletteTabPtr->SetContent(JointFragmentPalettePtr.ToSharedRef());
	}

	return JointFragmentPaletteTabPtr;
}

TSharedRef<SDockTab> FJointEditorToolkit::SpawnTab_Outliner(const FSpawnTabArgs& Args, FName TabIdentifier)
{
	TSharedRef<SDockTab> JointEditorOutlinerTabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointEditorOutlinerTabTitle", "Outliner"));

	if (TabIdentifier == EJointEditorTapIDs::OutlinerID && JointEditorOutlinerPtr.IsValid())
	{
		JointEditorOutlinerTabPtr->SetContent(JointEditorOutlinerPtr.ToSharedRef());
	}

	return JointEditorOutlinerTabPtr;
}


void FJointEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	//@TODO: Can't we do this sooner?

	if (DocumentManager) DocumentManager->SetTabManager(GetTabManager().ToSharedRef());


	const auto& LocalCategories = InTabManager->GetLocalWorkspaceMenuRoot()->GetChildItems();

	if (LocalCategories.Num() <= 0)
	{
		TSharedRef<FWorkspaceItem> Group = FWorkspaceItem::NewGroup(LOCTEXT("JointTabGroupName", "Joint Editor"));
		InTabManager->GetLocalWorkspaceMenuRoot()->AddItem(Group);
	}

	AssetEditorTabsCategory = LocalCategories.Num() > 0
		                          ? LocalCategories[0]
		                          : InTabManager->GetLocalWorkspaceMenuRoot();

	InTabManager->RegisterTabSpawner(EJointEditorTapIDs::EditorPreferenceID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_EditorPreference,
	                                                       EJointEditorTapIDs::EditorPreferenceID))
	            .SetDisplayName(LOCTEXT("EditorPreferenceTabLabel", "Joint Editor Preference"))
	            .SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Details"))
	            .SetGroup(AssetEditorTabsCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner(EJointEditorTapIDs::ContentBrowserID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_ContentBrowser,
	                                                       EJointEditorTapIDs::ContentBrowserID))
	            .SetDisplayName(LOCTEXT("ContentBrowserTabLabel", "Joint Browser"))
	            .SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Viewports"))
	            .SetGroup(AssetEditorTabsCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner(EJointEditorTapIDs::PaletteID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_Palettes,
	                                                       EJointEditorTapIDs::PaletteID))
	            .SetDisplayName(LOCTEXT("PaletteTabLabel", "Fragment Palette"))
	            .SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.MeshPaintMode"))
	            .SetGroup(AssetEditorTabsCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner(EJointEditorTapIDs::DetailsID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_Details,
	                                                       EJointEditorTapIDs::DetailsID))
	            .SetDisplayName(LOCTEXT("DetailsTabLabel", "Details"))
	            .SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Details"))
	            .SetGroup(AssetEditorTabsCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(EJointEditorTapIDs::SearchReplaceID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_Search,
	                                                       EJointEditorTapIDs::SearchReplaceID))
	            .SetDisplayName(LOCTEXT("SearchTabLabel", "Search"))
	            .SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Outliner"))
	            .SetGroup(AssetEditorTabsCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(EJointEditorTapIDs::OutlinerID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_Outliner,
	                                                       EJointEditorTapIDs::OutlinerID))
	            .SetDisplayName(LOCTEXT("OutlinerTabLabel", "Outliner"))
	            .SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "LevelEditor.Tabs.Outliner"))
	            .SetGroup(AssetEditorTabsCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner(EJointEditorTapIDs::CompileResultID,
	                                 FOnSpawnTab::CreateSP(this, &FJointEditorToolkit::SpawnTab_CompileResult,
	                                                       EJointEditorTapIDs::CompileResultID))
	            .SetDisplayName(LOCTEXT("CompileResultTabLabel", "Compile Result"))
	            .SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "Kismet.Tabs.CompilerResults"))
	            .SetGroup(AssetEditorTabsCategory.ToSharedRef());

	FWorkflowCentricApplication::RegisterTabSpawners(InTabManager);
}



void FJointEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(EJointEditorTapIDs::ContentBrowserID);
	InTabManager->UnregisterTabSpawner(EJointEditorTapIDs::EditorPreferenceID);
	InTabManager->UnregisterTabSpawner(EJointEditorTapIDs::PaletteID);
	InTabManager->UnregisterTabSpawner(EJointEditorTapIDs::OutlinerID);
	InTabManager->UnregisterTabSpawner(EJointEditorTapIDs::DetailsID);
	InTabManager->UnregisterTabSpawner(EJointEditorTapIDs::SearchReplaceID);
	InTabManager->UnregisterTabSpawner(EJointEditorTapIDs::CompileResultID);
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

void FJointEditorToolkit::InitializeDocumentManager()
{
	TSharedPtr<FJointEditorToolkit> ThisPtr(SharedThis(this));

	// Here we create the document manager for this editor instance
#if UE_VERSION_OLDER_THAN(5, 5, 0)
	DocumentManager = MakeShareable(new FDocumentTracker());
#else
	DocumentManager = MakeShareable(new FDocumentTracker());
#endif

	// @todo TabManagement
	DocumentManager->Initialize(ThisPtr);

	// // Register the document factories
	{
		TSharedRef<FDocumentTabFactory> GraphEditorFactory = MakeShareable(new FJointGraphEditorSummoner(
			ThisPtr,
			FJointGraphEditorSummoner::FOnCreateGraphEditorWidget::CreateSP(this, &FJointEditorToolkit::CreateGraphEditorWidget)));
		GraphEditorTabFactoryPtr = GraphEditorFactory;

		// Also store off a reference to the grapheditor factory so we can find all the tabs spawned by it later.
		DocumentManager->RegisterDocumentFactory(GraphEditorFactory);
	}
}

TSharedRef<SGraphEditor> FJointEditorToolkit::CreateGraphEditorWidget(TSharedRef<FTabInfo> TabInfo, UJointEdGraph* EdGraph)
{
	SJointGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FJointEditorToolkit::OnGraphEditorNodeTitleCommitted);
	InEvents.OnSelectionChanged = SJointGraphEditor::FOnSelectionChanged::CreateLambda( // for the graph editor selection change event - we need to wrap it with a lambda to pass the EdGraph ptr
		[this, EdGraph](const TSet<UObject*>& NewSelection)
		{
			this->OnGraphSelectedNodesChanged(EdGraph, NewSelection);
		});

	//Feed the toolkit to the graph.
	if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(EdGraph))
	{
		CastedGraph->SetToolkit(SharedThis(this));
	}

	TSharedRef<SJointGraphEditor> GraphEditorPtr = SNew(SJointGraphEditor)
		.TitleBar(this->GetGraphToastMessageHub())
		.AdditionalCommands(this->GetToolkitCommands())
		.IsEditable(this->IsInEditingMode()) // this is where we decide if the graph is editable or not - this can be used to make read only graphs...
		.GraphToEdit(EdGraph)
		.GraphEvents(InEvents);

	if (GraphEditorPtr->GetGraphPanel())
	{
		SJointGraphPanel* GraphPanelPtr = static_cast<SJointGraphPanel*>(GraphEditorPtr->GetGraphPanel());
		//GraphPanelPtr->SetAllowContinousZoomInterpolation(true); // buggy
	}

	return GraphEditorPtr;
}

FGraphPanelSelectionSet FJointEditorToolkit::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;

	if (GetFocusedGraphEditor().IsValid())
	{
		CurrentSelection = GetFocusedGraphEditor()->GetSelectedNodes();
	}

	return CurrentSelection;
}


TSharedPtr<class SJointToolkitToastMessageHub> FJointEditorToolkit::GetGraphToastMessageHub() const
{
	return GraphToastMessageHub;
}


void FJointEditorToolkit::CleanUpGraphToastMessageHub()
{
	if (GraphToastMessageHub)
	{
		GraphToastMessageHub->ReleaseVoltAnimationManager();
		GraphToastMessageHub.Reset();
	}
}

#include "IPropertyTable.h"

void FJointEditorToolkit::InitializeCompileResult()
{
	UJointEdGraph* MainGraph = GetMainJointGraph();
	
	if (MainGraph)
	{
		MainGraph->InitializeCompileResultIfNeeded();

		if (MainGraph->CompileResultPtr.IsValid())
		{
			MainGraph->OnCompileFinished.Unbind();
			MainGraph->CompileResultPtr->OnMessageTokenClicked().Clear();
			
			MainGraph->OnCompileFinished.BindSP(this, &FJointEditorToolkit::OnCompileJointGraphFinished);
			MainGraph->CompileResultPtr->OnMessageTokenClicked().AddSP(this, &FJointEditorToolkit::OnCompileResultTokenClicked);
		}	
	}

	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());
	
	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		Graph->InitializeCompileResultIfNeeded();
	}

	if (MainGraph)
	{
		MainGraph->CompileAllJointGraphFromRoot();
	}
}

void FJointEditorToolkit::FeedEditorSlateToEachTab() const
{
	struct FTabWidgetPair
	{
		FName TabId;
		TSharedPtr<SWidget> WidgetPtr;
	};

	TArray<FTabWidgetPair> TabWidgetPairs = {
		{EJointEditorTapIDs::DetailsID, DetailsViewPtr},
		{EJointEditorTapIDs::SearchReplaceID, ManagerViewerPtr},
		{EJointEditorTapIDs::ContentBrowserID, ContentBrowserPtr},
		{EJointEditorTapIDs::EditorPreferenceID, EditorPreferenceViewPtr}
	};

	for (const FTabWidgetPair& Pair : TabWidgetPairs)
	{
		const TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(Pair.TabId);
		if (Tab.IsValid() && Pair.WidgetPtr.IsValid())
		{
			Tab->ClearContent();
			Tab->SetContent(Pair.WidgetPtr.ToSharedRef());
		}
	}
}

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

	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		if (!Graph->CompileResultPtr.IsValid()) continue;

		if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
		{
			FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");

			CompileResultTabPtr->SetContent(
				MessageLogModule.CreateLogListingWidget(Graph->CompileResultPtr.ToSharedRef()));
		}
	}

	return CompileResultTabPtr;
}

FName FJointEditorToolkit::GetToolkitFName() const { return FName("Joint Editor"); }

FText FJointEditorToolkit::GetBaseToolkitName() const { return LOCTEXT("JointEditor", "Joint Editor"); }

FString FJointEditorToolkit::GetWorldCentricTabPrefix() const { return FString(); }

FLinearColor FJointEditorToolkit::GetWorldCentricTabColorScale() const { return FLinearColor(); }

void FJointEditorToolkit::PostUndo(bool bSuccess)
{
	RequestManagerViewerRefresh();

	// Clear selection, to avoid holding refs to nodes that go away
	if (TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GetFocusedGraphEditor())
	{
		CurrentGraphEditor->ClearSelectionSet();
		CurrentGraphEditor->NotifyGraphChanged();
	}

	// Also refresh the outliner, in case nodes were deleted
	RefreshJointEditorOutliner();
	CleanInvalidDocumentTabs();
	
	FSlateApplication::Get().DismissAllMenus();
}

void FJointEditorToolkit::PostRedo(bool bSuccess)
{
	RequestManagerViewerRefresh();

	// Clear selection, to avoid holding refs to nodes that go away
	if (TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GetFocusedGraphEditor())
	{
		CurrentGraphEditor->ClearSelectionSet();
		CurrentGraphEditor->NotifyGraphChanged();
	}

	// Also refresh the outliner, in case nodes were deleted
	RefreshJointEditorOutliner();
	CleanInvalidDocumentTabs();

	FSlateApplication::Get().DismissAllMenus();
}

void FJointEditorToolkit::OnBeginPIE(bool bArg)
{
}

void FJointEditorToolkit::OnEndPIE(bool bArg)
{
	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = Graph->GetCachedJointGraphNodes();

		for (TWeakObjectPtr<UJointEdGraphNode> CachedJointGraphNode : GraphNodes)
		{
			if (CachedJointGraphNode == nullptr) continue;

			if (!CachedJointGraphNode->GetGraphNodeSlate().IsValid()) continue;
			
			TSharedPtr<SJointGraphNodeBase> GraphNodeSlatePtr = CachedJointGraphNode->GetGraphNodeSlate().Pin();

			GraphNodeSlatePtr->ResetNodeBodyColorAnimation();
		}
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

	ToolkitCommands->MapAction(FGraphEditorCommands::Get().CollapseNodes,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnCollapseSelectionToSubGraph),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanCollapseSelectionToSubGraph),
	                           FIsActionChecked(),
	                           FIsActionButtonVisible::CreateSP(this, &FJointEditorToolkit::CanCollapseSelectionToSubGraph)
	);

	
	ToolkitCommands->MapAction(FGraphEditorCommands::Get().ExpandNodes,
							   FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnExpandNodes),
							   FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanExpandNodes),
							   FIsActionChecked(),
							   FIsActionButtonVisible::CreateSP(this, &FJointEditorToolkit::CanExpandNodes)
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
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::CompileAllJointGraphs),
	                           FCanExecuteAction::CreateSP(this, &FJointEditorToolkit::CanCompileAllJointGraphs)
	);

	ToolkitCommands->MapAction(
		Commands.OpenSearchTab
		, FExecuteAction::CreateSP(this, &FJointEditorToolkit::OpenSearchTab)
		, FCanExecuteAction());

	ToolkitCommands->MapAction(
		Commands.OpenReplaceTab
		, FExecuteAction::CreateSP(this, &FJointEditorToolkit::OpenReplaceTab)
		, FCanExecuteAction());

	if (GetNodePickingManager().IsValid())
	{
		ToolkitCommands->MapAction(
			Commands.EscapeNodePickingMode
			, FExecuteAction::CreateSP(GetNodePickingManager().ToSharedRef(), &FJointEditorNodePickingManager::EndNodePicking)
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


	ToolkitCommands->MapAction(Commands.DissolveSubNodeIntoParentNode,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnDissolveSubNode),
	                           FCanExecuteAction(),
	                           FIsActionChecked::CreateSP(
		                           this, &FJointEditorToolkit::CheckCanDissolveSubNode)
	);

	ToolkitCommands->MapAction(Commands.SolidifySubNodesFromParentNode,
	                           FExecuteAction::CreateSP(this, &FJointEditorToolkit::OnSolidifySubNode),
	                           FCanExecuteAction(),
	                           FIsActionChecked::CreateSP(
		                           this, &FJointEditorToolkit::CheckCanSolidifySubNode)
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

		InitializeDetailView();
		InitializeContentBrowser();
		InitializeEditorPreferenceView();
		InitializeManagerViewer();
		InitializeCompileResult();

		FeedEditorSlateToEachTab();
	}
}

void FJointEditorToolkit::RequestManagerViewerRefresh()
{
	if (ManagerViewerPtr.IsValid()) ManagerViewerPtr->RequestTreeRebuild();
}

UJointManager* FJointEditorToolkit::GetJointManager() const
{
	return JointManager.Get();
}

UJointEdGraph* FJointEditorToolkit::GetMainJointGraph() const
{
	if (GetJointManager() == nullptr) return nullptr;

	if (GetJointManager()->JointGraph == nullptr) return nullptr;

	return Cast<UJointEdGraph>(GetJointManager()->JointGraph);
}

UJointEdGraph* FJointEditorToolkit::GetFocusedJointGraph() const
{
	if (TSharedPtr<SJointGraphEditor> FocusedGraphEditor = GetFocusedGraphEditor())
	{
		if (UEdGraph* Graph = FocusedGraphEditor->GetCurrentGraph())
		{
			return Cast<UJointEdGraph>(Graph);
		}
	}

	return nullptr;
}

bool FJointEditorToolkit::IsGraphInCurrentJointManager(UEdGraph* Graph)
{
	if (Graph == nullptr) return false;

	return UJointEdGraph::GetAllGraphsFrom(GetJointManager()).Contains(Graph);
}

void FJointEditorToolkit::CreateNewRootGraphForJointManagerIfNeeded() const
{
	if (GetJointManager() == nullptr) return;

	if (GetJointManager()->JointGraph != nullptr) return;

	GetJointManager()->JointGraph = UJointEdGraph::CreateNewJointGraph(GetJointManager(), GetJointManager(), FName("MainGraph"));
	GetJointManager()->JointGraph->bAllowDeletion = false;
	
}

TSharedPtr<SDockTab> FJointEditorToolkit::OpenDocument(UObject* DocumentID, FDocumentTracker::EOpenDocumentCause OpenMode)
{
	if (!DocumentManager || !DocumentID) return nullptr;


	if (UJointEdGraph* Graph = Cast<UJointEdGraph>(DocumentID))
	{
		if (!IsGraphInCurrentJointManager(Graph))
		{
			// The graph does not belong to the current Joint Manager being edited.
			return nullptr;
		}

		TArray<TSharedPtr<SDockTab>> Tabs;

		DocumentManager->FindAllTabsForFactory(GraphEditorTabFactoryPtr, Tabs);

		for (TSharedPtr<SDockTab> DockTab : Tabs)
		{
			//cast to SJointGraphEditor
			TSharedRef<SJointGraphEditor> GraphEditor = StaticCastSharedRef<SJointGraphEditor>(DockTab->GetContent());

			if (GraphEditor->GetCurrentGraph() == Graph)
			{
				// The graph is already open, just bring it to front - 
				DockTab->ActivateInParent(ETabActivationCause::SetDirectly);
				return DockTab;
			}
		}

		// The graph is not open yet, open it now, and make it run OnLoaded

		TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);

		Graph->SetToolkit(SharedThis(this));
		Graph->OnLoaded();

		TSharedPtr<SDockTab> NewTab = DocumentManager->OpenDocument(Payload, OpenMode);
		NewTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateSP(this, &FJointEditorToolkit::OnGraphEditorTabClosed));

		return NewTab;
	}


	return nullptr;
}

void FJointEditorToolkit::CloseDocumentTab(UObject* DocumentID)
{
	if (DocumentID)
	{
		TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);
		DocumentManager->CloseTab(Payload);
	}
}


void FJointEditorToolkit::SaveEditedObjectState()
{
	if (GetJointManager())
	{
		// Clear currently edited documents
		GetJointManager()->LastEditedDocuments.Empty();

		// Ask all open documents to save their state, which will update LastEditedDocuments
		DocumentManager->SaveAllState();
	}
}

void FJointEditorToolkit::RestoreEditedObjectState()
{
	UJointManager* Manager = GetJointManager();

	if (Manager && Manager->LastEditedDocuments.Num() == 0)
	{
		// attach the main graph if no other graph was restored.
		if (UJointEdGraph* MainGraph = GetMainJointGraph())
		{
			OpenDocument(MainGraph, FDocumentTracker::OpenNewDocument);
		}
	}

	TSet<FSoftObjectPath> PathsToRemove;
	for (int32 i = 0; i < Manager->LastEditedDocuments.Num(); i++)
	{
		if (UObject* Obj = Manager->LastEditedDocuments[i].EditedObjectPath.ResolveObject())
		{
			if (UEdGraph* Graph = Cast<UEdGraph>(Obj))
			{
				struct LocalStruct
				{
					static TSharedPtr<SDockTab> OpenGraphTree(FJointEditorToolkit* InJointEditor, UEdGraph* InGraph)
					{
						FDocumentTracker::EOpenDocumentCause OpenCause = FDocumentTracker::QuickNavigateCurrentDocument;

						for (UObject* OuterObject = InGraph->GetOuter(); OuterObject; OuterObject = OuterObject->GetOuter())
						{
							if (OuterObject->IsA<UJointManager>())
							{
								// reached up to the blueprint for the graph, we are done climbing the tree
								OpenCause = FDocumentTracker::RestorePreviousDocument;
								break;
							}
							else if (UEdGraph* OuterGraph = Cast<UEdGraph>(OuterObject))
							{
								// Found another graph, open it up
								OpenGraphTree(InJointEditor, OuterGraph);
								break;
							}
						}

						return InJointEditor->OpenDocument(InGraph, OpenCause);
					}
				};
				TSharedPtr<SDockTab> TabWithGraph = LocalStruct::OpenGraphTree(this, Graph);
				if (TabWithGraph.IsValid())
				{
					TSharedRef<SJointGraphEditor> GraphEditor = StaticCastSharedRef<SJointGraphEditor>(TabWithGraph->GetContent());
					GraphEditor->SetViewLocation(Manager->LastEditedDocuments[i].SavedViewOffset, Manager->LastEditedDocuments[i].SavedZoomAmount);
				}
			}
			else
			{
				TSharedPtr<SDockTab> TabWithGraph = OpenDocument(Obj, FDocumentTracker::RestorePreviousDocument);
			}
		}
		else
		{
			PathsToRemove.Add(Manager->LastEditedDocuments[i].EditedObjectPath);
		}
	}

	// Older assets may have neglected to clean up this array when referenced objects were deleted, so
	// we'll check for that now. This is done to ensure we don't store invalid object paths indefinitely.
	if (PathsToRemove.Num() > 0)
	{
		Manager->LastEditedDocuments.RemoveAll([&PathsToRemove](const FEditedDocumentInfo& Entry)
		{
			return PathsToRemove.Contains(Entry.EditedObjectPath);
		});
	}
}

void FJointEditorToolkit::RefreshJointEditorOutliner()
{
	if (JointEditorOutlinerPtr)
	{
		JointEditorOutlinerPtr->GraphActionMenu->RefreshAllActions(true);
	}
}

void FJointEditorToolkit::CleanInvalidDocumentTabs()
{
	if (DocumentManager)
	{
		DocumentManager->CleanInvalidTabs();


		//iterate through all the tabs spawned by the graph editor factory and make sure they are still valid
		// (the document manager doesn't know about the validity of the actual graph objects)
		TArray< TSharedPtr<SDockTab> > Results;

		DocumentManager->FindAllTabsForFactory(GraphEditorTabFactoryPtr,Results);

		for (TSharedPtr<SDockTab> DockTab : Results)
		{
			//cast to SJointGraphEditor
			TSharedRef<SJointGraphEditor> GraphEditor = StaticCastSharedRef<SJointGraphEditor>(DockTab->GetContent());

			if (GraphEditor->GetCurrentGraph() == nullptr || !IsGraphInCurrentJointManager(GraphEditor->GetCurrentGraph()))
			{
				DockTab->RequestCloseTab();
			}
		}
	}
}


TSharedPtr<FDocumentTracker> FJointEditorToolkit::GetDocumentManager() const
{
	return DocumentManager;
}

void FJointEditorToolkit::OnGraphEditorFocused(TSharedRef<SGraphEditor> GraphEditor)
{
	// if the focused graph editor is the same as the current one, do nothing.
	if (FocusedGraphEditorPtr == GraphEditor) return;
	
	// cast check to SJointGraphEditor and store it.

	FocusedGraphEditorPtr = StaticCastSharedRef<SJointGraphEditor>(GraphEditor);

	//Update widgets for the focused graph editor.

	if (FocusedGraphEditorPtr.IsValid())
	{
		if (DetailsViewPtr.IsValid()) DetailsViewPtr->SetObjects(FocusedGraphEditorPtr.Pin()->GetSelectedNodes().Array());
	}

	if (JointFragmentPalettePtr) JointFragmentPalettePtr->RebuildWidget();

	if (ManagerViewerPtr) ManagerViewerPtr->RequestTreeRebuild();
}

void FJointEditorToolkit::OnGraphEditorBackgrounded(TSharedRef<SGraphEditor> GraphEditor)
{
}


void FJointEditorToolkit::OnGraphEditorTabClosed(TSharedRef<SDockTab> DockTab)
{
	SaveEditedObjectState();
}

TSharedPtr<SJointGraphEditor> FJointEditorToolkit::GetFocusedGraphEditor() const
{
	//cast to SJointGraphEditor
	return FocusedGraphEditorPtr.Pin();
}

void FJointEditorToolkit::CompileAllJointGraphs()
{
	if (UJointEdGraph* MainGraph = GetMainJointGraph())
	{
		MainGraph->CompileResultPtr->ClearMessages();
		MainGraph->CompileAllJointGraphFromRoot();
	}

	//Make it display the tab whenever users manually pressed the button.
	if (TabManager)
	{
		TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(EJointEditorTapIDs::CompileResultID);

		if (Tab.IsValid())
		{
			Tab->FlashTab();
		}
	}
}

bool FJointEditorToolkit::CanCompileAllJointGraphs()
{
	return !UJointDebugger::IsPIESimulating();
}


void FJointEditorToolkit::OnCompileJointGraphFinished(
	const UJointEdGraph::FJointGraphCompileInfo& CompileInfo) const
{
	int NumError = 0;
	int NumWarning = 0;
	int NumInfo = 0;
	int NumPerformanceWarning = 0;
	
	if (UJointEdGraph* MainGraph = GetMainJointGraph())
	{
		if (!MainGraph->CompileResultPtr) return;
		TWeakPtr<class IMessageLogListing> WeakCompileResultPtr = MainGraph->CompileResultPtr;

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

		NumError += WeakCompileResultPtr.Pin()->NumMessages(EMessageSeverity::Error);
		NumWarning += WeakCompileResultPtr.Pin()->NumMessages(EMessageSeverity::Warning);
		NumInfo += WeakCompileResultPtr.Pin()->NumMessages(EMessageSeverity::Info);
		NumPerformanceWarning += WeakCompileResultPtr.Pin()->NumMessages(EMessageSeverity::PerformanceWarning);


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
				TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(EJointEditorTapIDs::CompileResultID);

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

TSharedPtr<FJointEditorNodePickingManager> FJointEditorToolkit::GetNodePickingManager() const
{
	return NodePickingManager;
}


TSharedPtr<FJointEditorNodePickingManager> FJointEditorToolkit::GetOrCreateNodePickingManager()
{
	if (!NodePickingManager)
	{
		NodePickingManager = MakeShared<FJointEditorNodePickingManager>(SharedThis(this));
	}

	return NodePickingManager;
}


bool FJointEditorToolkit::CanSelectAllNodes() const
{
	return true;
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
	TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GetFocusedGraphEditor();

	if (!CurrentGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

bool FJointEditorToolkit::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

bool FJointEditorToolkit::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void FJointEditorToolkit::RenameNodes()
{
	TSharedPtr<SJointGraphEditor> FocusedGraphEd = GetFocusedGraphEditor();
	if (FocusedGraphEd.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt); SelectedNode != nullptr &&
				SelectedNode->GetCanRenameNode())
			{
				SelectedNode->RequestStartRenaming();
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

void FJointEditorToolkit::DeleteSelectedNodes()
{
	const TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GetFocusedGraphEditor();

	if (!CurrentGraphEditor.IsValid()) return;

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());
	
	FJointToolkitInlineUtils::StartModifyingGraphs(UJointEdGraph::GetAllGraphsFrom(GetJointManager()));

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();

	FJointEdUtils::RemoveNodes(SelectedNodes.Array());

	CurrentGraphEditor->ClearSelectionSet();

	FJointToolkitInlineUtils::EndModifyingGraphs(UJointEdGraph::GetAllGraphsFrom(GetJointManager()));

	RequestManagerViewerRefresh();
	RefreshJointEditorOutliner();
	CleanInvalidDocumentTabs();
}

void FJointEditorToolkit::DeleteSelectedDuplicatableNodes()
{
	TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GetFocusedGraphEditor();
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

void FJointEditorToolkit::OnCreateComment()
{
	TSharedPtr<SJointGraphEditor> GraphEditor = GetFocusedGraphEditor();
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
	TSharedPtr<SJointGraphEditor> GraphEditor = GetFocusedGraphEditor();

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
		if (!Object) continue;

		JumpToHyperlink(Object, false);

		return;
	}
}


void FJointEditorToolkit::OnCollapseSelectionToSubGraph()
{
	TSharedPtr<SJointGraphEditor> FocusedGraphEd = GetFocusedGraphEditor();
	if (!FocusedGraphEd.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet SelectedNodes = FocusedGraphEd->GetSelectedNodes();

	TSet<UEdGraphNode*> CollapsableNodes;
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (!(*NodeIt)) continue;

		if (UJointEdGraphNode* JointEdGraphNode = Cast<UJointEdGraphNode>(*NodeIt))
		{
			if (!JointEdGraphNode->IsSubNode())
			{
				if (JointEdGraphNode->CanUserDeleteNode())
				{
					CollapsableNodes.Add(JointEdGraphNode);
				}
			}
		}
		else if (UEdGraphNode* EdGraphNode = Cast<UEdGraphNode>(*NodeIt))
		{
			if (EdGraphNode->CanUserDeleteNode())
			{
				CollapsableNodes.Add(EdGraphNode);
			}
		}
	}

	if (CollapsableNodes.Num() > 0)
	{
		const FScopedTransaction Transaction(LOCTEXT("CollapseNodes", "Collapse Nodes to Sub-Graph"));
		CollapseNodes(CollapsableNodes);
	}
}

bool FJointEditorToolkit::CanCollapseSelectionToSubGraph() const
{
	TSharedPtr<SJointGraphEditor> FocusedGraphEd = GetFocusedGraphEditor();
	if (!FocusedGraphEd.IsValid())
	{
		return false;
	}

	const FGraphPanelSelectionSet SelectedNodes = FocusedGraphEd->GetSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (!(*NodeIt)) continue;

		if (UJointEdGraphNode* JointEdGraphNode = Cast<UJointEdGraphNode>(*NodeIt))
		{
			if (!JointEdGraphNode->IsSubNode())
			{
				if (JointEdGraphNode->CanUserDeleteNode())
				{
					return true;
				}
			}
		}
		else if (UEdGraphNode* EdGraphNode = Cast<UEdGraphNode>(*NodeIt))
		{
			if (EdGraphNode->CanUserDeleteNode())
			{
				return true;
			}
		}
	}

	return false;
}

void FJointEditorToolkit::OnExpandNodes()
{
	const FScopedTransaction Transaction( FGraphEditorCommands::Get().ExpandNodes->GetLabel() );
	GetJointManager()->Modify();

	TSet<UEdGraphNode*> ExpandedNodes;
	TSharedPtr<SJointGraphEditor> FocusedGraphEd = FocusedGraphEditorPtr.Pin();

	// Expand selected nodes into the focused graph context.
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	
	if(FocusedGraphEd)
	{
		FocusedGraphEd->ClearSelectionSet();
	}

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		ExpandedNodes.Empty();
		bool bExpandedNodesNeedUniqueGuid = true;

		DocumentManager->CleanInvalidTabs();

		if (UJointEdGraphNode_Composite* SelectedCompositeNode = Cast<UJointEdGraphNode_Composite>(*NodeIt))
		{
			// No need to assign unique GUIDs since the source graph will be removed.
			bExpandedNodesNeedUniqueGuid = false;

			// Expand the composite node back into the world
			UEdGraph* SourceGraph = SelectedCompositeNode->BoundGraph;
			ExpandNode(SelectedCompositeNode, SourceGraph, /*inout*/ ExpandedNodes);

			FJointEdUtils::RemoveGraph(Cast<UJointEdGraph>(SourceGraph));
		}
		
		UEdGraphNode* SourceNode = CastChecked<UEdGraphNode>(*NodeIt);
		check(SourceNode);
		MoveNodesToAveragePos(ExpandedNodes, FVector2D(SourceNode->NodePosX, SourceNode->NodePosY), bExpandedNodesNeedUniqueGuid);
	}

	//update editor UI
	RequestManagerViewerRefresh();
	RefreshJointEditorOutliner();
	CleanInvalidDocumentTabs();
}



void FJointEditorToolkit::MoveNodesToGraph(TArray<TObjectPtr<class UEdGraphNode>>& SourceNodes, UEdGraph* DestinationGraph, TSet<UEdGraphNode*>& OutExpandedNodes, UEdGraphNode** OutEntry, UEdGraphNode** OutResult, const bool bIsCollapsedGraph)
{
	// Move the nodes over, remembering any that are boundary nodes
	while (SourceNodes.Num())
	{
		UEdGraphNode* Node = SourceNodes.Pop();
		UEdGraph* OriginalGraph = Node->GetGraph();

		Node->Modify();
		OriginalGraph->Modify();
		Node->Rename(/*NewName=*/ nullptr, /*NewOuter=*/ DestinationGraph, REN_DontCreateRedirectors);

		// Remove the node from the original graph
		OriginalGraph->RemoveNode(Node, false);
		
		// We do not check CanPasteHere when determining CanCollapseNodes, unlike CanCollapseSelectionToFunction/Macro,
		// so when expanding a collapsed graph we don't want to check the CanPasteHere function:
		if (!bIsCollapsedGraph && !Node->CanPasteHere(DestinationGraph))
		{
			Node->BreakAllNodeLinks();
			continue;
		}

		// Successfully added the node to the graph, we may need to remove flags
		if (Node->HasAllFlags(RF_Transient) && !DestinationGraph->HasAllFlags(RF_Transient))
		{
			Node->SetFlags(RF_Transactional);
			Node->ClearFlags(RF_Transient);
			TArray<UObject*> Subobjects;
			GetObjectsWithOuter(Node, Subobjects);
			for (UObject* Subobject : Subobjects)
			{
				Subobject->ClearFlags(RF_Transient);
				Subobject->SetFlags(RF_Transactional);
			}
		}
		
		DestinationGraph->AddNode(Node, /* bFromUI */ false, /* bSelectNewNode */ false);
		
		if(UJointEdGraphNode_Composite* Composite = Cast<UJointEdGraphNode_Composite>(Node))
		{
			OriginalGraph->SubGraphs.Remove(Composite->BoundGraph);
			DestinationGraph->SubGraphs.Add(Composite->BoundGraph);
		}

		// Want to test exactly against tunnel, we shouldn't collapse embedded collapsed
		// nodes or macros, only the tunnels in/out of the collapsed graph
		if (Node->GetClass() == UJointEdGraphNode_Tunnel::StaticClass())
		{
			UJointEdGraphNode_Tunnel* TunnelNode = Cast<UJointEdGraphNode_Tunnel>(Node);
			if (TunnelNode->bCanHaveOutputs)
			{
				*OutEntry = Node;
			}
			else if (TunnelNode->bCanHaveInputs)
			{
				*OutResult = Node;
			}
		}
		else
		{
			OutExpandedNodes.Add(Node);
		}
	}
}


void FJointEditorToolkit::ExpandNode(UEdGraphNode* InNodeToExpand, UEdGraph* InSourceGraph, TSet<UEdGraphNode*>& OutExpandedNodes)
{
 	UEdGraph* DestinationGraph = InNodeToExpand->GetGraph(); // Graph that the composite node is in (parent graph)
	UEdGraph* SourceGraph = InSourceGraph; // Bound graph of the composite node
	check(SourceGraph);

	// Mark all edited objects so they will appear in the transaction record if needed.
	DestinationGraph->Modify();
	SourceGraph->Modify();
	InNodeToExpand->Modify();

	UEdGraphNode* Entry = nullptr;
	UEdGraphNode* Result = nullptr;

	const bool bIsCollapsedGraph = InNodeToExpand->IsA<UJointEdGraphNode_Composite>();

	// Move all of the nodes from the source graph to the destination graph

	MoveNodesToGraph(SourceGraph->Nodes, DestinationGraph, OutExpandedNodes, &Entry, &Result, bIsCollapsedGraph);
	
	// Handle the destruction of the component nodes
	const UJointEdGraphSchema* GraphSchema = GetDefault<UJointEdGraphSchema>();
	GraphSchema->PruneGatewayNode(Cast<UJointEdGraphNode_Composite>(InNodeToExpand), Entry, Result, nullptr, &OutExpandedNodes);

	if(Entry) Entry->DestroyNode();
	if(Result) Result->DestroyNode();

	// Make sure any subgraphs get propagated appropriately
	if (SourceGraph->SubGraphs.Num() > 0)
	{
		DestinationGraph->SubGraphs.Append(SourceGraph->SubGraphs);
		SourceGraph->SubGraphs.Empty();
	}

	// Fix up the outer for all of the nodes that were moved
	for (UEdGraphNode* OutExpandedNode : OutExpandedNodes)
	{
		if (!OutExpandedNode) continue;

		if (UJointEdGraphNode* JointNode = Cast<UJointEdGraphNode>(OutExpandedNode))
		{
			JointNode->SetOuterAs(DestinationGraph);

			for (UJointEdGraphNode* Subnode : JointNode->GetAllSubNodesInHierarchy())
			{
				if (!Subnode) continue;

				Subnode->SetOuterAs(DestinationGraph);

				// Clean up the slate.
				Subnode->SetGraphNodeSlate(nullptr);
				
			}
			JointNode->SetGraphNodeSlate(nullptr);
			// Clean up the slate.
			
			JointNode->Update();
		}
		else
		{
			OutExpandedNode->Rename(/*NewName=*/ nullptr, /*NewOuter=*/ DestinationGraph);
		}
	}
	
	// Remove the gateway node and source graph
	InNodeToExpand->DestroyNode();
	
}

bool FJointEditorToolkit::CanExpandNodes() const
{
	// Does the selection set contain any composite nodes that are legal to expand?
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (Cast<UJointEdGraphNode_Composite>(*NodeIt))
		{
			return true;
		}
	}

	return false;
}

void FJointEditorToolkit::CollapseNodes(TSet<class UEdGraphNode*>& InCollapsableNodes)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	TSharedPtr<SGraphEditor> FocusedGraphEd = GetFocusedGraphEditor();
	if (!FocusedGraphEd.IsValid())
	{
		return;
	}

	UEdGraph* SourceGraph = FocusedGraphEd->GetCurrentGraph();
	SourceGraph->Modify();

	//UK2Node_Composite
	// Create the composite node that will serve as the gateway into the subgraph
	UJointEdGraphNode_Composite* GatewayNode = nullptr;
	{
		GatewayNode = FJointSchemaAction_NewNode::SpawnNode<UJointEdGraphNode_Composite>(SourceGraph, nullptr, FVector2D(0, 0), true);
		check(GatewayNode);
	}

	FName GraphName;
	GraphName = MakeUniqueObjectName(GetJointManager(), UJointEdGraph::StaticClass(), TEXT("CollapseGraph"));

	// Rename the graph to the correct name
	UEdGraph* DestinationGraph = GatewayNode->BoundGraph;

	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FJointEditorNameValidator(GetJointManager(), GraphName));

	FString NewName = GraphName.ToString();
	NameValidator->FindValidString(NewName);
	DestinationGraph->Rename(*DestinationGraph->GetName(), DestinationGraph->GetOuter(), REN_DontCreateRedirectors);

	CollapseNodesIntoGraph(GatewayNode, GatewayNode->GetInputSink(), GatewayNode->GetOutputSource(), SourceGraph, DestinationGraph, InCollapsableNodes, false, true);

	RefreshJointEditorOutliner();
	CleanInvalidDocumentTabs();
}

void FJointEditorToolkit::CollapseNodesIntoGraph(UJointEdGraphNode_Composite* InGatewayNode, UJointEdGraphNode_Tunnel* InEntryNode, UJointEdGraphNode_Tunnel* InResultNode, UEdGraph* InSourceGraph, UEdGraph* InDestinationGraph,
                                                 TSet<UEdGraphNode*>& InCollapsableNodes, bool bCanDiscardEmptyReturnNode, bool bCanHaveWeakObjPtrParam)
{
	const UJointEdGraphSchema* JointGraphSchema = GetDefault<UJointEdGraphSchema>();

	// Keep track of the statistics of the node positions so the new nodes can be located reasonably well
	int32 SumNodeX = 0;
	int32 SumNodeY = 0;
	int32 MinNodeX = std::numeric_limits<int32>::max();
	int32 MinNodeY = std::numeric_limits<int32>::max();
	int32 MaxNodeX = std::numeric_limits<int32>::min();
	int32 MaxNodeY = std::numeric_limits<int32>::min();

	UEdGraphNode* InterfaceTemplateNode = nullptr;

	TArray<UEdGraphPin*> GatewayPins;

	// Move the nodes over, which may create cross-graph references that we need fix up ASAP
	for (TSet<UEdGraphNode*>::TConstIterator NodeIt(InCollapsableNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = *NodeIt;
		Node->Modify();

		// Update stats
		SumNodeX += Node->NodePosX;
		SumNodeY += Node->NodePosY;
		MinNodeX = FMath::Min<int32>(MinNodeX, Node->NodePosX);
		MinNodeY = FMath::Min<int32>(MinNodeY, Node->NodePosY);
		MaxNodeX = FMath::Max<int32>(MaxNodeX, Node->NodePosX);
		MaxNodeY = FMath::Max<int32>(MaxNodeY, Node->NodePosY);

		// Move the node over
		InSourceGraph->Nodes.Remove(Node);
		InDestinationGraph->Nodes.Add(Node);
		Node->Rename(/*NewName=*/ nullptr, /*NewOuter=*/ InDestinationGraph);

		// Move the sub-graph to the new sub graph - 
		if (UJointEdGraphNode_Composite* Composite = Cast<UJointEdGraphNode_Composite>(Node))
		{
			InSourceGraph->SubGraphs.Remove(Composite->BoundGraph);
			InDestinationGraph->SubGraphs.Add(Composite->BoundGraph);
		}

		// Find cross-graph links
		for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* LocalPin = Node->Pins[PinIndex];

			bool bIsGatewayPin = false;
			if (LocalPin->LinkedTo.Num())
			{
				for (int32 LinkIndex = 0; LinkIndex < LocalPin->LinkedTo.Num(); ++LinkIndex)
				{
					UEdGraphPin* TrialPin = LocalPin->LinkedTo[LinkIndex];
					if (!InCollapsableNodes.Contains(TrialPin->GetOwningNode()))
					{
						bIsGatewayPin = true;
						break;
					}
				}
			}

			// Thunk cross-graph links thru the gateway
			if (bIsGatewayPin)
			{
				// Local port is either the entry or the result node in the collapsed graph
				// Remote port is the node placed in the source graph
				UJointEdGraphNode_Tunnel* LocalPort = (LocalPin->Direction == EGPD_Input) ? InEntryNode : InResultNode;

				// Add a new pin to the entry/exit node and to the composite node
				UEdGraphPin* LocalPortPin = nullptr;
				UEdGraphPin* RemotePortPin = nullptr;


				// If there is a custom event being used as a template, we must check to see if any connected pins have already been built
				if (InterfaceTemplateNode && LocalPin->Direction == EGPD_Input)
				{
					// Find the pin on the entry node, we will use that pin's name to find the pin on the remote port
					UEdGraphPin* EntryNodePin = InEntryNode->FindPin(LocalPin->LinkedTo[0]->PinName);
					if (EntryNodePin)
					{
						LocalPin->BreakAllPinLinks();
						JointGraphSchema->TryCreateConnection(LocalPin,EntryNodePin);
						continue;
					}
				}

				if (LocalPin->LinkedTo[0]->GetOwningNode() != InEntryNode)
				{
					const FName UniquePortName = InGatewayNode->CreateUniquePinName(LocalPin->PinName);

					if (!RemotePortPin && !LocalPortPin)
					{
						FEdGraphPinType PinType = LocalPin->PinType;
						if (PinType.bIsWeakPointer && !PinType.IsContainer() && !bCanHaveWeakObjPtrParam)
						{
							PinType.bIsWeakPointer = false;
						}
						//add pin to the entry/result node

						FJointEdPinDataSetting Setting;
						Setting.bAlwaysDisplayNameText = true;
						
						FJointEdPinData& PinData1 = InGatewayNode->PinData.Add_GetRef(FJointEdPinData(UniquePortName, LocalPin->Direction, Setting));

						InGatewayNode->LockSynchronizing();
						InGatewayNode->UpdatePins();
						RemotePortPin = InGatewayNode->GetPinForPinDataFromThis(PinData1);

						FJointEdPinData& PinData2 = LocalPort->PinData.Add_GetRef(FJointEdPinData(UniquePortName, (LocalPin->Direction == EGPD_Input) ? EGPD_Output : EGPD_Input, Setting));
						LocalPort->LockSynchronizing();
						LocalPort->UpdatePins();
						LocalPortPin = LocalPort->GetPinForPinDataFromThis(PinData2);

						InGatewayNode->UnlockSynchronizing();
						LocalPort->UnlockSynchronizing();
					}
				}


				check(LocalPortPin);
				check(RemotePortPin);

				LocalPin->Modify();

				// Route the links
				for (int32 LinkIndex = 0; LinkIndex < LocalPin->LinkedTo.Num(); ++LinkIndex)
				{
					UEdGraphPin* RemotePin = LocalPin->LinkedTo[LinkIndex];
					RemotePin->Modify();

					if (!InCollapsableNodes.Contains(RemotePin->GetOwningNode()) && RemotePin->GetOwningNode() != InEntryNode && RemotePin->GetOwningNode() != InResultNode)
					{
						// Fix up the remote pin
						RemotePin->LinkedTo.Remove(LocalPin);
						// When given a composite node, we could possibly be given a pin with a different outer
						// which is bad! Then there would be a pin connecting to itself and cause an ensure
						if (RemotePin->GetOwningNode()->GetOuter() == RemotePortPin->GetOwningNode()->GetOuter())
						{
							JointGraphSchema->TryCreateConnection(RemotePin,RemotePortPin);
						}

						// The Entry Node only supports a single link, so if we made links above
						// we need to break them now, to make room for the new link.
						if (LocalPort == InEntryNode)
						{
							LocalPortPin->BreakAllPinLinks();
						}

						// Fix up the local pin
						LocalPin->LinkedTo.Remove(RemotePin);
						--LinkIndex;
						JointGraphSchema->TryCreateConnection(LocalPin,LocalPortPin);
					}
				}
			}
		}
	}

	// Reposition the newly created nodes
	const int32 NumNodes = InCollapsableNodes.Num();


	/*
	// Using the result pin, we will ensure that there is a path through the function by checking if it is connected. If it is not, link it to the entry node.
	if (UEdGraphPin* ResultExecFunc = JointGraphSchema->FindExecutionPin(*InResultNode, EGPD_Input))
	{
		if (ResultExecFunc->LinkedTo.Num() == 0)
		{
			JointGraphSchema->FindExecutionPin(*InEntryNode, EGPD_Output)->MakeLinkTo(JointGraphSchema->FindExecutionPin(*InResultNode, EGPD_Input));
		}
	}
	*/

	const int32 CenterX = (NumNodes == 0) ? SumNodeX : SumNodeX / NumNodes;
	const int32 CenterY = (NumNodes == 0) ? SumNodeY : SumNodeY / NumNodes;
	const int32 MinusOffsetX = 560; //@TODO: Random magic numbers
	const int32 PlusOffsetX = 600;

	// Put the gateway node at the center of the empty space in the old graph
	InGatewayNode->NodePosX = CenterX;
	InGatewayNode->NodePosY = CenterY;
	InGatewayNode->SnapToGrid(SNodePanel::GetSnapGridSize());

	// Put the entry and exit nodes on either side of the nodes in the new graph
	//@TODO: Should we recenter the whole ensemble?
	if (NumNodes != 0)
	{
		InEntryNode->NodePosX = MinNodeX - MinusOffsetX;
		InEntryNode->NodePosY = CenterY;
		InEntryNode->SnapToGrid(SNodePanel::GetSnapGridSize());

		InResultNode->NodePosX = MaxNodeX + PlusOffsetX;
		InResultNode->NodePosY = CenterY;
		InResultNode->SnapToGrid(SNodePanel::GetSnapGridSize());
	}

	//Update the collapsed node's outers to the new graph
	for (UEdGraphNode* InCollapsableNode : InCollapsableNodes)
	{
		if (!InCollapsableNode) continue;

		if (UJointEdGraphNode* JointNode = Cast<UJointEdGraphNode>(InCollapsableNode))
		{
			JointNode->SetOuterAs(InDestinationGraph);

			for (UJointEdGraphNode* Subnode : JointNode->GetAllSubNodesInHierarchy())
			{
				if (!Subnode) continue;

				Subnode->SetOuterAs(InDestinationGraph);

				// Clean up the slate.
				Subnode->SetGraphNodeSlate(nullptr);
			}
			
			// Clean up the slate.
			JointNode->SetGraphNodeSlate(nullptr);

			JointNode->Update();
		}
		else
		{
			InCollapsableNode->Rename(/*NewName=*/ nullptr, /*NewOuter=*/ InDestinationGraph);
		}
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


void FJointEditorToolkit::OpenSearchTab() const
{
	if (!ManagerViewerPtr.IsValid()) return;

	if (TSharedPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(EJointEditorTapIDs::SearchReplaceID))
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
		TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(EJointEditorTapIDs::SearchReplaceID);

		ManagerViewerPtr->SetMode(EJointManagerViewerMode::Search);
	}
}

void FJointEditorToolkit::OpenReplaceTab() const
{
	if (!ManagerViewerPtr.IsValid()) return;

	if (TSharedPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(EJointEditorTapIDs::SearchReplaceID))
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
		TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(EJointEditorTapIDs::SearchReplaceID);

		ManagerViewerPtr->SetMode(EJointManagerViewerMode::Replace);
	}
}

void FJointEditorToolkit::PopulateNodePickingToastMessage()
{
	ClearNodePickingToastMessage();

	TWeakPtr<SJointToolkitToastMessage> Message = GetOrCreateGraphToastMessageHub()->FindToasterMessage(
		NodePickingToastMessageGuid);

	if (Message.IsValid())
	{
		Message.Pin()->PlayAppearAnimation();
	}
	else
	{
		NodePickingToastMessageGuid = GetOrCreateGraphToastMessageHub()->AddToasterMessage(
			SNew(SJointToolkitToastMessage)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Normal * 2)
				[
					SNew(SVerticalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
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
						.Padding(FJointEditorStyle::Margin_Normal)
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
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Clipping(EWidgetClipping::ClipToBounds)
							.Text(LOCTEXT("PickingEnabledTitle", "Node Picking Enabled"))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Clipping(EWidgetClipping::ClipToBounds)
						.Text(LOCTEXT("PickingEnabled", "Click the node to select. Press ESC to escape."))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
					]
				]
			]
		);
	}
}

void FJointEditorToolkit::PopulateTransientEditingWarningToastMessage()
{
	ClearTransientEditingWarningToastMessage();


	TWeakPtr<SJointToolkitToastMessage> Message = GetOrCreateGraphToastMessageHub()->FindToasterMessage(
		TransientEditingToastMessageGuid);

	if (Message.IsValid())
	{
		Message.Pin()->PlayAppearAnimation();
	}
	else
	{
		TransientEditingToastMessageGuid = GetOrCreateGraphToastMessageHub()->AddToasterMessage(
			SNew(SJointToolkitToastMessage)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Normal * 2)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						.Clipping(EWidgetClipping::ClipToBounds)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
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
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Clipping(EWidgetClipping::ClipToBounds)
							.Text(LOCTEXT("PickingEnabledTitle",
							              "You are editing a transient & PIE duplicated object."))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Clipping(EWidgetClipping::ClipToBounds)
						.Text(LOCTEXT("PickingEnabled",
						              "Any modification on this graph will not be applied to the original asset."))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
					]
				]
			]
		);
	}
}

void FJointEditorToolkit::PopulateVisibilityChangeModeForSimpleDisplayPropertyToastMessage()
{
	ClearVisibilityChangeModeForSimpleDisplayPropertyToastMessage();


	TWeakPtr<SJointToolkitToastMessage> Message = GetOrCreateGraphToastMessageHub()->FindToasterMessage(
		VisibilityChangeModeForSimpleDisplayPropertyToastMessageGuid);

	if (Message.IsValid())
	{
		Message.Pin()->PlayAppearAnimation();
	}
	else
	{
		VisibilityChangeModeForSimpleDisplayPropertyToastMessageGuid = GetOrCreateGraphToastMessageHub()->AddToasterMessage(
			SNew(SJointToolkitToastMessage)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Normal)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SVerticalBox)
						.Clipping(EWidgetClipping::ClipToBounds)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox)
							.Clipping(EWidgetClipping::ClipToBounds)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FJointEditorStyle::Margin_Normal)
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
							.Padding(FJointEditorStyle::Margin_Normal)
							[
								SNew(STextBlock)
								.Clipping(EWidgetClipping::ClipToBounds)
								.Text(LOCTEXT("PickingEnabledTitle",
								              "Modifying Simple Display Property Visibility"))
								.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Clipping(EWidgetClipping::ClipToBounds)
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

void FJointEditorToolkit::PopulateNodePickerCopyToastMessage()
{
	ClearNodePickerCopyToastMessage();


	TWeakPtr<SJointToolkitToastMessage> Message = GetOrCreateGraphToastMessageHub()->FindToasterMessage(
		NodePickerCopyToastMessageGuid);

	if (Message.IsValid())
	{
		Message.Pin()->PlayAppearAnimation();
	}
	else
	{
		NodePickerCopyToastMessageGuid = GetOrCreateGraphToastMessageHub()->AddToasterMessage(
			SNew(SJointToolkitToastMessage)
			.SizeDecreaseInterpolationSpeed(2)
			.RemoveAnimationDuration(0.5)
			.Duration(1.5)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Normal)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SVerticalBox)
						.Clipping(EWidgetClipping::ClipToBounds)
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
							.Padding(FJointEditorStyle::Margin_Normal)
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
							.Padding(FJointEditorStyle::Margin_Normal)
							[
								SNew(STextBlock)
								.Clipping(EWidgetClipping::ClipToBounds)
								.Text(LOCTEXT("CopyTitle",
								              "Node Pointer Copied!"))
								.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Clipping(EWidgetClipping::ClipToBounds)
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

	TWeakPtr<SJointToolkitToastMessage> Message = GetOrCreateGraphToastMessageHub()->FindToasterMessage(NodePickerPasteToastMessageGuid);

	if (Message.IsValid())
	{
		Message.Pin()->PlayAppearAnimation();
	}
	else
	{
		NodePickerPasteToastMessageGuid = GetOrCreateGraphToastMessageHub()->AddToasterMessage(
			SNew(SJointToolkitToastMessage)
			.SizeDecreaseInterpolationSpeed(2)
			.RemoveAnimationDuration(0.5)
			.Duration(1.5)
			[
				SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Normal)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SVerticalBox)
						.Clipping(EWidgetClipping::ClipToBounds)
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
							.Padding(FJointEditorStyle::Margin_Normal)
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
							.Padding(FJointEditorStyle::Margin_Normal)
							[
								SNew(STextBlock)
								.Clipping(EWidgetClipping::ClipToBounds)
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

void FJointEditorToolkit::PopulateNeedReopeningToastMessage()
{
	//You can only show this message once, so if the message is already shown, do not show it again.
	if (!GetOrCreateGraphToastMessageHub()->HasToasterMessage(RequestReopenToastMessageGuid))
	{
		TWeakPtr<SJointToolkitToastMessage> Message = GetOrCreateGraphToastMessageHub()->FindToasterMessage(RequestReopenToastMessageGuid);

		if (Message.IsValid())
		{
			Message.Pin()->PlayAppearAnimation();
		}
		else
		{
			RequestReopenToastMessageGuid = GetOrCreateGraphToastMessageHub()->AddToasterMessage(
				SNew(SJointToolkitToastMessage)
				.Duration(-1)
				[
					SNew(SBorder)
					.Padding(FJointEditorStyle::Margin_Normal)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SBorder)
						.Padding(FJointEditorStyle::Margin_Normal)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.BorderBackgroundColor(FJointEditorStyle::Color_Node_Invalid)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SVerticalBox)
							.Clipping(EWidgetClipping::ClipToBounds)
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
								.Padding(FJointEditorStyle::Margin_Normal)
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
								.Padding(FJointEditorStyle::Margin_Normal)
								[
									SNew(STextBlock)
									.Clipping(EWidgetClipping::ClipToBounds)
									.Text(LOCTEXT("ReopeningRequest",
									              "Graph Editor Reported Invalidated Slate Reference Error.\nPlease reopen the editor to solve the issue."))
									.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
								]
							]
						]
					]
				]
			);
		}
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
	if (GraphToastMessageHub.IsValid()) GraphToastMessageHub->RemoveToasterMessage(NodePickerCopyToastMessageGuid, true);
}

void FJointEditorToolkit::ClearNodePickerPastedToastMessage() const
{
	if (GraphToastMessageHub.IsValid()) GraphToastMessageHub->RemoveToasterMessage(NodePickerPasteToastMessageGuid, true);
}

void FJointEditorToolkit::OnContentBrowserAssetDoubleClicked(const FAssetData& AssetData)
{
	if (AssetData.GetAsset()) AssetViewUtils::OpenEditorForAsset(AssetData.GetAsset());
}

void FJointEditorToolkit::StartHighlightingNode(UJointEdGraphNode* NodeToHighlight, bool bBlinkForOnce)
{
	if (NodeToHighlight == nullptr) return;

	if (!NodeToHighlight->GetGraphNodeSlate().IsValid()) return;

	TSharedPtr<SJointGraphNodeBase> GraphNodeSlate = NodeToHighlight->GetGraphNodeSlate().Pin();

	GraphNodeSlate->PlayHighlightAnimation(bBlinkForOnce);
}

void FJointEditorToolkit::StopHighlightingNode(UJointEdGraphNode* NodeToHighlight)
{
	if (NodeToHighlight == nullptr) return;

	if (!NodeToHighlight->GetGraphNodeSlate().IsValid()) return;

	TSharedPtr<SJointGraphNodeBase> GraphNodeSlate = NodeToHighlight->GetGraphNodeSlate().Pin();

	GraphNodeSlate->StopHighlightAnimation();

}

void FJointEditorToolkit::JumpToNode(UEdGraphNode* Node, const bool bRequestRename)
{
	if (!GetFocusedGraphEditor().IsValid()) return;

	const UEdGraphNode* FinalCenterTo = Node;

	//if the node was UJointEdGraphNode type, then make it sure to be centered to its parentmost node.
	if (Cast<UJointEdGraphNode>(Node))
	{
		StartHighlightingNode(Cast<UJointEdGraphNode>(Node), true);

		FinalCenterTo = Cast<UJointEdGraphNode>(Node)->GetParentmostNode();
	}

	GetFocusedGraphEditor()->JumpToNode(FinalCenterTo, false, false);
}

void FJointEditorToolkit::JumpToHyperlink(UObject* ObjectReference, bool bRequestRename)
{
	if (UEdGraphNode* GraphNodeNode = Cast<UEdGraphNode>(ObjectReference))
	{
		OpenDocument(GraphNodeNode->GetGraph(), FDocumentTracker::OpenNewDocument);

		JumpToNode(GraphNodeNode, false);
	}
	else if (UJointNodeBase* Node = Cast<UJointNodeBase>(ObjectReference))
	{
		TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

		for (UJointEdGraph* Graph : Graphs)
		{
			if (!Graph) continue;

			if (!Graph->GetCachedJointNodeInstances().Contains(Node)) continue;

			OpenDocument(Graph, FDocumentTracker::OpenNewDocument);

			JumpToNode(Graph->FindGraphNodeForNodeInstance(Node), false);
		}
	}
	else if (const UBlueprintGeneratedClass* Class = Cast<const UBlueprintGeneratedClass>(ObjectReference))
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Class->ClassGeneratedBy);
	}
	else if ((ObjectReference != nullptr) && ObjectReference->IsAsset())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(const_cast<UObject*>(ObjectReference));
	}
	else if (UJointEdGraph* Graph = Cast<UJointEdGraph>(ObjectReference))
	{
		// open or create or activate the editor for this graph
		OpenDocument(Graph, FDocumentTracker::OpenNewDocument);
	}
	else
	{
		UE_LOG(LogBlueprint, Warning, TEXT("Unknown type of hyperlinked object (%s), cannot focus it"),
		       *GetNameSafe(ObjectReference));
	}
}


void FJointEditorToolkit::SelectAllNodes()
{
	if (TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GetFocusedGraphEditor())
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}


void FJointEditorToolkit::CopySelectedNodes()
{
	// Get the current graph

	if (!GetFocusedGraphEditor()) return;

	UEdGraph* Graph = GetFocusedGraphEditor()->GetCurrentGraph();

	if (Graph == nullptr) return;

	//Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	//Target nodes to copy. Ensure to be cleaned up before copying (it's necessary to empty it up because of the compiler optimization can cause issues sometimes.)
	TSet<UObject*> NodesToCopy;
	
	NodesToCopy.Empty();

	for (UObject* SelectedNode : SelectedNodes)
	{
		if (SelectedNode == nullptr) continue;
		
		if (UEdGraphNode* GraphNode = Cast<UEdGraphNode>(SelectedNode))
		{
			if (GraphNode->CanDuplicateNode()) NodesToCopy.Add(GraphNode);
		}
	}
	
	
	// Remove the nodes that are contained within other selected nodes - we only want to copy the top-level nodes
	for (int i = NodesToCopy.Num() - 1; i >= 0; --i)
	{
		UObject* NodeToCheck = NodesToCopy.Array()[i];

		if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(NodeToCheck))
		{
			for (UJointEdGraphNode* SubNode : CastedNode->GetAllSubNodesInHierarchy())
			{
				if (NodesToCopy.Contains(SubNode))
				{
					NodesToCopy.Remove(SubNode);
				}
			}
		}
	}
	
		
	
	for (UObject* NodeToCopy : NodesToCopy)
	{
		if (UEdGraphNode* CastedNode = Cast<UEdGraphNode>(NodeToCopy))
			CastedNode->PrepareForCopying();	
	}

	FString ExportedText;
	
	FEdGraphUtilities::ExportNodesToText(NodesToCopy, ExportedText);

	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
	
	for (UObject* NodeToCopy : NodesToCopy)
	{
		if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(NodeToCopy))
			CastedNode->PostCopyNode();	
	}

}

void FJointEditorToolkit::PasteNodes()
{
	if (const TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GetFocusedGraphEditor())
	{
		PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
	}

	RequestManagerViewerRefresh();
}

void FJointEditorToolkit::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();

	RequestManagerViewerRefresh();
}

void FJointEditorToolkit::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();

	RequestManagerViewerRefresh();
}

void FJointEditorToolkit::OnGraphSelectedNodesChanged(UEdGraph* InGraph, const TSet<class UObject*>& NewSelection)
{
	if (GetNodePickingManager().IsValid() && GetNodePickingManager()->IsInNodePicking())
	{
		for (UObject* Selection : NewSelection)
		{
			if (Selection == nullptr) continue;

			UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(Selection);

			if (CastedGraphNode == nullptr) continue;

			TSharedPtr<FJointEditorNodePickingManagerResult> Result = FJointEditorNodePickingManagerResult::MakeInstance();
			Result->Node = CastedGraphNode->GetCastedNodeInstance();
			Result->OptionalEdNode = CastedGraphNode;

			GetNodePickingManager()->PerformNodePicking(Result);

			break;
		}
	}
	else
	{
		SelectProvidedObjectOnDetail(NewSelection);
		NotifySelectionChangeToNodeSlates(InGraph, NewSelection);
	}
}

void FJointEditorToolkit::NotifySelectionChangeToNodeSlates(UEdGraph* InGraph, const TSet<class UObject*>& NewSelection) const
{
	if (JointManager == nullptr) return;

	if (UJointEdGraph* CastedGraph = InGraph ? Cast<UJointEdGraph>(InGraph) : nullptr)
	{
		TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCachedJointGraphNodes();

		for (TWeakObjectPtr<UJointEdGraphNode> GraphNode : GraphNodes)
		{
			if (!GraphNode.IsValid()) continue;

			const TSharedPtr<SJointGraphNodeBase> NodeSlate = GraphNode->GetGraphNodeSlate().Pin();

			if (!NodeSlate.IsValid()) continue;

			NodeSlate->OnGraphSelectionChanged(NewSelection);
		}
	}

	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}

void FJointEditorToolkit::PasteNodesHere(const FVector2D& Location)
{

	//early returns
	TSharedPtr<SJointGraphEditor> CurrentGraphEditor = GetFocusedGraphEditor();

	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}
	
	UJointEdGraph* CastedGraph = CurrentGraphEditor->GetCurrentGraph() ? Cast<UJointEdGraph>(CurrentGraphEditor->GetCurrentGraph()) : nullptr;

	if (CastedGraph == nullptr)
	{
		return;
	}
	
	//Get the currently selected node on the graph and store it to use it when if we need to attach some nodes that will be imported right after this.
	//But if there is any node that can be placed directly on the graph among the imported nodes, we will not implement those dependent nodes.
	//This is for the fragment copy & paste between nodes.
	
	
	UJointEdGraphNode* AttachTargetNode = nullptr;
	
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	for (UObject* SelectedNode : SelectedNodes)
	{
		if (SelectedNode == nullptr) continue;

		if (UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(SelectedNode))
		{
			//Only can work with a single object. Not considering paste action for the multiple nodes at once, because it will be confusing and annoying sometimes (ex, Users didn't know that they selected unwanted node to paste to.)
			if (AttachTargetNode != nullptr) return;
			
			AttachTargetNode = Node;
		}
	}

	// Start to do actual paste operation - make a transaction for undo/redo
	const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());

	CastedGraph->Modify();
	CastedGraph->LockUpdates();
	
	// Clear the selection set (newly pasted stuff will be selected)
	CurrentGraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;

	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;

	FEdGraphUtilities::ImportNodesFromText(CastedGraph, TextToImport, PastedNodes);
	
	// Move the pasted nodes to the location of the paste request
	FJointEdUtils::MoveNodesAtLocation(PastedNodes, Location);
	
	// Handle any fixup of the imported nodes
	FJointEdUtils::MarkNodesAsModifiedAndValidateName(PastedNodes);
	
	// See if the newly pasted nodes can be attached to the selected node on the graph.
	// if it's a node that had a parent node when it was copied, and we have a valid attach target node, then attach it to the target node.
	if (AttachTargetNode)
	{
		AttachTargetNode->Modify();

		if (AttachTargetNode->GetCastedNodeInstance()) AttachTargetNode->GetCastedNodeInstance()->Modify();

		for (UEdGraphNode* InEdGraphNode : PastedNodes)
		{
			if (InEdGraphNode == nullptr) continue;
		
			if (UJointEdGraphNode_Fragment* CastedPastedNode = Cast<UJointEdGraphNode_Fragment>(InEdGraphNode))
			{
				CastedPastedNode->ParentNode = AttachTargetNode;
				AttachTargetNode->AddSubNode(CastedPastedNode);

				// Remove the Paste Node from the graph.
				CastedGraph->RemoveNode(CastedPastedNode);
			}
		}
	}else
	{
		for (UEdGraphNode* InEdGraphNode : PastedNodes)
		{
			if (InEdGraphNode == nullptr) continue;
		
			if (UJointEdGraphNode_Fragment* CastedPastedNode = Cast<UJointEdGraphNode_Fragment>(InEdGraphNode))
			{
				CastedGraph->RemoveNode(CastedPastedNode);
			}
		}
	}
	
	// Explicitly call PostProcessPastedNodes on the pasted nodes one more time since we modified attachment. (FEdGraphUtilities::ImportNodesFromText already called it once)
	FEdGraphUtilities::PostProcessPastedNodes(PastedNodes);
	
	// Notify the graph that graph nodes have been pasted.
	if (CastedGraph)
	{
		CastedGraph->OnNodesPasted(TextToImport);
		CastedGraph->UnlockUpdates();
		CastedGraph->NotifyGraphChanged();
	}

	// Update UI
	CurrentGraphEditor->NotifyGraphChanged();

	// Mark the package as dirty
	UObject* GraphOwner = CastedGraph->GetOuter();
	if (GraphOwner)
	{
		GraphOwner->PostEditChange();
		GraphOwner->MarkPackageDirty();
	}
	
	// Select the newly pasted nodes on the graph
	TSet<class UObject*> Selection;

	for (UEdGraphNode* PastedNode : PastedNodes)
	{
		if (!PastedNode) continue;

		Selection.Add(PastedNode);
	}

	SelectProvidedObjectOnGraph(Selection);
	
}


void FJointEditorToolkit::OnEnableBreakpoint()
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (*NodeIt == nullptr) continue;
		UJointEdGraphNode* Node = CastChecked<UJointEdGraphNode>(*NodeIt);
		if (Node == nullptr) continue;

		FJointNodeDebugData* DebugData = UJointDebugger::GetDebugDataForInstance(Node);

		if (DebugData && DebugData->bHasBreakpoint == true)
		{
			DebugData->bIsBreakpointEnabled = true;
			UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(Node, DebugData);
		}
	}

	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}


bool FJointEditorToolkit::CanEnableBreakpoint() const
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (!*NodeIt) continue;
		
		UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(*NodeIt);
		if (Node == nullptr) continue;

		FJointNodeDebugData* DebugData = UJointDebugger::GetDebugDataForInstance(Node);

		if (DebugData && DebugData->bHasBreakpoint == true && DebugData->bIsBreakpointEnabled == false)
		{
			return true;
		}
	}
	

	return false;
}

void FJointEditorToolkit::OnToggleBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TSet<UJointEdGraph*> ModifiedGraphs;

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);
		if (SelectedNode == nullptr) continue;

		if (TArray<FJointNodeDebugData>* DebugDataArr = UJointDebugger::GetCorrespondingDebugDataForGraph(SelectedNode->GetCastedGraph()))
		{
			//Mark this graph as modified.
			ModifiedGraphs.Add(SelectedNode->GetCastedGraph());

			//Toggle the breakpoint state.
			if (FJointNodeDebugData* Data = UJointDebugger::GetDebugDataForInstanceFrom(DebugDataArr, SelectedNode); Data != nullptr)
			{
				if (Data->bHasBreakpoint) //remove breakpoint - while keeping the debug data instance.
				{
					Data->bHasBreakpoint = false;
					Data->bIsBreakpointEnabled = false;
					
					UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(SelectedNode, Data);
				}
				else //add breakpoint - but with the existing debug data instance.
				{
					Data->bHasBreakpoint = true;
					Data->bIsBreakpointEnabled = true;
					
					UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(SelectedNode, Data);
				}
			}
			else
			{
				//add a breakpoint - but add a new debug data instance.
				//(This is the first time to add a breakpoint for this node)

				FJointNodeDebugData NewDebugData;

				NewDebugData.Node = FJointEdUtils::GetOriginalJointGraphNodeFromJointGraphNode(SelectedNode);

				if (NewDebugData.Node)
				{
					NewDebugData.bHasBreakpoint = true;
					NewDebugData.bIsBreakpointEnabled = true;

					DebugDataArr->Add(NewDebugData);

					UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(SelectedNode, &NewDebugData);
				}
			}
		}
	}
	
	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
	
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

void FJointEditorToolkit::OnDisableBreakpoint()
{

	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (*NodeIt) continue;
		
		UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(*NodeIt);
		if (Node == nullptr) continue;

		FJointNodeDebugData* DebugData = UJointDebugger::GetDebugDataForInstance(Node);

		if (DebugData && DebugData->bHasBreakpoint == true && DebugData->bIsBreakpointEnabled == true)
		{
			DebugData->bIsBreakpointEnabled = false;
			UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(Node, DebugData);
		}
	}
	

	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}


bool FJointEditorToolkit::CanDisableBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TMap<UJointEdGraph*, TArray<FJointNodeDebugData>*> GraphToDebugDataMap;

	// Collect all the graphs that are related to the selected nodes.
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (!SelectedNode) continue;

		UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(SelectedNode);
		if (Node == nullptr) continue;

		//Avoid duplicate graph search.
		if (GraphToDebugDataMap.Contains(Node->GetCastedGraph())) continue;

		if (TArray<FJointNodeDebugData>* DebugDataArr = UJointDebugger::GetCorrespondingDebugDataForGraph(Node->GetCastedGraph()))
		{
			GraphToDebugDataMap.Add(Node->GetCastedGraph(), DebugDataArr);
		}
	}

	// check if every selected node has a breakpoint and is enabled.
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (!SelectedNode) continue;
		UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(SelectedNode);
		if (Node == nullptr) continue;

		if (TArray<FJointNodeDebugData>* DebugDataArr = GraphToDebugDataMap.FindRef(Node->GetCastedGraph()))
		{
			FJointNodeDebugData* Data = UJointDebugger::GetDebugDataForInstanceFrom(DebugDataArr, Node);

			if (Data != nullptr && Data->Node != nullptr && Data->bHasBreakpoint && !Data->bIsBreakpointEnabled) return false;
		}
	}

	return true;
}

void FJointEditorToolkit::OnAddBreakpoint()
{

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TMap<UJointEdGraph*, TArray<FJointNodeDebugData>*> GraphToDebugDataMap;

	// Collect all the graphs that are related to the selected nodes.
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (SelectedNode == nullptr) continue;
		UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(SelectedNode);
		if (Node == nullptr) continue;

		//Avoid duplicate graph search.
		if (GraphToDebugDataMap.Contains(Node->GetCastedGraph())) continue;

		if (TArray<FJointNodeDebugData>* DebugDataArr = UJointDebugger::GetCorrespondingDebugDataForGraph(Node->GetCastedGraph()))
		{
			GraphToDebugDataMap.Add(Node->GetCastedGraph(), DebugDataArr);
		}
	}

	// check if every selected node has a breakpoint and is enabled.
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (SelectedNode == nullptr) continue;
		UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(SelectedNode);
		if (Node == nullptr) continue;

		if (TArray<FJointNodeDebugData>* DebugDataArr = GraphToDebugDataMap.FindRef(Node->GetCastedGraph()))
		{
			FJointNodeDebugData* Data = UJointDebugger::GetDebugDataForInstanceFrom(DebugDataArr, Node);

			if (Data != nullptr)
			{
				Data->bHasBreakpoint = true;
				Data->bIsBreakpointEnabled = false;
				UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(Node, Data);
			}
			else
			{
				FJointNodeDebugData NewDebugData;

				NewDebugData.Node = Node;
				NewDebugData.bHasBreakpoint = true;
				NewDebugData.bIsBreakpointEnabled = true;

				DebugDataArr->Add(NewDebugData);
				UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(Node, Data);
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

			FJointNodeDebugData* Data = UJointDebugger::GetDebugDataForInstance(SelectedNode);

			if (Data == nullptr) return true;
			if (Data->bHasBreakpoint == false) return true;
		}
	}

	return false;
}

void FJointEditorToolkit::OnRemoveBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);

		if (SelectedNode)
		{
			FJointNodeDebugData* Data = UJointDebugger::GetDebugDataForInstance(SelectedNode);

			if (Data != nullptr)
			{
				Data->bHasBreakpoint = false;
				Data->bIsBreakpointEnabled = false;
				UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(SelectedNode, Data);
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
			FJointNodeDebugData* Data = UJointDebugger::GetDebugDataForInstance(SelectedNode);

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
	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		for (FJointNodeDebugData& DebugData : Graph->DebugData)
		{
			DebugData.bHasBreakpoint = false;
			UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(DebugData.Node, &DebugData);
		}
	}
	
	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}

bool FJointEditorToolkit::CanRemoveAllBreakpoints() const
{
	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		return !Graph->DebugData.IsEmpty();
	}

	return false;
}

void FJointEditorToolkit::OnEnableAllBreakpoints()
{
	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		for (FJointNodeDebugData& DebugData : Graph->DebugData)
		{
			DebugData.bIsBreakpointEnabled = true;
			UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(DebugData.Node, &DebugData);
		}
	}
	
	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}

bool FJointEditorToolkit::CanEnableAllBreakpoints() const
{
	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		for (const FJointNodeDebugData& DebugData : Graph->DebugData)
		{
			if (!DebugData.bIsBreakpointEnabled) return true;
		}

		return false;
	}

	return false;
}

void FJointEditorToolkit::OnDisableAllBreakpoints()
{
	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		for (FJointNodeDebugData& DebugData : Graph->DebugData)
		{
			DebugData.bIsBreakpointEnabled = false;
			UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(DebugData.Node, &DebugData);
		}
	}
	
	UJointDebugger::NotifyDebugDataChanged(GetJointManager());
}

bool FJointEditorToolkit::CanDisableAllBreakpoints() const
{
	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		for (const FJointNodeDebugData& DebugData : Graph->DebugData)
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

void FJointEditorToolkit::OnDissolveSubNode()
{
	// dissolve all the sub nodes under the selected nodes - and refresh the selected nodes' parent nodes - but only once for each parent node for the performance reason.

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TSet<UJointEdGraphNode*> ParentNodesToRefresh;
	TSet<UJointEdGraphNode*> NodesToApply;

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);

		if (SelectedNode == nullptr) continue;

		NodesToApply.Add(SelectedNode);

		const FScopedTransaction Transaction(NSLOCTEXT("JointEdTransaction", "TransactionTitle_SolidifySubNode", "Solidify Sub Node"));

		TArray<UJointEdGraphNode*> SubNodes = SelectedNode->GetAllSubNodesInHierarchy();

		for (UJointEdGraphNode* SubNode : SubNodes)
		{
			if (!SubNode) continue;

			UJointEdGraphNode_Fragment* CastedJointEdGraphNode = Cast<UJointEdGraphNode_Fragment>(SubNode);

			if (CastedJointEdGraphNode == nullptr) continue;

			NodesToApply.Add(CastedJointEdGraphNode);

			if (CastedJointEdGraphNode->ParentNode && !NodesToApply.Contains(CastedJointEdGraphNode->ParentNode)) ParentNodesToRefresh.Add(CastedJointEdGraphNode->ParentNode);
		}
	}

	TArray<UJointEdGraphNode*> AsArray = NodesToApply.Array();

	for (int32 i = AsArray.Num() - 1; i >= 0; --i)
	{
		UJointEdGraphNode* NodeToApply = AsArray[i];

		if (NodeToApply == nullptr) continue;

		if (UJointEdGraphNode_Fragment* CastedJointEdGraphNode = Cast<UJointEdGraphNode_Fragment>(NodeToApply))
		{
			CastedJointEdGraphNode->DissolveSelf();
		}
		else
		{
			//for the cases if it was not a fragment node but a parent node that has sub nodes, we need to refresh it too.
			NodeToApply->ReconstructNode();
		}
	}

	for (UJointEdGraphNode* ParentNodeToRefresh : ParentNodesToRefresh)
	{
		if (ParentNodeToRefresh == nullptr) continue;

		ParentNodeToRefresh->ReconstructNode();
	}
}

bool FJointEditorToolkit::CheckCanDissolveSubNode() const
{
	return false;
}

void FJointEditorToolkit::OnSolidifySubNode()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	TSet<UJointEdGraphNode*> NodesToApply;
	TSet<UJointEdGraphNode*> ParentNodesToRefresh;

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UJointEdGraphNode* SelectedNode = Cast<UJointEdGraphNode>(*NodeIt);

		if (SelectedNode == nullptr) continue;

		NodesToApply.Add(SelectedNode);

		const FScopedTransaction Transaction(NSLOCTEXT("JointEdTransaction", "TransactionTitle_SolidifySubNode", "Solidify Sub Node"));

		TArray<UJointEdGraphNode*> SubNodes = SelectedNode->GetAllSubNodesInHierarchy();

		for (UJointEdGraphNode* SubNode : SubNodes)
		{
			if (!SubNode) continue;

			UJointEdGraphNode_Fragment* CastedJointEdGraphNode = Cast<UJointEdGraphNode_Fragment>(SubNode);

			if (CastedJointEdGraphNode == nullptr) continue;

			NodesToApply.Add(CastedJointEdGraphNode);

			if (CastedJointEdGraphNode->ParentNode && !NodesToApply.Contains(CastedJointEdGraphNode->ParentNode)) ParentNodesToRefresh.Add(CastedJointEdGraphNode->ParentNode);
		}
	}

	TArray<UJointEdGraphNode*> AsArray = NodesToApply.Array();

	for (int32 i = AsArray.Num() - 1; i >= 0; --i)
	{
		UJointEdGraphNode* NodeToApply = AsArray[i];

		if (NodeToApply == nullptr) continue;

		if (UJointEdGraphNode_Fragment* CastedJointEdGraphNode = Cast<UJointEdGraphNode_Fragment>(NodeToApply))
		{
			CastedJointEdGraphNode->SolidifySelf();
		}
		else
		{
			//for the cases if it was not a fragment node but a parent node that has sub nodes, we need to refresh it too.
			NodeToApply->ReconstructNode();
		}
	}

	for (UJointEdGraphNode* ParentNodeToRefresh : ParentNodesToRefresh)
	{
		if (ParentNodeToRefresh == nullptr) continue;

		ParentNodeToRefresh->ReconstructNode();
	}
}

bool FJointEditorToolkit::CheckCanSolidifySubNode() const
{
	return false;
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

void FJointEditorToolkit::SelectProvidedObjectOnGraph(TSet<UObject*> NewSelection)
{
	TArray<UObject*> Selection = NewSelection.Array();

	if (GetFocusedGraphEditor().IsValid() && GetFocusedGraphEditor()->GetGraphPanel() != nullptr)
	{
		GetFocusedGraphEditor()->GetGraphPanel()->SelectionManager.SetSelectionSet(NewSelection);
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

void FJointEditorToolkit::MoveNodesToAveragePos(TSet<UEdGraphNode*>& AverageNodes, FVector2D SourcePos, bool bExpandedNodesNeedUniqueGuid) const
{
	if (AverageNodes.Num() > 0)
	{
		FVector2D AvgNodePosition(0.0f, 0.0f);

		for (TSet<UEdGraphNode*>::TIterator It(AverageNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			AvgNodePosition.X += Node->NodePosX;
			AvgNodePosition.Y += Node->NodePosY;
		}

		float InvNumNodes = 1.0f / float(AverageNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;

		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEditorPtr.Pin();

		for (UEdGraphNode* ExpandedNode : AverageNodes)
		{
			ExpandedNode->NodePosX = (ExpandedNode->NodePosX - AvgNodePosition.X) + SourcePos.X;
			ExpandedNode->NodePosY = (ExpandedNode->NodePosY - AvgNodePosition.Y) + SourcePos.Y;

			ExpandedNode->SnapToGrid(SNodePanel::GetSnapGridSize());

			if (bExpandedNodesNeedUniqueGuid)
			{
				ExpandedNode->CreateNewGuid();
			}

			//Add expanded node to selection
			FocusedGraphEd->SetNodeSelection(ExpandedNode, true);
		}
	}
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


void FJointEditorToolkit::OnGraphEditorNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo,
                                                          UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		const FScopedTransaction Transaction(NSLOCTEXT("JointEdTransaction", "TransactionTitle_RenameNode", "Rename node"));
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

TSharedPtr<class SJointToolkitToastMessageHub> FJointEditorToolkit::GetOrCreateGraphToastMessageHub()
{
	if (!GraphToastMessageHub.IsValid())
	{
		GraphToastMessageHub = SNew(SJointToolkitToastMessageHub);
	}

	return GraphToastMessageHub;
}

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

TSharedRef<class SWidget> FJointEditorToolkit::OnGetDebuggerActorsMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	if (UJointDebugger* DebuggerSingleton = UJointDebugger::Get())
	{
		TArray<AJointActor*> MatchingInstances;

		//If it already has the debugging asset, then use that instance to getter the Joint manager because the toolkit's GetJointManager() will return the instance of the duplicated, transient version of it.
		DebuggerSingleton->GetMatchingInstances(GetDebuggingJointInstance().IsValid()
			                                        ? GetDebuggingJointInstance().Get()->OriginalJointManager.Get()
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

void FJointEditorToolkit::OnDebuggerActorSelected(TWeakObjectPtr<AJointActor> InstanceToDebug)
{
	//Feed it to the debugger.
	if (InstanceToDebug != nullptr)
	{
		FJointEditorToolkit* Toolkit = nullptr;

		UJointDebugger::Get()->AssignDebuggingInstance(InstanceToDebug.Get());

		FJointEdUtils::FindOrOpenJointEditorInstanceFor(InstanceToDebug->GetJointManager(), true);
	}
}

bool FJointEditorToolkit::IsPlayInEditorActive() const
{
	return GEditor->PlayWorld != nullptr;
}

bool FJointEditorToolkit::IsInEditingMode() const
{
	UJointManager* Manager = GetJointManager();
	if (!FSlateApplication::Get().InKismetDebuggingMode())
	{
		if (!IsPlayInEditorActive())
		{
			return true;
		}
		else
		{
			if (Manager && Manager->IsAsset()) return true; // editing the asset, not the instance.
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
