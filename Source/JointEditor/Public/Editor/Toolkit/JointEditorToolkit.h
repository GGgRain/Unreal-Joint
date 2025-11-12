//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "BlueprintEditor.h"
#include "Editor/Graph/JointEdGraph.h"
#include "EditorUndoClient.h"
#include "JointEditorGraphDocument.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "Misc/NotifyHook.h"

class UJointEdGraphNode_Composite;
class UJointEdGraphNode_Tunnel;
class FTabInfo;
class SJointGraphEditor;
class FDocumentTabFactory;
class FJointEditorNodePickingManager;
class SJointToolkitToastMessageHub;
class UVoltAnimationManager;
class UJointNodeBase;
class SJointTree;
class FSpawnTabArgs;
class ISlateStyle;
class IToolkitHost;
class SDockTab;
class SJointEditorOutliner;

class UJointManager;
class UJointEdGraph;

namespace EJointEditorTapIDs
{
	static const FName AppIdentifier("JointEditorToolkit_019");
	static const FName DetailsID("Joint_DetailsID");
	static const FName PaletteID("Joint_PaletteID");
	static const FName OutlinerID("Joint_OutlinerID");
	static const FName SearchReplaceID("Joint_SearchID");
	static const FName CompileResultID("Joint_CompileResultID");
	static const FName ContentBrowserID("Joint_ContentBrowserID");
	static const FName EditorPreferenceID("Joint_EditorPreferenceID");
	static const FName GraphID("Document"); // This is the tab id used for graph documents - HARDCODED on the engine source and can't help with it our side (more specifically, it's not changeable on the lower version of the engine), Damn.
}

namespace EJointEditorModes
{
	static const FName StandaloneMode("JointEditor_StandaloneMode");
}



/**
 * Implements an Editor toolkit for textures.
 */
class JOINTEDITOR_API FJointEditorToolkit : public FWorkflowCentricApplication, public FEditorUndoClient, public FNotifyHook
{
public:

	FJointEditorToolkit();

	virtual ~FJointEditorToolkit() override;

public:

	//Initialize Joint manager editor and open up the Joint manager for the provided asset.
	void InitJointEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost,
							UJointManager* InJointManager);
	void CleanUp();

	virtual void OnClose() override;

public:
	//Editor Slate Initialization

	void InitializeDetailView();
	void InitializeContentBrowser();
	void InitializeEditorPreferenceView();
	void InitializePaletteView();
	void InitializeOutliner();
	void InitializeManagerViewer();
	void InitializeCompileResult();

	void FeedEditorSlateToEachTab() const;

public:
	
	TSharedRef<SDockTab> SpawnTab_EditorPreference(const FSpawnTabArgs& Args, FName TabIdentifier) const;
	TSharedRef<SDockTab> SpawnTab_ContentBrowser(const FSpawnTabArgs& Args, FName TabIdentifier);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args, FName TabIdentifier);
	TSharedRef<SDockTab> SpawnTab_Palettes(const FSpawnTabArgs& Args, FName TabIdentifier);
	TSharedRef<SDockTab> SpawnTab_Outliner(const FSpawnTabArgs& Args, FName TabIdentifier);
	TSharedRef<SDockTab> SpawnTab_Search(const FSpawnTabArgs& Args, FName TabIdentifier);
	TSharedRef<SDockTab> SpawnTab_CompileResult(const FSpawnTabArgs& Args, FName TabIdentifier);

public:
	//~ Begin FWorkflowCentricApplication Interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	//~ End FWorkflowCentricApplication Interface

	//~ Begin IToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	//~ End IToolkit Interface

public:
	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

public:

	void OnBeginPIE(bool bArg);
	void OnEndPIE(bool bArg);
	
public:

	void BindGraphEditorCommands();
	void BindDebuggerCommands();

public:
	
	void ExtendToolbar();

	virtual void RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager);

public:
	//Change the Joint manager we are editing with the current editor.
	void SetJointManagerBeingEdited(UJointManager* NewManager);

public:
	
	void RequestManagerViewerRefresh();

public:

	/**
	 * Get the Joint manager that the toolkit is currently editing.
	 * @return the Joint manager that the toolkit is currently editing.
	 */
	UJointManager* GetJointManager() const;

public:

	//Graph Object related

	/**
	 * Get the main graph that the toolkit is currently editing.
	 * @return the main graph that the toolkit is currently editing. You can access sub graphs from the main graph.
	 */
	UJointEdGraph* GetMainJointGraph() const;

	/**
	 * Get the currently focused graph that the toolkit is editing.
	 * @return the currently focused graph that the toolkit is editing. You can access sub graphs from the main graph.
	 */
	UJointEdGraph* GetFocusedJointGraph() const;

	//@note : if you're looking for a function that can return all graphs, please use FJointEdUtils::GetAllGraphsFrom()
	
	bool IsGraphInCurrentJointManager(UEdGraph* Graph);

private:

	/**
	 * Create a new root graph for the current Joint manager if it doesn't have one.
	 * We need to create a new graph for editing on the toolkit side - the asset may not have any graph in some unfortunate cases.
	 */
	void CreateNewRootGraphForJointManagerIfNeeded() const;

public:

	// Document management
	
	void InitializeDocumentManager();
	
public:

	TSharedPtr<SDockTab> OpenDocument(UObject* DocumentID, FDocumentTracker::EOpenDocumentCause OpenMode = FDocumentTracker::OpenNewDocument);

	void CloseDocumentTab(UObject* DocumentID);
	
	void SaveEditedObjectState();

	/** Create new tab for each element of LastEditedObjects array */
	void RestoreEditedObjectState();

public:
	
	void RefreshJointEditorOutliner();
	void CleanInvalidDocumentTabs();

public:

	TSharedPtr<FDocumentTracker> GetDocumentManager() const;
	
private:

	// Document tracker for managing editor tabs.
	TSharedPtr<class FDocumentTracker> DocumentManager;

public:
	
	/**
	 * Callbacks for graph editor events.
	 */

	TSharedRef<SGraphEditor> CreateGraphEditorWidget(TSharedRef<FTabInfo> TabInfo, UJointEdGraph* EdGraph);
	
	void OnGraphEditorFocused(TSharedRef<SGraphEditor> GraphEditor);
	
	void OnGraphEditorBackgrounded(TSharedRef<SGraphEditor> GraphEditor);
	
	void OnGraphEditorTabClosed(TSharedRef<SDockTab> DockTab);

	void OnGraphEditorNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);
	
	void OnGraphSelectedNodesChanged(UEdGraph* InGraph, const TSet<class UObject*>& NewSelection);

public:
	
	void NotifySelectionChangeToNodeSlates(UEdGraph* InGraph, const TSet<class UObject*>& NewSelection) const;
	
public:

	FGraphPanelSelectionSet GetSelectedNodes() const;
	
public:

	TWeakPtr<FDocumentTabFactory> GraphEditorTabFactoryPtr;

public:

	/** Get the currently focused graph editor, if any */
	TSharedPtr<SJointGraphEditor> GetFocusedGraphEditor() const;
	
private:

	TWeakPtr<SJointGraphEditor> FocusedGraphEditorPtr;

public:
	
	TSharedPtr<class IDetailsView> EditorPreferenceViewPtr;
	TSharedPtr<class SJointList> ContentBrowserPtr;
	TSharedPtr<class IDetailsView> DetailsViewPtr;
	TSharedPtr<class SJointManagerViewer> ManagerViewerPtr;
	TSharedPtr<class SJointFragmentPalette> JointFragmentPalettePtr;
	TSharedPtr<class SJointEditorOutliner> JointEditorOutlinerPtr;

	TSharedPtr<class FJointEditorToolbar> Toolbar;

public:
	
	TSharedPtr<class SJointToolkitToastMessageHub> GetOrCreateGraphToastMessageHub();
	TSharedPtr<class SJointToolkitToastMessageHub> GetGraphToastMessageHub() const;

	void CleanUpGraphToastMessageHub();

public:

	TSharedPtr<SJointToolkitToastMessageHub> GraphToastMessageHub;

private:
	
	TWeakObjectPtr<UJointManager> JointManager;

public:
	
	void OnDebuggingJointInstanceChanged(TWeakObjectPtr<AJointActor> WeakObject);
	
	void SetDebuggingJointInstance(TWeakObjectPtr<AJointActor> InDebuggingJointInstance);

	TWeakObjectPtr<AJointActor> GetDebuggingJointInstance();

public:

	//Debugger related

	TSharedRef<class SWidget> OnGetDebuggerActorsMenu();
	
	FText GetDebuggerActorDesc() const;
	
	void OnDebuggerActorSelected(TWeakObjectPtr<AJointActor> InstanceToDebug);


private:

	/**
	 * Debugging instance that has been selected from the toolkit.
	 */
	TWeakObjectPtr<AJointActor> DebuggingJointInstance;

public:
	
	/**
	 * Compile all graphs that the toolkit is currently editing.
	 */
	void CompileAllJointGraphs();

	/**
	 * Check whether all graphs that the toolkit is currently editing can be compiled.
	 * @return true if all graphs that the toolkit is currently editing can be compiled, false otherwise.
	 */
	bool CanCompileAllJointGraphs();

	void OnCompileJointGraphFinished(const UJointEdGraph::FJointGraphCompileInfo& CompileInfo) const;
	
	void OnCompileResultTokenClicked(const TSharedRef<IMessageToken>& MessageToken);

public:

	/**
	 * Get the node picking manager for the editor.
	 * @return the node picking manager for the editor.
	 */
	TSharedPtr<FJointEditorNodePickingManager> GetNodePickingManager() const;

	TSharedPtr<FJointEditorNodePickingManager> GetOrCreateNodePickingManager();

private:

	/**
	 * Node picking manager for the editor.
	 * Access this manager to perform node picking actions.
	 */
	TSharedPtr<FJointEditorNodePickingManager> NodePickingManager;
	

public:

	// Delegates for graph editor commands
	
	void SelectAllNodes();
	bool CanSelectAllNodes() const;

	void CopySelectedNodes();
	bool CanCopyNodes() const;

	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);
	
	bool CanPasteNodes() const;
	
	void CutSelectedNodes();
	bool CanCutNodes() const;

	void DuplicateNodes();
	bool CanDuplicateNodes() const;

	void RenameNodes();
	bool CanRenameNodes() const;

	void DeleteSelectedNodes();
	void DeleteSelectedDuplicatableNodes();
	bool CanDeleteNodes() const;
	
	void OnCreateComment();
	void OnCreateFoundation();

	bool CanJumpToSelection();
	void OnJumpToSelection();

	/** Called when a selection of nodes are being collapsed into a sub-graph */
	void OnCollapseSelectionToSubGraph();
	bool CanCollapseSelectionToSubGraph() const;

	void CollapseNodes(TSet<class UEdGraphNode*>& InCollapsableNodes);
	void CollapseNodesIntoGraph(UJointEdGraphNode_Composite* InGatewayNode, UJointEdGraphNode_Tunnel* InEntryNode, UJointEdGraphNode_Tunnel* InResultNode, UEdGraph* InSourceGraph, UEdGraph* InDestinationGraph, TSet<UEdGraphNode*>& InCollapsableNodes, bool bCanDiscardEmptyReturnNode, bool bCanHaveWeakObjPtrParam);

	void MoveNodesToGraph(TArray<TObjectPtr<class UEdGraphNode>>& SourceNodes, UEdGraph* DestinationGraph, TSet<UEdGraphNode*>& OutExpandedNodes, UEdGraphNode** OutEntry, UEdGraphNode** OutResult, const bool bIsCollapsedGraph);

	/** Called when a selection of nodes are being collapsed into a sub-graph */
	void OnExpandNodes();
	bool CanExpandNodes() const;
	
	void ExpandNode(UEdGraphNode* InNodeToExpand, UEdGraph* InSourceGraph, TSet<UEdGraphNode*>& OutExpandedNodes);
	
	void ToggleShowNormalConnection();
	bool IsShowNormalConnectionChecked() const;

	void ToggleShowRecursiveConnection();
	bool IsShowRecursiveConnectionChecked() const;

public:

	//Breakpoint action

	void OnEnableBreakpoint();
	bool CanEnableBreakpoint() const;
	
	void OnToggleBreakpoint();
	bool CanToggleBreakpoint() const;
	
	void OnDisableBreakpoint();
	bool CanDisableBreakpoint() const;
	
	void OnAddBreakpoint();
	bool CanAddBreakpoint() const;
	
	void OnRemoveBreakpoint();
	bool CanRemoveBreakpoint() const;

public:

	void OnRemoveAllBreakpoints();
	bool CanRemoveAllBreakpoints() const;
	
	void OnEnableAllBreakpoints();
	bool CanEnableAllBreakpoints() const;

	void OnDisableAllBreakpoints();
	bool CanDisableAllBreakpoints() const;
	
	void OnToggleDebuggerExecution();
	bool GetCheckedToggleDebuggerExecution() const;

	void OnDissolveSubNode();
	bool CheckCanDissolveSubNode() const;

	void OnSolidifySubNode();
	bool CheckCanSolidifySubNode() const;

	void OnToggleVisibilityChangeModeForSimpleDisplayProperty();
	bool GetCheckedToggleVisibilityChangeModeForSimpleDisplayProperty() const;

public:
	
	void OpenSearchTab() const;

	void OpenReplaceTab() const;

public:
	
	void PopulateNodePickingToastMessage();

	void PopulateTransientEditingWarningToastMessage();

	void PopulateVisibilityChangeModeForSimpleDisplayPropertyToastMessage();

	void PopulateNodePickerCopyToastMessage();
	
	void PopulateNodePickerPastedToastMessage();

	void PopulateNeedReopeningToastMessage();
	
	
	void ClearNodePickingToastMessage() const;

	void ClearTransientEditingWarningToastMessage() const;

	void ClearVisibilityChangeModeForSimpleDisplayPropertyToastMessage() const;

	void ClearNodePickerCopyToastMessage() const;

	void ClearNodePickerPastedToastMessage() const;

public:

	FGuid NodePickingToastMessageGuid;
	
	FGuid TransientEditingToastMessageGuid;
	
	FGuid VisibilityChangeModeForSimpleDisplayPropertyToastMessageGuid;

	FGuid NodePickerCopyToastMessageGuid;

	FGuid NodePickerPasteToastMessageGuid;
	
	FGuid RequestReopenToastMessageGuid;

public:

	void OnContentBrowserAssetDoubleClicked(const FAssetData& AssetData);
	
private:

	bool bIsOnVisibilityChangeModeForSimpleDisplayProperty = false;

public:

	//Convenient Editor Action.

	/**
	 * Highlight the provided node on the graph.
	 * It's just a single blink animation :D
	 * @param NodeToHighlight The target node to highlight.
	 * @param bBlinkForOnce Whether to play the animation for once.
	 */
	void StartHighlightingNode(class UJointEdGraphNode* NodeToHighlight, bool bBlinkForOnce);

	/**
	 * Stop highlighting the provided node on the graph.
	 * @param NodeToHighlight The target node to stop highlight.
	 */
	void StopHighlightingNode(class UJointEdGraphNode* NodeToHighlight);

private:

	//hyperlink related
	
	/**
	 * Move the viewport to the provided node to make it be centered.
	 */
	void JumpToNode(UEdGraphNode* Node, bool bRequestRename = false);

public:

	void JumpToHyperlink(UObject* ObjectReference, bool bRequestRename = false);

	/**
	 * Select provided object on graph panel and detail panel.
	 * Only be selected when if the object is UEdGraphNode type.
	 * 
	 * This will trigger OnSelectedNodesChanged.
	 * 
	 * @param NewSelection New selections to feed the graph panel. 
	 */
	void SelectProvidedObjectOnGraph(TSet<class UObject*> NewSelection);
	
	/**
	 * Select provided object on detail tab.
	 * @param NewSelection New selections to feed the detail tab. 
	 */
	void SelectProvidedObjectOnDetail(const TSet<class UObject*>& NewSelection);

public:

	void MoveNodesToAveragePos(TSet<UEdGraphNode*>& AverageNodes, FVector2D SourcePos, bool bExpandedNodesNeedUniqueGuid = false) const;

public:
	
	// Check if we are currently in Play In Editor mode.
	bool IsPlayInEditorActive() const;
	
	// Check if the current editor is in editing mode.
	bool IsInEditingMode() const;
	
};
