//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointEditor.h"

#include "AssetToolsModule.h"
#include "JointEdGraphNodesCustomization.h"

#include "JointEditorCommands.h"
#include "JointEditorStyle.h"

#include "JointFragmentActions.h"
#include "JointGraphPinSlateFactory.h"
#include "JointManagerActions.h"
#include "JointGraphNodeSlateFactory.h"

#include "EdGraphUtilities.h"
#include "ISequencerModule.h"
#include "JointBuildPresetActions.h"
#include "JointEditorSettings.h"
#include "JointManagement.h"
#include "JointManagementTabs.h"
#include "JointManager.h"

#include "Debug/JointDebugger.h"
#include "EditorTools/SJointBulkSearchReplace.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Node/JointFragment.h"
#include "SharedType/JointSharedTypes.h"
#include "Widgets/Docking/SDockTab.h"
#include "Node/JointNodeBase.h"

#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Sequencer/JointMovieTrackEditor.h"
#include "UObject/CoreRedirects.h"

#define LOCTEXT_NAMESPACE "JointEditorModule"

TSharedPtr<FExtensibilityManager> FJointEditorModule::GetMenuExtensibilityManager()
{
	return MenuExtensibilityManager;
}

TSharedPtr<FExtensibilityManager> FJointEditorModule::GetToolBarExtensibilityManager()
{
	return ToolBarExtensibilityManager;
}


void FJointEditorModule::StartupModule()
{
	FJointEditorCommands::Register();
	FJointDebuggerCommands::Register();

	RegisterModuleCommand();

	FJointEditorStyle::ResetToDefault();

	RegisterAssetTools();
	RegisterMenuExtensions();
	RegisterSequencerTrack();
	UnregisterClassLayout(); // Remove Default Layout;
	RegisterClassLayout();
	
	//Joint 2.9: JointCoreRedirects
	AppendActiveJointRedirects();


	JointManagementTabHandler = FJointManagementTabHandler::MakeInstance();

	JointManagementTabHandler->AddSubTab(FJointManagementTab_JointEditorUtilityTab::MakeInstance());
	JointManagementTabHandler->AddSubTab(FJointManagementTab_NodeClassManagementTab::MakeInstance());

	JointNodeStyleFactory = MakeShareable(new FJointGraphNodeSlateFactory);
	JointGraphPinFactory = MakeShareable(new FJointGraphPinSlateFactory);

	FEdGraphUtilities::RegisterVisualNodeFactory(JointNodeStyleFactory);
	FEdGraphUtilities::RegisterVisualPinFactory(JointGraphPinFactory);

	RegisterDebugger();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(JointToolTabNames::JointManagementTab,
	                                                  FOnSpawnTab::CreateRaw(
		                                                  this, &FJointEditorModule::OnSpawnJointManagementTab))
		.SetDisplayName(LOCTEXT("JointManagementDisplayName", "Joint Management"))
		.SetMenuType(ETabSpawnerMenuType::Enabled)
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FJointEditorStyle::GetStyleSetName(), "ClassIcon.JointManager"));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(JointToolTabNames::JointBulkSearchReplaceTab,
	                                                  FOnSpawnTab::CreateRaw(
		                                                  this, &FJointEditorModule::OnSpawnJointBulkSearchReplaceTab))
		.SetDisplayName(LOCTEXT("JointBulkSearchReplaceDisplayName", "Joint Bulk Search & Replace"))
		.SetMenuType(ETabSpawnerMenuType::Enabled)
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FJointEditorStyle::GetStyleSetName(), "ClassIcon.JointManager"));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(JointToolTabNames::JointCompilerTab,
	                                                  FOnSpawnTab::CreateRaw(
		                                                  this, &FJointEditorModule::OnSpawnJointCompilerTab))
		.SetDisplayName(LOCTEXT("JointCompilerDisplayName", "Joint Compiler"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FJointEditorModule::ShutdownModule()
{
	FlushClassCaches();

	UnregisterAssetTools();
	UnregisterMenuExtensions();
	UnregisterSequencerTrack();
	UnregisterClassLayout();
	UnregisterDebugger();

	FEdGraphUtilities::UnregisterVisualNodeFactory(JointNodeStyleFactory);
	FEdGraphUtilities::UnregisterVisualPinFactory(JointGraphPinFactory);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(JointToolTabNames::JointManagementTab);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(JointToolTabNames::JointBulkSearchReplaceTab);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(JointToolTabNames::JointCompilerTab);

	MenuExtensibilityManager.Reset();
	RegisteredAssetTypeActions.Reset();
	ToolBarExtensibilityManager.Reset();
	JointNodeStyleFactory.Reset();
	JointGraphPinFactory.Reset();
	JointManagementTabHandler.Reset();
}

bool FJointEditorModule::SupportsDynamicReloading()
{
	return true;
}

void FJointEditorModule::RegisterAssetTools()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	EAssetTypeCategories::Type AssetCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("Joint")), FText::FromName(TEXT("Joint")));

	RegisterAssetTypeAction(AssetTools, MakeShareable(new FJointManagerActions(AssetCategory)));
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FJointFragmentActions(AssetCategory)));
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FJointBuildPresetActions(AssetCategory)));
}

void FJointEditorModule::RegisterSequencerTrack()
{
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>( "Sequencer" );

	JointNativeMovieTrackCreateEditorHandle = SequencerModule.RegisterTrackEditor( FOnCreateTrackEditor::CreateStatic( &FJointMovieTrackEditor::CreateTrackEditor ) );
}

void FJointEditorModule::UnregisterSequencerTrack()
{
	ISequencerModule& SequencerModule = FModuleManager::Get().GetModuleChecked<ISequencerModule>( "Sequencer" );

	SequencerModule.UnRegisterTrackEditor( JointNativeMovieTrackCreateEditorHandle );
}

void FJointEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	RegisteredAssetTypeActions.Add(Action);
}

void FJointEditorModule::RegisterModuleCommand()
{
	FJointEditorCommands::Get().PluginCommands->MapAction(
		FJointEditorCommands::Get().OpenJointManagementTab
		, FExecuteAction::CreateStatic(&FJointEditorModule::OpenJointManagementTab)
	);

	FJointEditorCommands::Get().PluginCommands->MapAction(
		FJointEditorCommands::Get().OpenJointBulkSearchReplaceTab
		, FExecuteAction::CreateStatic(&FJointEditorModule::OpenJointBulkSearchReplaceTab)
	);

	FJointEditorCommands::Get().PluginCommands->MapAction(
		FJointEditorCommands::Get().OpenJointCompilerTab
		, FExecuteAction::CreateStatic(&FJointEditorModule::OpenJointCompilerTab)
	);
}

void FJointEditorModule::AppendActiveJointRedirects()
{
	if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		TArray<FCoreRedirect> Redirects;

		for (const FJointCoreRedirect& CoreRedirect : Settings->JointCoreRedirects)
		{
			FCoreRedirect RawCoreRedirect(
				ECoreRedirectFlags::Type_Class,
				FJointCoreRedirectObjectName::ConvertToCoreRedirectObjectName(CoreRedirect.OldName),
				FJointCoreRedirectObjectName::ConvertToCoreRedirectObjectName(CoreRedirect.NewName));

			Redirects.Add(RawCoreRedirect);
		}

		FCoreRedirects::AddRedirectList(TArrayView<FCoreRedirect>(Redirects), "JointCoreRedirect");
	}
}

void FJointEditorModule::OpenJointManagementTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(JointToolTabNames::JointManagementTab);
}

void FJointEditorModule::OpenJointBulkSearchReplaceTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(JointToolTabNames::JointBulkSearchReplaceTab);
}

void FJointEditorModule::OpenJointCompilerTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(JointToolTabNames::JointCompilerTab);
}

TSharedRef<SDockTab> FJointEditorModule::OnSpawnJointManagementTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return JointManagementTabHandler->SpawnJointManagementTab();
}

TSharedRef<SDockTab> FJointEditorModule::OnSpawnJointBulkSearchReplaceTab(const FSpawnTabArgs& SpawnTabArgs)
{
	auto NomadTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("JointBulkSearchReplaceTabTitle", "Joint Bulk Search & Replace"));

	auto TabManager = FGlobalTabmanager::Get()->NewTabManager(NomadTab);
	TabManager->SetOnPersistLayout(
		FTabManager::FOnPersistLayout::CreateStatic(
			[](const TSharedRef<FTabManager::FLayout>& InLayout)
			{
				if (InLayout->GetPrimaryArea().Pin().IsValid())
				{
					FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, InLayout);
				}
			}
		)
	);

	NomadTab->SetTabIcon(FJointEditorStyle::Get().GetBrush("ClassIcon.JointManager"));

	NomadTab->SetOnTabClosed(
		SDockTab::FOnTabClosedCallback::CreateStatic(
			[](TSharedRef<SDockTab> Self, TWeakPtr<FTabManager> TabManager)
			{
				TSharedPtr<FTabManager> OwningTabManager = TabManager.Pin();
				if (OwningTabManager.IsValid())
				{
					FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, OwningTabManager->PersistLayout());
					OwningTabManager->CloseAllAreas();
				}
			}
			, TWeakPtr<FTabManager>(TabManager)
		)
	);

	FJointEditorModule& EditorModule = FModuleManager::GetModuleChecked<FJointEditorModule>("JointEditor");

	auto MainWidget = SNew(SJointBulkSearchReplace)
		.TabManager(TabManager);

	NomadTab->SetContent(MainWidget);
	return NomadTab;
}

TSharedRef<SDockTab> FJointEditorModule::OnSpawnJointCompilerTab(const FSpawnTabArgs& SpawnTabArgs)
{
	auto NomadTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("JointCompilerTabTitle", "Joint Compiler"));

	auto TabManager = FGlobalTabmanager::Get()->NewTabManager(NomadTab);
	TabManager->SetOnPersistLayout(
		FTabManager::FOnPersistLayout::CreateStatic(
			[](const TSharedRef<FTabManager::FLayout>& InLayout)
			{
				if (InLayout->GetPrimaryArea().Pin().IsValid())
				{
					FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, InLayout);
				}
			}
		)
	);

	NomadTab->SetTabIcon(FJointEditorStyle::Get().GetBrush("ClassIcon.JointManager"));

	NomadTab->SetOnTabClosed(
		SDockTab::FOnTabClosedCallback::CreateStatic(
			[](TSharedRef<SDockTab> Self, TWeakPtr<FTabManager> TabManager)
			{
				TSharedPtr<FTabManager> OwningTabManager = TabManager.Pin();
				if (OwningTabManager.IsValid())
				{
					FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, OwningTabManager->PersistLayout());
					OwningTabManager->CloseAllAreas();
				}
			}
			, TWeakPtr<FTabManager>(TabManager)
		)
	);

	FJointEditorModule& EditorModule = FModuleManager::GetModuleChecked<FJointEditorModule>("JointEditor");

	//auto MainWidget = SNew(SJointManagement).TabManager(TabManager);

	//NomadTab->SetContent(MainWidget);
	return NomadTab;
}

void FJointEditorModule::UnregisterAssetTools()
{
	FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");

	if (AssetToolsModule != nullptr)
	{
		IAssetTools& AssetTools = AssetToolsModule->Get();

		for (auto Action : RegisteredAssetTypeActions) { AssetTools.UnregisterAssetTypeActions(Action); }
	}
}

void FJointEditorModule::RegisterClassLayout()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	//Class Detail Customization

	PropertyModule.RegisterCustomClassLayout(FName(*UJointNodeBase::StaticClass()->GetName())
	                                         , FOnGetDetailCustomizationInstance::CreateStatic(
		                                         &FJointNodeInstanceCustomizationBase::MakeInstance));

	PropertyModule.RegisterCustomClassLayout(FName(*UJointEdGraphNode::StaticClass()->GetName())
	                                         , FOnGetDetailCustomizationInstance::CreateStatic(
		                                         &FJointEdGraphNodesCustomizationBase::MakeInstance));

	PropertyModule.RegisterCustomClassLayout(FName(*UJointEdGraph::StaticClass()->GetName())
	                                         , FOnGetDetailCustomizationInstance::CreateStatic(
		                                         &FJointEdGraphCustomization::MakeInstance));

	PropertyModule.RegisterCustomClassLayout(FName(*UJointManager::StaticClass()->GetName())
	                                         , FOnGetDetailCustomizationInstance::CreateStatic(
		                                         &FJointManagerCustomization::MakeInstance));


	PropertyModule.RegisterCustomClassLayout(FName(*UJointBuildPreset::StaticClass()->GetName())
	                                         , FOnGetDetailCustomizationInstance::CreateStatic(
		                                         &FJointBuildPresetCustomization::MakeInstance));


	PropertyModule.RegisterCustomPropertyTypeLayout(FName(*FJointNodePointer::StaticStruct()->GetName()),
	                                                FOnGetPropertyTypeCustomizationInstance::CreateStatic(
		                                                &FJointNodePointerStructCustomization::MakeInstance));
}

void FJointEditorModule::UnregisterClassLayout()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	//Class Detail Customization
	PropertyModule.UnregisterCustomClassLayout(FName(*UJointNodeBase::StaticClass()->GetName()));

	PropertyModule.UnregisterCustomClassLayout(FName(*UJointEdGraphNode::StaticClass()->GetName()));

	PropertyModule.UnregisterCustomClassLayout(FName(*UJointEdGraph::StaticClass()->GetName()));

	PropertyModule.UnregisterCustomClassLayout(FName(*UJointManager::StaticClass()->GetName()));

	PropertyModule.UnregisterCustomClassLayout(FName(*UJointBuildPreset::StaticClass()->GetName()));

	PropertyModule.UnregisterCustomPropertyTypeLayout(FName(*FJointNodePointer::StaticStruct()->GetName()));
}


void FJointEditorModule::StoreClassCaches()
{
	if (!ClassCache.IsValid())
	{
		ClassCache = MakeShareable(new FJointGraphNodeClassHelper(UJointNodeBase::StaticClass()));
		FJointGraphNodeClassHelper::AddObservedBlueprintClasses(UJointFragment::StaticClass());
		ClassCache->UpdateAvailableBlueprintClasses();
	}


	if (!EdClassCache.IsValid())
	{
		EdClassCache = MakeShareable(new FJointGraphNodeClassHelper(UJointEdGraphNode::StaticClass()));
		FJointGraphNodeClassHelper::AddObservedBlueprintClasses(UJointEdGraphNode::StaticClass());
		EdClassCache->UpdateAvailableBlueprintClasses();
	}
}

void FJointEditorModule::FlushClassCaches()
{
	ClassCache.Reset();
	EdClassCache.Reset();
}

void FJointEditorModule::RegisterMenuExtensions()
{
	MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
	ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);
}

void FJointEditorModule::UnregisterMenuExtensions()
{
	MenuExtensibilityManager.Reset();
	ToolBarExtensibilityManager.Reset();
}

void FJointEditorModule::RegisterDebugger()
{
	UnregisterDebugger();

	JointDebugger = NewObject<UJointDebugger>();

	JointDebugger->AddToRoot();
}

void FJointEditorModule::UnregisterDebugger()
{
	if (JointDebugger != nullptr && JointDebugger->IsValidLowLevel())
	{
		JointDebugger->RemoveFromRoot();

		JointDebugger = nullptr;
	}
}

TSharedPtr<FJointGraphNodeClassHelper> FJointEditorModule::GetEdClassCache()
{
	return EdClassCache;
}

TSharedPtr<FJointGraphNodeClassHelper> FJointEditorModule::GetClassCache()
{
	return ClassCache;
}

FJointEditorModule* FJointEditorModule::Get()
{
	IModuleInterface& EditorModule = FModuleManager::Get().LoadModuleChecked("JointEditor");

	FJointEditorModule* CastedModule = static_cast<FJointEditorModule*>(&EditorModule);

	return CastedModule;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJointEditorModule, JointEditor)
