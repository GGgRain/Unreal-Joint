// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "GameFramework/Actor.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "BlueprintUtilities.h"
#include "Framework/Commands/InputChord.h"
#include "Framework/Commands/UICommandList.h"
#include "Engine/LevelStreaming.h"
#include "GraphEditor.h"

#include "Misc/EngineVersionComparison.h"


class UEdGraph;
class SGraphPanel;
struct FNotificationInfo;
struct Rect;
class FMenuBuilder;
class FAssetEditorToolkit;

/**
 * Interface and wrapper for GraphEditor widgets.
 * Gracefully handles the GraphEditorModule being unloaded.
 */
class SJointGraphEditor : public SGraphEditor
{
	
public:

	SLATE_BEGIN_ARGS(SJointGraphEditor)
		: _AdditionalCommands( nullptr )
		, _IsEditable(true)
		, _DisplayAsReadOnly(false)
		, _IsEmpty(false)
		, _GraphToEdit(nullptr)
#if UE_VERSION_OLDER_THAN(5,1,0)
		, _GraphToDiff(nullptr)
#else
		, _DiffResults(nullptr)
#endif
		, _AutoExpandActionMenu(false)
		, _ShowGraphStateOverlay(true)
		{}

		SLATE_ARGUMENT( TSharedPtr<FUICommandList>, AdditionalCommands )
		SLATE_ATTRIBUTE( bool, IsEditable )		
		SLATE_ATTRIBUTE( bool, DisplayAsReadOnly )		
		SLATE_ATTRIBUTE( bool, IsEmpty )	
		SLATE_ARGUMENT( TSharedPtr<SWidget>, TitleBar )
		SLATE_ATTRIBUTE( FGraphAppearanceInfo, Appearance )
		SLATE_EVENT( FEdGraphEvent, OnGraphModuleReloaded )
		SLATE_ARGUMENT( UEdGraph*, GraphToEdit )
#if UE_VERSION_OLDER_THAN(5,1,0)
		SLATE_ARGUMENT( UEdGraph*, GraphToDiff )
#else
		SLATE_ARGUMENT( TSharedPtr<TArray<FDiffSingleResult>>, DiffResults )
#endif
		SLATE_ARGUMENT( SGraphEditor::FGraphEditorEvents, GraphEvents)
		SLATE_ARGUMENT( bool, AutoExpandActionMenu )
		SLATE_ARGUMENT( TWeakPtr<FAssetEditorToolkit>, AssetEditorToolkit)
		SLATE_EVENT(FSimpleDelegate, OnNavigateHistoryBack)
		SLATE_EVENT(FSimpleDelegate, OnNavigateHistoryForward)

		/** Show overlay elements for the graph state such as the PIE and read-only borders and text */
		SLATE_ATTRIBUTE(bool, ShowGraphStateOverlay)				
	SLATE_END_ARGS()

	/**
	 * Loads the GraphEditorModule and constructs a GraphEditor as a child of this widget.
	 *
	 * @param InArgs   Declaration params from which to construct the widget.
	 */
	void Construct( const FArguments& InArgs );

	virtual FVector2D GetPasteLocation() const override
	{
		if (Implementation.IsValid())
		{
			return Implementation->GetPasteLocation();
		}
		else
		{
			return FVector2D::ZeroVector;
		}
	}

	/* Set new viewer location and optionally set the current bookmark */
	virtual void SetViewLocation(const FVector2D& Location, float ZoomAmount, const FGuid& BookmarkId = FGuid()) override
	{
		if (Implementation.IsValid())
		{
			Implementation->SetViewLocation(Location, ZoomAmount, BookmarkId);
		}
	}

	/**
	 * Gets the view location of the graph
	 *
	 * @param OutLocation		Will have the current view location
	 * @param OutZoomAmount		Will have the current zoom amount
	 */
	virtual void GetViewLocation(FVector2D& OutLocation, float& OutZoomAmount) override
	{
		if (Implementation.IsValid())
		{
			Implementation->GetViewLocation(OutLocation, OutZoomAmount);
		}
	}

	/**
	 * Gets the current graph view bookmark
	 *
	 * @param OutBookmarkId		Will have the current bookmark ID
	 */
	virtual void GetViewBookmark(FGuid& OutBookmarkId) override
	{
		if (Implementation.IsValid())
		{
			Implementation->GetViewBookmark(OutBookmarkId);
		}
	}

	/** Check if node title is visible with optional flag to ensure it is */
	virtual bool IsNodeTitleVisible(const class UEdGraphNode* Node, bool bRequestRename) override
	{
		bool bResult = false;
		if (Implementation.IsValid())
		{
			bResult = Implementation->IsNodeTitleVisible(Node, bRequestRename);
		}
		return bResult;
	}

	/* Lock two graph editors together */
	virtual void LockToGraphEditor(TWeakPtr<SGraphEditor> Other) override
	{
		if (Implementation.IsValid())
		{
			Implementation->LockToGraphEditor(Other);
		}
	}

	/* Unlock two graph editors from each other */
	virtual void UnlockFromGraphEditor(TWeakPtr<SGraphEditor> Other) override
	{
		if (Implementation.IsValid())
		{
			Implementation->UnlockFromGraphEditor(Other);
		}
	}

	/** Bring the specified node into view */
	virtual void JumpToNode( const class UEdGraphNode* JumpToMe, bool bRequestRename = false, bool bSelectNode = true ) override
	{
		if (Implementation.IsValid())
		{
			Implementation->JumpToNode(JumpToMe, bRequestRename, bSelectNode);
		}
	}

	/** Bring the specified pin into view */
	virtual void JumpToPin( const class UEdGraphPin* JumpToMe ) override
	{
		if (Implementation.IsValid())
		{
			Implementation->JumpToPin(JumpToMe);
		}
	}

	/*Set the pin visibility mode*/
	virtual void SetPinVisibility(SGraphEditor::EPinVisibility InVisibility) override
	{
		if (Implementation.IsValid())
		{
			Implementation->SetPinVisibility(InVisibility);
		}
	}

	/** Register an active timer on the graph editor. */
	virtual TSharedRef<FActiveTimerHandle> RegisterActiveTimer(float TickPeriod, FWidgetActiveTimerDelegate TickFunction) override
	{
		if (Implementation.IsValid())
		{
			return Implementation->RegisterActiveTimer(TickPeriod, TickFunction);
		}
		return TSharedPtr<FActiveTimerHandle>().ToSharedRef();
	}

	/** @return a reference to the list of selected graph nodes */
	virtual const FGraphPanelSelectionSet& GetSelectedNodes() const override
	{
		static FGraphPanelSelectionSet NoSelection;

		if (Implementation.IsValid())
		{
			return Implementation->GetSelectedNodes();
		}
		else
		{
			return NoSelection;
		}
	}

	/** Clear the selection */
	virtual void ClearSelectionSet() override
	{
		if (Implementation.IsValid())
		{
			Implementation->ClearSelectionSet();
		}
	}

	/** Set the selection status of a node */
	virtual void SetNodeSelection(UEdGraphNode* Node, bool bSelect) override
	{
		if (Implementation.IsValid())
		{
			Implementation->SetNodeSelection(Node, bSelect);
		}
	}
	
	/** Select all nodes */
	virtual void SelectAllNodes() override
	{
		if (Implementation.IsValid())
		{
			Implementation->SelectAllNodes();
		}		
	}

	virtual class UEdGraphPin* GetGraphPinForMenu() override
	{
		if ( Implementation.IsValid() )
		{
			return Implementation->GetGraphPinForMenu();
		}
		else
		{
			return NULL;
		}
	}

	virtual class UEdGraphNode* GetGraphNodeForMenu() override
	{
		if ( Implementation.IsValid() )
		{
			return Implementation->GetGraphNodeForMenu();
		}
		else
		{
			return NULL;
		}
	}

	// Zooms out to fit either all nodes or only the selected ones
	virtual void ZoomToFit(bool bOnlySelection) override
	{
		if (Implementation.IsValid())
		{
			return Implementation->ZoomToFit(bOnlySelection);
		}
	}

	/** Get Bounds for selected nodes, false if nothing selected*/
	virtual bool GetBoundsForSelectedNodes( class FSlateRect& Rect, float Padding  ) override
	{
		if (Implementation.IsValid())
		{
			return Implementation->GetBoundsForSelectedNodes(Rect, Padding);
		}
		return false;
	}

	/** Get Bounds for the specified node, returns false on failure */
	virtual bool GetBoundsForNode( const UEdGraphNode* InNode, class FSlateRect& Rect, float Padding ) const override
	{
		if (Implementation.IsValid())
		{
			return Implementation->GetBoundsForNode(InNode, Rect, Padding);
		}
		return false;
	}

	virtual void StraightenConnections() override
	{
		if (Implementation.IsValid())
		{
			return Implementation->StraightenConnections();
		}
	}

	virtual void StraightenConnections(UEdGraphPin* SourcePin, UEdGraphPin* PinToAlign = nullptr) const override
	{
		if (Implementation.IsValid())
		{
			return Implementation->StraightenConnections(SourcePin, PinToAlign);
		}
	}

	virtual void RefreshNode(UEdGraphNode& Node) override
	{
		if (Implementation.IsValid())
		{
			return Implementation->RefreshNode(Node);
		}
	}
	
	// Invoked to let this widget know that the GraphEditor module is being unloaded.
	void OnModuleUnloading();

	/** Invoked when the Graph being edited changes in some way. */
	virtual void NotifyGraphChanged() override
	{
		if (Implementation.IsValid())
		{
			Implementation->NotifyGraphChanged();
		}
	}

	/* Get the title bar if there is one */
	virtual TSharedPtr<SWidget> GetTitleBar() const override
	{
		if (Implementation.IsValid())
		{
			return Implementation->GetTitleBar();
		}
		return TSharedPtr<SWidget>();
	}

	/** Show notification on graph */
	virtual void AddNotification(FNotificationInfo& Info, bool bSuccess) override
	{
		if (Implementation.IsValid())
		{
			Implementation->AddNotification(Info, bSuccess);
		}
	}

	/** Capture keyboard */
	virtual void CaptureKeyboard() override
	{
		if (Implementation.IsValid())
		{
			Implementation->CaptureKeyboard();
		}
	}

	/** Sets the current node, pin and connection factory. */
	virtual void SetNodeFactory(const TSharedRef<class FGraphNodeFactory>& NewNodeFactory) override
	{
		if (Implementation.IsValid())
		{
			Implementation->SetNodeFactory(NewNodeFactory);
		}
	}
	
	virtual void OnCollapseNodes() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnCollapseNodes();
		}
	}

	virtual bool CanCollapseNodes() const override
	{
		return Implementation.IsValid() ? Implementation->CanCollapseNodes() : false;
	}

	virtual void OnExpandNodes() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnExpandNodes();
		}
	}

	virtual bool CanExpandNodes() const override
	{
		return Implementation.IsValid() ? Implementation->CanExpandNodes() : false;
	}

	virtual void OnAlignTop() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnAlignTop();
		}
	}

	virtual void OnAlignMiddle() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnAlignMiddle();
		}
	}

	virtual void OnAlignBottom() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnAlignBottom();
		}
	}

	virtual void OnAlignLeft() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnAlignLeft();
		}
	}

	virtual void OnAlignCenter() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnAlignCenter();
		}
	}

	virtual void OnAlignRight() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnAlignRight();
		}
	}


	virtual void OnStraightenConnections() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnStraightenConnections();
		}
	}


	virtual void OnDistributeNodesH() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnDistributeNodesH();
		}
	}

	virtual void OnDistributeNodesV() override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnDistributeNodesV();
		}
	}


	virtual int32 GetNumberOfSelectedNodes() const override
	{
		if (Implementation.IsValid())
		{
			return Implementation->GetNumberOfSelectedNodes();
		}
		return 0;
	}


	/** Returns the currently selected node if there is a single node selected (if there are multiple nodes selected or none selected, it will return nullptr) */
	virtual UEdGraphNode* GetSingleSelectedNode() const override
	{
		if (Implementation.IsValid())
		{
			return Implementation->GetSingleSelectedNode();
		}
		return nullptr;
	}

	// Returns the first graph editor that is viewing the specified graph
	static TSharedPtr<SJointGraphEditor> FindGraphEditorForGraph(const UEdGraph* Graph);


	/** Returns the graph panel used for this graph editor */
	virtual SGraphPanel* GetGraphPanel() const override
	{
		if (Implementation.IsValid())
		{
			return Implementation->GetGraphPanel();
		}
		return nullptr;
	}

protected:
	/** Invoked when the underlying Graph is being changed. */
	virtual void OnGraphChanged(const struct FEdGraphEditAction& InAction) override
	{
		if (Implementation.IsValid())
		{
			Implementation->OnGraphChanged(InAction);
		}
	}

private:
	
	static void RegisterGraphEditor(const TSharedRef<SJointGraphEditor>& InGraphEditor);

	void ConstructImplementation( const FArguments& InArgs );

private:
	
	/** The actual implementation of the GraphEditor */
	TSharedPtr<SJointGraphEditor> Implementation;

	/** Active GraphEditor wrappers; we will notify these about the module being unloaded so they can handle it gracefully. */
	static TArray< TWeakPtr<SJointGraphEditor> > AllInstances;

	// This callback is triggered whenever the graph module is reloaded
	FEdGraphEvent OnGraphModuleReloadedCallback;

	// The graph editor module needs to access AllInstances, but no-one else should be able to
	friend class FGraphEditorModule;
	
};
