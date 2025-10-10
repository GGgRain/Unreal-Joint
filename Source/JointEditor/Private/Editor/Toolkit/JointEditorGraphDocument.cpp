#include "JointEditorGraphDocument.h"

#include "BlueprintEditor.h"
#include "JointEditorToolkit.h"
#include "JointEdUtils.h"
#include "JointManager.h"
#include "SGraphPanel.h"
#include "Widgets/Docking/SDockTab.h"

#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"

#define LOCTEXT_NAMESPACE "JointEditorGraphDocument"

void FJointGraphTabHistory::EvokeHistory(TSharedPtr<FTabInfo> InTabInfo, bool bPrevTabMatches)
{
	FWorkflowTabSpawnInfo SpawnInfo;
	SpawnInfo.Payload = Payload;
	SpawnInfo.TabInfo = InTabInfo;

	if(bPrevTabMatches)
	{
		TSharedPtr<SDockTab> DockTab = InTabInfo->GetTab().Pin();
		GraphEditor = StaticCastSharedRef<SGraphEditor>(DockTab->GetContent());
	}
	else
	{
		TSharedRef< SGraphEditor > GraphEditorRef = StaticCastSharedRef< SGraphEditor >(FactoryPtr.Pin()->CreateTabBody(SpawnInfo));
		GraphEditor = GraphEditorRef;
		FactoryPtr.Pin()->UpdateTab(InTabInfo->GetTab().Pin(), SpawnInfo, GraphEditorRef);
	}
}

void FJointGraphTabHistory::SaveHistory()
{
	if (IsHistoryValid())
	{
		check(GraphEditor.IsValid());
		GraphEditor.Pin()->GetViewLocation(SavedLocation, SavedZoomAmount);
		GraphEditor.Pin()->GetViewBookmark(SavedBookmarkId);
	}
}

void FJointGraphTabHistory::RestoreHistory()
{
	if (IsHistoryValid())
	{
		check(GraphEditor.IsValid());
		GraphEditor.Pin()->SetViewLocation(SavedLocation, SavedZoomAmount, SavedBookmarkId);
	}
}



FJointGraphEditorSummoner::FJointGraphEditorSummoner(
		TSharedPtr<class FJointEditorToolkit> InJointEditorToolkitPtr,
		FOnCreateGraphEditorWidget CreateGraphEditorWidgetCallback
	) : FDocumentTabFactoryForObjects<UJointEdGraph>(EJointEditorTapIDs::GraphID, InJointEditorToolkitPtr)
	, JointEditorToolkitPtr(InJointEditorToolkitPtr)
	, OnCreateGraphEditorWidget(CreateGraphEditorWidgetCallback)
{
}

void FJointGraphEditorSummoner::OnTabActivated(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());

	if (UEdGraph* Graph = GraphEditor->GetCurrentGraph())
	{
		if (UJointEdGraph* JointGraph = Cast<UJointEdGraph>(Graph))
		{
			JointGraph->ReallocateGraphPanelToGraphNodeSlates(SharedThis(GraphEditor->GetGraphPanel())); // Notify the graph that it has been loaded (in case it needs to refresh any internal data)
		}
	}
	
	JointEditorToolkitPtr.Pin()->OnGraphEditorFocused(GraphEditor);
}

void FJointGraphEditorSummoner::OnTabBackgrounded(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	JointEditorToolkitPtr.Pin()->OnGraphEditorBackgrounded(GraphEditor);
}

void FJointGraphEditorSummoner::OnTabRefreshed(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	
	GraphEditor->NotifyGraphChanged();
}

void FJointGraphEditorSummoner::SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());

	FVector2D ViewLocation;
	float ZoomAmount;
	GraphEditor->GetViewLocation(ViewLocation, ZoomAmount);

	UJointEdGraph* Graph = Payload->IsValid() ? FTabPayload_UObject::CastChecked<UJointEdGraph>(Payload) : nullptr;

	if (Graph && JointEditorToolkitPtr.Pin()->IsGraphInCurrentJointManager(Graph))
	{
		// Don't save references to external graphs.
		JointEditorToolkitPtr.Pin()->GetJointManager()->LastEditedDocuments.Add(FEditedDocumentInfo(Graph, ViewLocation, ZoomAmount));
	}
}

TAttribute<FText> FJointGraphEditorSummoner::ConstructTabNameForObject(UJointEdGraph* DocumentID) const
{
	return TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FJointLocalKismetCallbacks::GetGraphDisplayName, (const UJointEdGraph*)DocumentID));
}

TSharedRef<SWidget> FJointGraphEditorSummoner::CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UJointEdGraph* DocumentID) const
{
	check(Info.TabInfo.IsValid());
	return OnCreateGraphEditorWidget.Execute(Info.TabInfo.ToSharedRef(), DocumentID);
}



const FSlateBrush* FJointGraphEditorSummoner::GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UJointEdGraph* DocumentID) const
{
	const FSlateBrush* IconOut = nullptr;
	
	FJointEdUtils::GetGraphIconFor(DocumentID, IconOut);
		
	return IconOut;
}

TSharedRef<FGenericTabHistory> FJointGraphEditorSummoner::CreateTabHistoryNode(TSharedPtr<FTabPayload> Payload)
{
	return MakeShareable(new FJointGraphTabHistory(SharedThis(this), Payload));
}


#undef LOCTEXT_NAMESPACE