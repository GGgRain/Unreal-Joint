// Copyright Epic Games, Inc. All Rights Reserved.


#include "EditorWidget/JointGraphEditor.h"
#include "EdGraph/EdGraph.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBox.h"
#include "GraphEditorModule.h"
#include "EditorWidget/SJointGraphEditorImpl.h"
#include "Widgets/Layout/SMissingWidget.h"

// List of all active GraphEditor wrappers
TArray< TWeakPtr<SJointGraphEditor> > SJointGraphEditor::AllInstances;



void SJointGraphEditor::ConstructImplementation( const FArguments& InArgs )
{
	FGraphEditorModule& GraphEdModule = FModuleManager::LoadModuleChecked<FGraphEditorModule>(TEXT("GraphEditor"));

	// Construct the implementation and make it the contents of this widget.
	Implementation = SNew(SJointGraphEditorImpl)
		.AdditionalCommands(InArgs._AdditionalCommands)
		.IsEditable(InArgs._IsEditable)
		.DisplayAsReadOnly(InArgs._DisplayAsReadOnly)
		.Appearance(InArgs._Appearance)
		.TitleBar(InArgs._TitleBar)
		.GraphToEdit(InArgs._GraphToEdit)
		.GraphEvents(InArgs._GraphEvents)
		.AutoExpandActionMenu(InArgs._AutoExpandActionMenu)
#if UE_VERSION_OLDER_THAN(5,1,0)
		.GraphToDiff(InArgs._GraphToDiff)
#else
		.DiffResults(InArgs._DiffResults)
#endif
		.OnNavigateHistoryBack(InArgs._OnNavigateHistoryBack)
		.OnNavigateHistoryForward(InArgs._OnNavigateHistoryForward)
		.ShowGraphStateOverlay(InArgs._ShowGraphStateOverlay);

	Implementation->AssetEditorToolkit = InArgs._AssetEditorToolkit;

	this->ChildSlot
	[
		SNew( SBox )
		.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("GraphEditorPanel")))
		[
			Implementation.ToSharedRef()
		]
	];
}


/**
 * Loads the GraphEditorModule and constructs a GraphEditor as a child of this widget.
 *
 * @param InArgs   Declaration params from which to construct the widget.
 */
void SJointGraphEditor::Construct( const FArguments& InArgs )
{
	EdGraphObj = InArgs._GraphToEdit;
	OnGraphModuleReloadedCallback = InArgs._OnGraphModuleReloaded;
	AssetEditorToolkit = InArgs._AssetEditorToolkit;

	// Register this widget with the module so that we can gracefully handle the module being unloaded.
	// See OnModuleUnloading()
	RegisterGraphEditor( SharedThis(this) );

	// Register a graph modified handler
	if (EdGraphObj != NULL)
	{
		EdGraphObj->AddOnGraphChangedHandler( FOnGraphChanged::FDelegate::CreateSP( this, &SJointGraphEditor::OnGraphChanged ) );
	}

	// Make the actual GraphEditor instance
	ConstructImplementation(InArgs);
}

// Invoked to let this widget know that the GraphEditor module is being unloaded.
void SJointGraphEditor::OnModuleUnloading()
{
	this->ChildSlot
	[
		SMissingWidget::MakeMissingWidget()
	];

	check( Implementation.IsUnique() ); 
	Implementation.Reset();
}

void SJointGraphEditor::RegisterGraphEditor( const TSharedRef<SJointGraphEditor>& InGraphEditor )
{
	// Compact the list of GraphEditor instances
	for (int32 WidgetIndex = 0; WidgetIndex < AllInstances.Num(); ++WidgetIndex)
	{
		if (!AllInstances[WidgetIndex].IsValid())
		{
			AllInstances.RemoveAt(WidgetIndex);
			--WidgetIndex;
		}
	}

	AllInstances.Add(InGraphEditor);
}

TSharedPtr<SJointGraphEditor> SJointGraphEditor::FindGraphEditorForGraph(const UEdGraph* Graph)
{
	for (TWeakPtr<SJointGraphEditor>& WeakWidget : AllInstances)
	{
		TSharedPtr<SJointGraphEditor> WidgetPtr = WeakWidget.Pin();
		if (WidgetPtr.IsValid() && (WidgetPtr->GetCurrentGraph() == Graph))
		{
			return WidgetPtr;
		}
	}

	return nullptr;
}
