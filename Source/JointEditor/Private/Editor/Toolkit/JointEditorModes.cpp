#include "JointEditorModes.h"

#include "JointEditorToolbar.h"
#include "EditorWidget/JointGraphEditor.h"
#include "EditorWidget/JointToolkitToastMessages.h"


FJointEditorApplicationMode::FJointEditorApplicationMode(TSharedPtr<class FJointEditorToolkit> InToolkit)
	: FApplicationMode(EJointEditorModes::StandaloneMode)
{
	JointEditorToolkit = InToolkit;
	
	TabLayout = FTabManager::NewLayout("Standalone_JointEditor_Layout_v9")
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

	
	//JointEditorToolkit.Pin()->Toolbar->AddModesToolbar(ToolbarExtender);
}

void FJointEditorApplicationMode::Initialize()
{
	/*
	TSharedRef<FDocumentTabFactory> GraphEditorFactory = MakeShareable(
		new FJointGraphEditorSummoner(
			JointEditorToolkit.Pin(),
			FJointGraphEditorSummoner::FOnCreateGraphEditorWidget::CreateSP(this, &FJointEditorApplicationMode::CreateGraphEditorWidget)
		));

	// Also store off a reference to the grapheditor factory so we can find all the tabs spawned by it later.
	GraphEditorTabFactoryPtr = GraphEditorFactory;
	*/
}

void FJointEditorApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	check(JointEditorToolkit.IsValid());
	TSharedPtr<FJointEditorToolkit> JointEditorToolkitPtr = JointEditorToolkit.Pin();
	
	JointEditorToolkitPtr->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	JointEditorToolkitPtr->GetDocumentManager()->RegisterDocumentFactory(GraphEditorTabFactoryPtr.Pin());

	
	JointEditorToolkitPtr->PushTabFactories(EditorTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FJointEditorApplicationMode::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();

	check(JointEditorToolkit.IsValid());
	TSharedPtr<FJointEditorToolkit> JointEditorToolkitPtr = JointEditorToolkit.Pin();
	
	JointEditorToolkitPtr->SaveEditedObjectState();
}

void FJointEditorApplicationMode::PostActivateMode()
{
	// Reopen any documents that were open when the blueprint was last saved
	check(JointEditorToolkit.IsValid());
	TSharedPtr<FJointEditorToolkit> JointEditorToolkitPtr = JointEditorToolkit.Pin();
	JointEditorToolkitPtr->RestoreEditedObjectState();

	FApplicationMode::PostActivateMode();
}

TSharedPtr<FTabManager::FLayout> FJointEditorApplicationMode::GetTabLayout()
{
	return TabLayout;
}
