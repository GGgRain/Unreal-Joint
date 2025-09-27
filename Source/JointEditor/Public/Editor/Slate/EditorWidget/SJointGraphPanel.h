// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Misc/Attribute.h"
#include "EdGraph/EdGraphPin.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Layout/Geometry.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "Animation/CurveSequence.h"
#include "UObject/GCObject.h"
#include "GraphEditor.h"
#include "SNodePanel.h"
#include "SGraphNode.h"
#include "GraphEditAction.h"
#include "SGraphPin.h"
#include "GraphSplineOverlapResult.h"
#include "SGraphPanel.h"

#include "Misc/EngineVersionComparison.h"


class FActiveTimerHandle;
class FSlateWindowElementList;
class IToolTip;
class IMenu;
class UEdGraph;

class SJointGraphPanel : public SGraphPanel
{
public:

	SLATE_BEGIN_ARGS( SJointGraphPanel )
		: _GraphObj( nullptr )
#if UE_VERSION_OLDER_THAN(5,1,0)
		, _GraphObjToDiff( nullptr )
#else
		, _DiffResults( nullptr )
#endif
		, _InitialZoomToFit( false )
		, _IsEditable( true )
		, _DisplayAsReadOnly( false )
		, _ShowGraphStateOverlay(true)
		, _OnUpdateGraphPanel()
		{
			_Clipping = EWidgetClipping::ClipToBounds;
		}

		SLATE_EVENT( FOnGetContextMenuFor, OnGetContextMenuFor )
		SLATE_EVENT( SGraphEditor::FOnSelectionChanged, OnSelectionChanged )
		SLATE_EVENT( FSingleNodeEvent, OnNodeDoubleClicked )
		SLATE_EVENT( SGraphEditor::FOnDropActor, OnDropActor )
		SLATE_EVENT( SGraphEditor::FOnDropStreamingLevel, OnDropStreamingLevel )
		SLATE_ARGUMENT( class UEdGraph*, GraphObj )
#if UE_VERSION_OLDER_THAN(5,1,0)
		SLATE_ARGUMENT( class UEdGraph*, GraphObjToDiff )
#else
		SLATE_ARGUMENT( TSharedPtr<TArray<FDiffSingleResult>>, DiffResults )
#endif
		SLATE_ARGUMENT( bool, InitialZoomToFit )
		SLATE_ATTRIBUTE( bool, IsEditable )
		SLATE_ATTRIBUTE( bool, DisplayAsReadOnly )
		/** Show overlay elements for the graph state such as the PIE and read-only borders and text */
		SLATE_ATTRIBUTE(bool, ShowGraphStateOverlay)
		SLATE_EVENT( FOnNodeVerifyTextCommit, OnVerifyTextCommit )
		SLATE_EVENT( FOnNodeTextCommitted, OnTextCommitted )
		SLATE_EVENT( SGraphEditor::FOnSpawnNodeByShortcut, OnSpawnNodeByShortcut )
		SLATE_EVENT( FOnUpdateGraphPanel, OnUpdateGraphPanel )
		SLATE_EVENT( SGraphEditor::FOnDisallowedPinConnection, OnDisallowedPinConnection )
		SLATE_EVENT( SGraphEditor::FOnDoubleClicked, OnDoubleClicked )
		//SLATE_ATTRIBUTE( FGraphAppearanceInfo, Appearance )
	SLATE_END_ARGS()

	/**
	 * Construct a widget
	 *
	 * @param InArgs    The declaration describing how the widgets should be constructed.
	 */
	void Construct( const SJointGraphPanel::FArguments& InArgs );

	void Construct( const SJointGraphPanel::FArguments& InArgs, const SGraphPanel::FArguments& InNestedArgs);


public:
	
	void PaintBackground(const FSlateBrush* BackgroundImage, const FGeometry& AllottedGeometry,
	                     const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                     int32& DrawLayerId) const;

public:

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;


public:

	void SetAllowContinousZoomInterpolation(bool bAllow);

private:
	
	static inline float Joint_FancyMod(float Value, float Size);

private:

	TAttribute<bool> Joint_DisplayAsReadOnly;

	/** The current node factory to create nodes, pins and connections. Uses the static FNodeFactory if not set. */
	TSharedPtr<class FGraphNodeFactory> Joint_NodeFactory;

	/** Sets the current widget factory. */
	void SetNodeFactory(const TSharedRef<class FGraphNodeFactory>& NewNodeFactory);
	
	void Joint_OnSplineHoverStateChanged(const FGraphSplineOverlapResult& NewSplineHoverState);
};

