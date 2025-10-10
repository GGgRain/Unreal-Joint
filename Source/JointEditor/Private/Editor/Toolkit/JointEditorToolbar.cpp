//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointEditorToolbar.h"

#include "Misc/Attribute.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"

#include "Widgets/SWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "JointEditorToolkit.h"
#include "JointEditorStyle.h"
#include "JointEditorCommands.h"
#include "JointEdUtils.h"

#include "Modules/ModuleManager.h"
#include "ClassViewerModule.h"
#include "JointManager.h"
#include "JointEditor.h"
#include "Debug/JointDebugger.h"
#include "Widgets/Input/SComboButton.h"

#define LOCTEXT_NAMESPACE "JointEditorToolbar"

class FAssetToolsModule;

class SJointModeSeparator : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SJointModeSeparator)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArg)
	{
		SBorder::Construct(
			SBorder::FArguments()
			.BorderImage(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("BlueprintEditor.PipelineSeparator"))
			.Padding(0.0f)
		);
	}

	// SWidget interface
	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const float Height = 20.0f;
		const float Thickness = 16.0f;
		return FVector2D(Thickness, Height);
	}

	// End of SWidget interface
};

void FJointEditorToolbar::AddDebuggerToolbar(TSharedPtr<FExtender> Extender)
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<FJointEditorToolkit> JointEditor)
		{
			TSharedPtr<FJointEditorToolkit> JointEditorPtr = JointEditor.Pin();

			ToolbarBuilder.BeginSection("Debug");
			ToolbarBuilder.BeginStyleOverride("CurveEditorToolbar");
			//No other way to implement the style in the system. WHY, WHY? EPIC!!!!!!
			{
				ToolbarBuilder.AddToolBarButton(
					FJointDebuggerCommands::Get().PausePlaySession,
					NAME_None,
					FText::GetEmpty(),
					LOCTEXT("PausePlaySessionToolTip", "Pause simulation."),
					FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "PlayWorld.PausePlaySession")
				);

				ToolbarBuilder.AddToolBarButton(
					FJointDebuggerCommands::Get().ResumePlaySession,
					NAME_None,
					FText::GetEmpty(),
					LOCTEXT("ResumePlaySessionToolTip", "Resume simulation."),
					FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "PlayWorld.ResumePlaySession")
				);

				ToolbarBuilder.AddToolBarButton(
					FJointDebuggerCommands::Get().StopPlaySession,
					NAME_None,
					FText::GetEmpty(),
					LOCTEXT("StopPlaySessionToolTip", "Stop simulation."),
					FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "PlayWorld.StopPlaySession")
				);

				const TAttribute<EVisibility> SelectionBoxVisibility = TAttribute<EVisibility>::CreateLambda([]
					{
						return UJointDebugger::IsPIESimulating() ? EVisibility::Visible : EVisibility::Collapsed;
					});

				TSharedRef<SWidget> SelectionBox = SNew(SComboButton)
					.Visibility(SelectionBoxVisibility)
					.OnGetMenuContent(JointEditorPtr.Get(), &FJointEditorToolkit::OnGetDebuggerActorsMenu)
					.ButtonContent()
					[
						SNew(STextBlock)
						.ToolTipText(LOCTEXT("SelectDebugActor", "Pick actor to debug"))
						.Text(JointEditorPtr.Get(), &FJointEditorToolkit::GetDebuggerActorDesc)
					];


				ToolbarBuilder.AddToolBarButton(
					FJointDebuggerCommands::Get().ForwardInto,
					NAME_None,
					FText::GetEmpty(),
					LOCTEXT("ForwardIntoToolTip",
					        "Resume and break again when met the next node. Can go into the sub trees."),
					FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "PlayWorld.StepInto")
				);
				ToolbarBuilder.AddToolBarButton(
					FJointDebuggerCommands::Get().ForwardOver,
					NAME_None,
					FText::GetEmpty(),
					LOCTEXT("ForwardOverToolTip",
					        "Resume and break again when met the next node. Can not go into the sub trees."),
					FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "PlayWorld.StepOver")
				);
				ToolbarBuilder.AddToolBarButton(
					FJointDebuggerCommands::Get().StepOut,
					NAME_None,
					FText::GetEmpty(),
					LOCTEXT("StepOutToolTip",
					        "Resume and break again when met the next node. Must be another base node."),
					FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "PlayWorld.StepOut")
				);
				ToolbarBuilder.AddWidget(SelectionBox);
			}
			ToolbarBuilder.EndSection();
			ToolbarBuilder.EndStyleOverride();


			/*
			FToolMenuEntry ShowCurrentStatementEntry = FToolMenuEntry::InitToolBarButton(FPlayWorldCommands::Get().ShowCurrentStatement, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("ShowCurrentStatement")));
			ShowCurrentStatementEntry.StyleNameOverride = FName("Toolbar.BackplateLeft");

			FToolMenuEntry StepIntoEntry = FToolMenuEntry::InitToolBarButton(FPlayWorldCommands::Get().StepInto, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("StepInto")));
			StepIntoEntry.StyleNameOverride = FName("Toolbar.BackplateCenter");

			FToolMenuEntry StepOverEntry = FToolMenuEntry::InitToolBarButton(FPlayWorldCommands::Get().StepOver, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("StepOver")));
			StepOverEntry.StyleNameOverride = FName("Toolbar.BackplateCenter");

			FToolMenuEntry StepOutEntry = FToolMenuEntry::InitToolBarButton(FPlayWorldCommands::Get().StepOut, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("StepOut")));
			StepOutEntry.StyleNameOverride = FName("Toolbar.BackplateRight");

			InSection.AddEntry(ShowCurrentStatementEntry);

			InSection.AddEntry(StepIntoEntry);

			InSection.AddEntry(StepOverEntry);

			InSection.AddEntry(StepOutEntry);*/

			//ComboBox for the debugging instance selection.
		}
	};

	TSharedPtr<FJointEditorToolkit> JointEditorPtr = JointEditor.Pin();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, JointEditorPtr->GetToolkitCommands(),
	                                     FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, JointEditor));
	JointEditorPtr->AddToolbarExtender(ToolbarExtender);
}


void FJointEditorToolbar::AddJointToolbar(TSharedPtr<FExtender> Extender)
{
	check(JointEditor.IsValid());
	TSharedPtr<FJointEditorToolkit> JointEditorPtr = JointEditor.Pin();

	TSharedPtr<FExtender> BeforeToolbarExtender = MakeShareable(new FExtender);
	BeforeToolbarExtender->AddToolBarExtension("Asset"
	                                           , EExtensionHook::Before
	                                           , JointEditorPtr->GetToolkitCommands()
	                                           , FToolBarExtensionDelegate::CreateSP(
		                                           this, &FJointEditorToolbar::FillJointToolbar_BeforeAsset));
	JointEditorPtr->AddToolbarExtender(BeforeToolbarExtender);

	TSharedPtr<FExtender> AfterToolbarExtender = MakeShareable(new FExtender);
	AfterToolbarExtender->AddToolBarExtension("Asset"
	                                          , EExtensionHook::After
	                                          , JointEditorPtr->GetToolkitCommands()
	                                          , FToolBarExtensionDelegate::CreateSP(
		                                          this, &FJointEditorToolbar::FillJointToolbar_AfterAsset));
	JointEditorPtr->AddToolbarExtender(AfterToolbarExtender);
}

void FJointEditorToolbar::AddEditorModuleToolbar(TSharedPtr<FExtender> Extender)
{
	check(JointEditor.IsValid());
	TSharedPtr<FJointEditorToolkit> JointEditorPtr = JointEditor.Pin();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("JointDebugger"
	                                     , EExtensionHook::After
	                                     , FJointEditorCommands::Get().PluginCommands
	                                     , FToolBarExtensionDelegate::CreateSP(
		                                     this, &FJointEditorToolbar::FillEditorModuleToolbar));
	JointEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FJointEditorToolbar::FillJointToolbar_BeforeAsset(FToolBarBuilder& ToolbarBuilder)
{
	check(JointEditor.IsValid());
	TSharedPtr<FJointEditorToolkit> JointEditorPtr = JointEditor.Pin();

	const FJointEditorCommands& Commands = FJointEditorCommands::Get();

	ToolbarBuilder.BeginSection("Joint");
	{
		const TAttribute<FSlateIcon> CompileIcon = TAttribute<FSlateIcon>::CreateLambda([this]
			{
				static const FName CompileStatusBackground("Blueprint.CompileStatus.Background");
				static const FName CompileStatusUnknown("Blueprint.CompileStatus.Overlay.Unknown");
				static const FName CompileStatusError("Blueprint.CompileStatus.Overlay.Error");
				static const FName CompileStatusGood("Blueprint.CompileStatus.Overlay.Good");
				static const FName CompileStatusWarning("Blueprint.CompileStatus.Overlay.Warning");
			
				UJointManager* BlueprintObj = JointEditor.Pin()->GetJointManager();
			
				if(!BlueprintObj) return FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), CompileStatusUnknown, NAME_None, CompileStatusUnknown);
			
				EBlueprintStatus Status = BlueprintObj->Status;

				switch (Status)
				{
				default:
				case BS_Unknown:
				case BS_Dirty:
					return FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), CompileStatusBackground, NAME_None,
					                  CompileStatusUnknown);
				case BS_Error:
					return FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), CompileStatusBackground, NAME_None,
					                  CompileStatusError);
				case BS_UpToDate:
					return FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), CompileStatusBackground, NAME_None,
					                  CompileStatusGood);
				case BS_UpToDateWithWarnings:
					return FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), CompileStatusBackground, NAME_None,
					                  CompileStatusWarning);
				}
			});

		const TAttribute<FText> CompileTooltip = TAttribute<FText>::CreateLambda([this]
			{
				UJointManager* BlueprintObj = JointEditor.Pin()->GetJointManager();

				if(!BlueprintObj) return LOCTEXT("Recompile_Status", "Unknown status; should recompile");
			
				EBlueprintStatus Status = BlueprintObj->Status;
			
				switch (Status)
				{
				default:
				case BS_Unknown:
					return LOCTEXT("Recompile_Status", "Unknown status; should recompile");
				case BS_Dirty:
					return LOCTEXT("Dirty_Status", "Dirty; needs to be recompiled");
				case BS_Error:
					return LOCTEXT("CompileError_Status",
					               "There was an error during compilation, see the log for details");
				case BS_UpToDate:
					return LOCTEXT("GoodToGo_Status", "Good to go");
				case BS_UpToDateWithWarnings:
					return LOCTEXT("GoodToGoWarning_Status",
					               "There was a warning during compilation, see the log for details");
				}
			});


		ToolbarBuilder.AddToolBarButton(
			Commands.CompileJoint
			, NAME_None
			, TAttribute<FText>()
			, CompileTooltip
			//TAttribute<FText>(BlueprintEditorToolbar.ToSharedRef(), &FBlueprintEditorToolbar::GetStatusTooltip),

			, CompileIcon
		);
	}
	ToolbarBuilder.EndSection();
}

void FJointEditorToolbar::FillJointToolbar_AfterAsset(FToolBarBuilder& ToolbarBuilder)
{
	check(JointEditor.IsValid());
	TSharedPtr<FJointEditorToolkit> JointEditorPtr = JointEditor.Pin();

	const FJointEditorCommands& Commands = FJointEditorCommands::Get();

	ToolbarBuilder.BeginSection("JointEditor");
	{
		const FText VisibilityLabel = LOCTEXT("Visibility_Label", "Visibility");
		const FText VisibilityTooltip = LOCTEXT("Visibility_ToolTip", "Toggle the visibilities.");
		const FSlateIcon VisibilityTaskIcon = FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(),
		                                                 "LevelEditor.ViewOptions");

		ToolbarBuilder.AddComboButton(
			FUIAction(
				FExecuteAction()
			)
			, FOnGetContent::CreateSP(this, &FJointEditorToolbar::GenerateVisibilityMenu)
			, VisibilityLabel
			, VisibilityTooltip
			, VisibilityTaskIcon
		);

		const FText SearchReplaceLabel = LOCTEXT("SearchReplace_Label", "Search & Replace");
		const FText SearchReplaceTooltip = LOCTEXT("SearchReplace_ToolTip", "Open the Search & Replace tab.");
		const FSlateIcon SearchReplaceTaskIcon = FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(),
		                                                    "LevelEditor.Tabs.Outliner");

		ToolbarBuilder.AddToolBarButton(
			FUIAction(
				FExecuteAction::CreateSP(this, &FJointEditorToolbar::Task_HandleOpenSearchReplaceTab)
			)
			, NAME_None
			, SearchReplaceLabel
			, SearchReplaceTooltip
			, SearchReplaceTaskIcon
		);
	}
	ToolbarBuilder.EndSection();


	ToolbarBuilder.BeginSection("JointAssets");
	{
		const FText AssetLabel = LOCTEXT("Asset_Label", "Create Joint Asset");
		const FText AssetTooltip = LOCTEXT("Asset_ToolTip", "Create a new Joint asset with selected type.");
		const FSlateIcon AssetTaskIcon = FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(),
		                                            "MainFrame.PackageProject");

		ToolbarBuilder.AddComboButton(
			FUIAction(
				FExecuteAction()
				, FCanExecuteAction::CreateSP(this, &FJointEditorToolbar::Task_CanCreateNewJointAsset)
				, FIsActionChecked()
			)
			, FOnGetContent::CreateSP(this, &FJointEditorToolbar::Task_HandleCreateNewJointAsset)
			, AssetLabel
			, AssetTooltip
			, AssetTaskIcon
		);
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("JointDebugger");
	{
		const FText DebuggingLabel = LOCTEXT("Debugging_Label", "Debugging");
		const FText DebuggerTooltip = LOCTEXT("Debugging_ToolTip", "Options related to the debugging.");
		const FSlateIcon DebuggingTaskIcon = FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(),
		                                                "Debug");

		ToolbarBuilder.AddComboButton(
			FUIAction(
				FExecuteAction()
				, FIsActionChecked()
			)
			, FOnGetContent::CreateSP(this, &FJointEditorToolbar::GenerateDebuggerMenu)
			, DebuggingLabel
			, DebuggerTooltip
			, DebuggingTaskIcon
		);
	}
	ToolbarBuilder.EndSection();
}


void FJointEditorToolbar::FillEditorModuleToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(JointEditor.IsValid());
	TSharedPtr<FJointEditorToolkit> JointEditorPtr = JointEditor.Pin();

	ToolbarBuilder.BeginSection("EditorModule");
	{
		const FText JointToolsLabel = LOCTEXT("JointTools_Label", "Joint Tools");
		const FText JointToolsTooltip = LOCTEXT("JointTools_ToolTip", "Open Joint tools.");
		const FSlateIcon JointToolsTaskIcon = FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "DeveloperTools.MenuIcon");

		ToolbarBuilder.AddComboButton(
			FUIAction(
				FExecuteAction()
			)
			, FOnGetContent::CreateSP(this, &FJointEditorToolbar::GenerateJointToolsMenu)
			, JointToolsLabel
			, JointToolsTooltip
			, JointToolsTaskIcon
		);
		
	}
	ToolbarBuilder.EndSection();
}

TSharedRef<SWidget> FJointEditorToolbar::GenerateDebuggerMenu() const
{
	TSharedPtr<FJointEditorToolkit> JointEditorPtr = JointEditor.Pin();

	FMenuBuilder ShowMenuBuilder(false, JointEditorPtr.Get()->GetToolkitCommands());
	{
		const FJointEditorCommands& Commands = FJointEditorCommands::Get();

		ShowMenuBuilder.AddMenuEntry(Commands.RemoveAllBreakpoints);
		ShowMenuBuilder.AddMenuEntry(Commands.DisableAllBreakpoints);
		ShowMenuBuilder.AddMenuEntry(Commands.EnableAllBreakpoints);

		ShowMenuBuilder.AddMenuSeparator("Debugger");

		ShowMenuBuilder.AddMenuEntry(Commands.ToggleDebuggerExecution);
		
	}

	return ShowMenuBuilder.MakeWidget();
}

#include "Toolkit/JointEditorCommands.h"

TSharedRef<SWidget> FJointEditorToolbar::GenerateVisibilityMenu() const
{
	TSharedPtr<FJointEditorToolkit> JointEditorPtr = JointEditor.Pin();

	FMenuBuilder ShowMenuBuilder(false, JointEditorPtr.Get()->GetToolkitCommands());
	{
		const FJointEditorCommands& Commands = FJointEditorCommands::Get();

		ShowMenuBuilder.AddMenuEntry(Commands.SetShowNormalConnection);
		ShowMenuBuilder.AddMenuEntry(Commands.SetShowRecursiveConnection);
		ShowMenuBuilder.AddMenuEntry(Commands.ShowIndividualVisibilityButtonForSimpleDisplayProperty);
	}

	return ShowMenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FJointEditorToolbar::GenerateJointToolsMenu() const
{

	FMenuBuilder ShowMenuBuilder(true, FJointEditorCommands::Get().PluginCommands.ToSharedRef());
	{
		const FJointEditorCommands& Commands = FJointEditorCommands::Get();
		
		ShowMenuBuilder.AddMenuEntry(Commands.OpenJointManagementTab);
		ShowMenuBuilder.AddMenuEntry(Commands.OpenJointBulkSearchReplaceTab);
	}

	return ShowMenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FJointEditorToolbar::Task_HandleCreateNewJointAsset() const
{
	if (!JointEditor.IsValid()) return SNullWidget::NullWidget;

	FClassViewerInitializationOptions Options;
	Options.bShowUnloadedBlueprints = true;
	
#if UE_VERSION_OLDER_THAN(5,0,0)
	Options.ClassFilter = MakeShareable(new FJointEdUtils::FJointAssetFilter);
#else
	Options.ClassFilters.Add(MakeShareable(new FJointEdUtils::FJointAssetFilter));
#endif
	

	const FOnClassPicked OnPicked(FOnClassPicked::CreateLambda([this](UClass* ChosenClass)
		{
			if (JointEditor.IsValid())
			{
				FString BasePath;

				if (JointEditor.Pin()->GetJointManager() && JointEditor.Pin()->GetJointManager()->GetOutermostObject())
				{
					BasePath = JointEditor.Pin()->GetJointManager()->GetOutermostObject()->GetPathName();
				}
				
				FJointEdUtils::HandleNewAssetActionClassPicked(BasePath, ChosenClass);
			}
		}));

	return FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, OnPicked);
}

bool FJointEditorToolbar::Task_CanCreateNewJointAsset() { return !UJointDebugger::IsPIESimulating(); }

void FJointEditorToolbar::Task_HandleOpenSearchReplaceTab()
{
	if (JointEditor.IsValid()) JointEditor.Pin()->OpenSearchTab();
}


#undef LOCTEXT_NAMESPACE
