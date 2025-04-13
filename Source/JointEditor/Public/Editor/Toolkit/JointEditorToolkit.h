//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "Editor/Graph/JointEdGraph.h"
#include "EditorUndoClient.h"
#include "VoltAnimationTrack.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "Misc/NotifyHook.h"

class FJointEditorNodePickingManager;
class SJointToolkitToastMessageHub;
class UVoltAnimationManager;
class UJointNodeBase;
class SJointTree;
class FSpawnTabArgs;
class ISlateStyle;
class IToolkitHost;
class SDockTab;

class UJointManager;
class UJointEdGraph;

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
	void InitializeManagerViewer();
	void InitializeGraphEditor();
	void InitializeCompileResult();

	void FeedEditorSlateToEachTab() const;

public:
	TSharedRef<SDockTab> SpawnTab_EditorPreference(const FSpawnTabArgs& Args, FName TabIdentifier) const;
	TSharedRef<SDockTab> SpawnTab_ContentBrowser(const FSpawnTabArgs& Args, FName TabIdentifier);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args, FName TabIdentifier);
	TSharedRef<SDockTab> SpawnTab_Palettes(const FSpawnTabArgs& Args, FName TabIdentifier);
	TSharedRef<SDockTab> SpawnTab_Graph(const FSpawnTabArgs& Args, FName TabIdentifier);
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
	
	void CreateNewGraphForEditingJointManagerIfNeeded() const;

	void InitializeGraph();

	void OnGraphRequestUpdate();

public:

	/**
	 * Get the graph that the toolkit is currently editing.
	 * @return the graph that the toolkit is currently editing.
	 */
	UJointEdGraph* GetJointGraph() const;
	
	/**
	 * Get the Joint manager that the toolkit is currently editing.
	 * @return the Joint manager that the toolkit is currently editing.
	 */
	UJointManager* GetJointManager() const;

public:
	
	TSharedPtr<class IDetailsView> EditorPreferenceViewPtr;
	TSharedPtr<class SJointList> ContentBrowserPtr;
	TSharedPtr<class SJointGraphEditor> GraphEditorPtr;
	TSharedPtr<class IDetailsView> DetailsViewPtr;
	TSharedPtr<class SJointManagerViewer> ManagerViewerPtr;
	TSharedPtr<class SWidget> JointFragmentPalettePtr;
	
	TSharedPtr<class FJointEditorToolbar> Toolbar;

public:

	TSharedPtr<SJointToolkitToastMessageHub> GraphToastMessageHub;

private:
	
	TWeakObjectPtr<UJointManager> JointManager;

	FOnGraphRequestUpdate::FDelegate GraphRequestUpdateDele;
	
public:
	
	void OnDebuggingJointInstanceChanged(TWeakObjectPtr<AJointActor> WeakObject);
	
	void SetDebuggingJointInstance(TWeakObjectPtr<AJointActor> InDebuggingJointInstance);

	TWeakObjectPtr<AJointActor> GetDebuggingJointInstance();

private:

	/**
	 * Debugging instance that has been selected from the toolkit.
	 */
	TWeakObjectPtr<AJointActor> DebuggingJointInstance;

protected:

	/** Handle to the registered OnClassListUpdated delegate */
	FDelegateHandle OnClassListUpdatedDelegateHandle;
	
public:

	void HandleNewAssetActionClassPicked(UClass* InClass) const;
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);
	bool OnVerifyNodeTitleChanged(const FText& InText,UEdGraphNode* NodeBeingEdited, FText& OutErrorMessage);
	virtual void OnClassListUpdated();

public:
	//Toolkit actions

	FGraphPanelSelectionSet GetSelectedNodes() const;
	
	void OnGraphSelectedNodesChanged(const TSet<class UObject*>& NewSelection);
	
	void NotifySelectionChangeToNodeSlates(const TSet<class UObject*>& NewSelection) const;
	
public:
	
	void OnCompileJoint();
	bool CanCompileJoint();

	void OnCompileJointFinished(const UJointEdGraph::FJointGraphCompileInfo& CompileInfo) const;
	void OnCompileResultTokenClicked(const TSharedRef<IMessageToken>& MessageToken);

public:

	// Delegates for graph editor commands
	
	void SelectAllNodes();
	bool CanSelectAllNodes() const;

	void CopySelectedNodes();
	bool CanCopyNodes() const;

	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);
	bool CanPasteNodes() const;
	virtual void FixupPastedNodes(const TSet<UEdGraphNode*>& NewPastedGraphNodes, const TMap<FGuid/*New*/, FGuid/*Old*/>& NewToOldNodeMapping);
	
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

public:

	void OnBeginPIE(bool bArg);
	
	void OnEndPIE(bool bArg);

public:

	void OnContentBrowserAssetDoubleClicked(const FAssetData& AssetData);
	
private:

	/**
	 * Node picking manager for the editor.
	 * Access this manager to perform node picking actions.
	 */
	TSharedPtr<FJointEditorNodePickingManager> NodePickingManager;
	
public:

	/**
	 * Get the node picking manager for the editor.
	 * @return the node picking manager for the editor.
	 */
	TSharedPtr<FJointEditorNodePickingManager> GetNodePickingManager() const;

	void AllocateNodePickerIfNeeded();

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
	
	/**
	 * Move the viewport to the provided node to make it be centered.
	 */
	void JumpToNode(UEdGraphNode* Node, bool bRequestRename = false);
	
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

	//Debugger related

	TSharedRef<class SWidget> OnGetDebuggerActorsMenu();
	FText GetDebuggerActorDesc() const;
	void OnDebuggerActorSelected(TWeakObjectPtr<AJointActor> InstanceToDebug);

public:
	/**
	 * Find or add Joint Editor Toolkit for the provided asset or object.
	 * @param ObjectRelatedTo The joint manager related object.
	 * @param bOpenIfNotPresent If not present, open up.
	 * @return Found toolkit for the provided asset.
	 */
	static FJointEditorToolkit* FindOrOpenEditorInstanceFor(UObject* ObjectRelatedTo, const bool& bOpenIfNotPresent = true);

};
