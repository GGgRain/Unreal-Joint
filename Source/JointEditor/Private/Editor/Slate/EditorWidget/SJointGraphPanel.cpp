// Copyright Epic Games, Inc. All Rights Reserved.


#include "EditorWidget/SJointGraphPanel.h"
#include "Rendering/DrawElements.h"
#include "EdGraph/EdGraph.h"
#include "Layout/WidgetPath.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Types/SlateAttributeMetaData.h"
#include "EdGraphNode_Comment.h"
#include "Editor.h"
#include "GraphEditorSettings.h"
#include "NodeFactory.h"

#include "ConnectionDrawingPolicy.h"

#include "GraphDiffControl.h"
#include "JointEdGraphSchema.h"
#include "JointEditorSettings.h"
#include "JointEditorStyle.h"

#include "ScopedTransaction.h"


#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 1, 0)

#else

#include "SNodePanel.h"
#include "DiffResults.h"

#endif

DEFINE_LOG_CATEGORY_STATIC(LogGraphPanel, Log, All);

//////////////////////////////////////////////////////////////////////////
// SJointGraphPanel

//////////////////////////////////////////////////////////////////////////

void SJointGraphPanel::Construct(const SJointGraphPanel::FArguments& InArgs)
{
	Joint_DisplayAsReadOnly = InArgs._DisplayAsReadOnly;

	const SGraphPanel::FArguments& NestedArgs = SGraphPanel::FArguments()
		.OnGetContextMenuFor(InArgs._OnGetContextMenuFor)
		.OnSelectionChanged(InArgs._OnSelectionChanged)
		.OnNodeDoubleClicked(InArgs._OnNodeDoubleClicked)
		.OnDropActor(InArgs._OnDropActor)
		.OnDropStreamingLevel(InArgs._OnDropStreamingLevel)
		.GraphObj(InArgs._GraphObj)
#if UE_VERSION_OLDER_THAN(5, 1, 0)
		.GraphObjToDiff(InArgs._GraphObjToDiff)
#else
		.DiffResults(InArgs._DiffResults)
#endif
		.InitialZoomToFit(InArgs._InitialZoomToFit)
		.IsEditable(InArgs._IsEditable)
		.DisplayAsReadOnly(InArgs._DisplayAsReadOnly)
		.ShowGraphStateOverlay(InArgs._ShowGraphStateOverlay)
		.OnVerifyTextCommit(InArgs._OnVerifyTextCommit)
		.OnTextCommitted(InArgs._OnTextCommitted)
		.OnSpawnNodeByShortcut(InArgs._OnSpawnNodeByShortcut)
		.OnUpdateGraphPanel(InArgs._OnUpdateGraphPanel)
		.OnDisallowedPinConnection(InArgs._OnDisallowedPinConnection)
		.OnDoubleClicked(InArgs._OnDoubleClicked);


	SGraphPanel::Construct(NestedArgs);
}

void SJointGraphPanel::Construct(const SJointGraphPanel::FArguments& InArgs,
                                 const SGraphPanel::FArguments& InNestedArgs)
{
	Joint_DisplayAsReadOnly = InNestedArgs._DisplayAsReadOnly;

	SGraphPanel::Construct(InNestedArgs);
}

namespace JointNodePanelDefs
{
	// Default Zoom Padding Value
	static const float DefaultZoomPadding = 10.f;
	// Node Culling Guardband Area
	static const float GuardBandArea = 0.10f;
	// Scaling factor to reduce speed of mouse zooming
	static const float MouseZoomScaling = 0.01f;
};


FReply SJointGraphPanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	LastPointerEvent = MouseEvent;
	LastPointerGeometry = MyGeometry;

	// Save the mouse position to use in OnPaint for spline hit detection
	SavedMousePosForOnPaintEventLocalSpace = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	// Invalidate the spline results if we moved very far
	const FVector2D MouseDelta = SavedMousePosForOnPaintEventLocalSpace - PreviousFrameSavedMousePosForSplineOverlap;
	const float MouseDeltaLengthSquared = MouseDelta.SizeSquared();
	const bool b_FromParent_CursorInDeadZone = MouseDeltaLengthSquared <= FMath::Square(
		FSlateApplication::Get().GetDragTriggerDistance());

	if (!b_FromParent_CursorInDeadZone)
	{
		//@TODO: Should we do this or just rely on the next OnPaint?
		// Our frame-latent approximation is going to be totally junk if the mouse is moving quickly
		Joint_OnSplineHoverStateChanged(FGraphSplineOverlapResult());
	}

	const bool bIsRightMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);
	const bool bIsLeftMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
	const bool bIsMiddleMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);
	const FModifierKeysState ModifierKeysState = FSlateApplication::Get().GetModifierKeys();

	PastePosition = PanelCoordToGraphCoord(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));

	if (this->HasMouseCapture())
	{
		const FVector2D CursorDelta = MouseEvent.GetCursorDelta();
		// Track how much the mouse moved since the mouse down.
		TotalMouseDelta += CursorDelta.Size();

		const bool bShouldZoom = bIsRightMouseButtonDown && (bIsLeftMouseButtonDown || bIsMiddleMouseButtonDown ||
			ModifierKeysState.IsAltDown() || FSlateApplication::Get().IsUsingTrackpad());
		if (bShouldZoom)
		{
			FReply ReplyState = FReply::Handled();

			TotalMouseDeltaXY += CursorDelta.X + CursorDelta.Y;

			const int32 ZoomLevelDelta = FMath::RoundToInt(TotalMouseDeltaXY * JointNodePanelDefs::MouseZoomScaling);

			// Get rid of mouse movement that's been 'used up' by zooming
			if (ZoomLevelDelta != 0)
			{
				TotalMouseDeltaXY -= (ZoomLevelDelta / JointNodePanelDefs::MouseZoomScaling);
			}

			// Perform zoom centered on the cached start offset
			ChangeZoomLevel(ZoomLevelDelta, ZoomStartOffset, MouseEvent.IsControlDown());

			this->bIsPanning = false;

			if (FSlateApplication::Get().IsUsingTrackpad() && ZoomLevelDelta != 0)
			{
				this->bIsZoomingWithTrackpad = true;
				bShowSoftwareCursor = true;
			}

			// Stop the zoom-to-fit in favor of user control
			CancelZoomToFit();

			return ReplyState;
		}
		else if (bIsRightMouseButtonDown)
		{
			FReply ReplyState = FReply::Handled();

			if (!CursorDelta.IsZero())
			{
				bShowSoftwareCursor = true;
			}

			// Panning and mouse is outside of panel? Pasting should just go to the screen center.
			PastePosition = PanelCoordToGraphCoord(0.5f * MyGeometry.GetLocalSize());

			this->bIsPanning = true;
			ViewOffset -= CursorDelta / GetZoomAmount();

			// Stop the zoom-to-fit in favor of user control
			CancelZoomToFit();

			return ReplyState;
		}
		else if (bIsMiddleMouseButtonDown)
		{
			FReply ReplyState = FReply::Handled();

			if (!CursorDelta.IsZero())
			{
				bShowSoftwareCursor = true;
			}

			// Panning and mouse is outside of panel? Pasting should just go to the screen center.
			PastePosition = PanelCoordToGraphCoord(0.5f * MyGeometry.Size);

			this->bIsPanning = true;
			ViewOffset -= CursorDelta / GetZoomAmount();

			return ReplyState;
		}
		else if (bIsLeftMouseButtonDown)
		{
			TSharedPtr<SNode> NodeBeingDragged = NodeUnderMousePtr.Pin();

			if (IsEditable.Get())
			{
				// Update the amount to pan panel
				UpdateViewOffset(MyGeometry, MouseEvent.GetScreenSpacePosition());

				const bool bCursorInDeadZone = TotalMouseDelta <= FSlateApplication::Get().GetDragTriggerDistance();

				if (NodeBeingDragged.IsValid())
				{
					if (!bCursorInDeadZone)
					{
						// Note, NodeGrabOffset() comes from the node itself, so it's already scaled correctly.
						FVector2D AnchorNodeNewPos = PanelCoordToGraphCoord(
							MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())) - NodeGrabOffset;

						// Snap to grid
						const float SnapSize = UJointEditorSettings::GetJointGridSnapSize();
						AnchorNodeNewPos.X = SnapSize * FMath::RoundToFloat(AnchorNodeNewPos.X / SnapSize);
						AnchorNodeNewPos.Y = SnapSize * FMath::RoundToFloat(AnchorNodeNewPos.Y / SnapSize);

						// Dragging an unselected node automatically selects it.
						SelectionManager.StartDraggingNode(NodeBeingDragged->GetObjectBeingDisplayed(), MouseEvent);

						// Move all the selected nodes.
						{
							const FVector2D AnchorNodeOldPos = NodeBeingDragged->GetPosition();
							const FVector2D DeltaPos = AnchorNodeNewPos - AnchorNodeOldPos;

							// Perform movement in 3 passes:

							// 1. Gather all selected nodes positions and calculate new positions
							struct FDefferedNodePosition
							{
								SNode* Node;
								FVector2D NewPosition;
							};
							TArray<FDefferedNodePosition> DefferedNodesToMove;


							// 2. Deffer actual move transactions to mouse release or focus lost
							bool bStoreOriginalNodePositions = OriginalNodePositions.Num() == 0;

#if UE_VERSION_OLDER_THAN(5, 3, 0)
							
							for (FGraphPanelSelectionSet::TIterator NodeIt(SelectionManager.SelectedNodes); NodeIt; ++
								 NodeIt)
							{
								if (TSharedRef<SNode>* pWidget = NodeToWidgetLookup.Find(*NodeIt))
								{
									SNode& Widget = pWidget->Get();
									FDefferedNodePosition NodePosition = {&Widget, Widget.GetPosition() + DeltaPos};
									DefferedNodesToMove.Add(NodePosition);

									if (bStoreOriginalNodePositions)
									{
										OriginalNodePositions.FindOrAdd(*pWidget) = Widget.GetPosition();
									}
								}
							}
#else
							
							for (TSet<TObjectPtr<class UObject>>::TIterator NodeIt(SelectionManager.SelectedNodes); NodeIt; ++NodeIt)
							{
								if (TSharedRef<SNode>* pWidget = NodeToWidgetLookup.Find(*NodeIt))
								{
									SNode& Widget = pWidget->Get();
									FDefferedNodePosition NodePosition = { &Widget, Widget.GetPosition() + DeltaPos };
									DefferedNodesToMove.Add(NodePosition);

									if (bStoreOriginalNodePositions)
									{
#if UE_VERSION_OLDER_THAN(5, 6, 0)
										OriginalNodePositions.FindOrAdd(*pWidget) = Widget.GetPosition();

#else
										OriginalNodePositions.FindOrAdd(*pWidget) = Widget.GetPosition2f();

#endif
									}
								}
							}
#endif

							// 3. Move selected nodes to new positions
							SNode::FNodeSet NodeFilter;

							for (int32 NodeIdx = 0; NodeIdx < DefferedNodesToMove.Num(); ++NodeIdx)
							{
								DefferedNodesToMove[NodeIdx].Node->MoveTo(
									DefferedNodesToMove[NodeIdx].NewPosition, NodeFilter, false);
							}
						}
					}

					return FReply::Handled();
				}
			}

			if (!NodeBeingDragged.IsValid())
			{
				// We are marquee selecting
				const FVector2D GraphMousePos = PanelCoordToGraphCoord(
					MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
				Marquee.Rect.UpdateEndPoint(GraphMousePos);

				FindNodesAffectedByMarquee(/*out*/ Marquee.AffectedNodes);
				return FReply::Handled();
			}

			// Stop the zoom-to-fit in favor of user control
			CancelZoomToFit();
		}
	}

	return FReply::Unhandled();
}

void SJointGraphPanel::PaintBackground(const FSlateBrush* BackgroundImage, const FGeometry& AllottedGeometry,
                                       const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                       int32& DrawLayerId) const
{
	const bool bAntialias = true;

	const int32 RulePeriod = static_cast<int32>(FJointEditorStyle::GetUEEditorSlateStyleSet().GetFloat(
		"Graph.Panel.GridRulePeriod"));
	check(RulePeriod > 0);

	const FLinearColor GraphBackGroundImageColor(UJointEditorSettings::Get()->BackgroundColor);
	const FLinearColor RegularColor(UJointEditorSettings::Get()->RegularGridColor);
	const FLinearColor RuleColor(UJointEditorSettings::Get()->RuleGridColor);
	const FLinearColor CenterColor(UJointEditorSettings::Get()->CenterGridColor);
	const float GraphSmallestGridSize = UJointEditorSettings::Get()->SmallestGridSize;
	const float RawZoomFactor = GetZoomAmount();
	const float NominalGridSize = UJointEditorSettings::GetJointGridSnapSize();

	float ZoomFactor = RawZoomFactor;
	float Inflation = 1.0f;
	while (ZoomFactor * Inflation * NominalGridSize <= GraphSmallestGridSize)
	{
		Inflation *= 2.0f;
	}

	const float GridCellSize = NominalGridSize * ZoomFactor * Inflation;

	const float GraphSpaceGridX0 = Joint_FancyMod(ViewOffset.X, Inflation * NominalGridSize * RulePeriod);
	const float GraphSpaceGridY0 = Joint_FancyMod(ViewOffset.Y, Inflation * NominalGridSize * RulePeriod);

	float ImageOffsetX = GraphSpaceGridX0 * -ZoomFactor;
	float ImageOffsetY = GraphSpaceGridY0 * -ZoomFactor;

#if UE_VERSION_OLDER_THAN(5, 6, 0)
	const FVector2D ZeroSpace = GraphCoordToPanelCoord(FVector2D::ZeroVector);
#else
	const FVector2f ZeroSpace = GraphCoordToPanelCoord(FVector2f::ZeroVector);
#endif
	
	// Fill the background
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		DrawLayerId,
		AllottedGeometry.ToPaintGeometry(),
		BackgroundImage,
		ESlateDrawEffect::NoGamma | ESlateDrawEffect::NoPixelSnapping,
		GraphBackGroundImageColor
	);

	TArray<FVector2D> LinePoints;
	new(LinePoints) FVector2D(0.0f, 0.0f);
	new(LinePoints) FVector2D(0.0f, 0.0f);

	//If we want to use grid then show grid, otherwise don't render the grid
	if (UJointEditorSettings::Get()->bUseGrid == true)
	{
		// Horizontal bars
		for (int32 GridIndex = 0; ImageOffsetY < AllottedGeometry.GetLocalSize().Y; ImageOffsetY += GridCellSize, ++
		     GridIndex)
		{
			if (ImageOffsetY >= 0.0f)
			{
				const bool bIsRuleLine = (GridIndex % RulePeriod) == 0;
				const int32 Layer = bIsRuleLine ? (DrawLayerId + 1) : DrawLayerId;

				const FLinearColor* Color = bIsRuleLine ? &RuleColor : &RegularColor;
				if (FMath::IsNearlyEqual(ZeroSpace.Y, ImageOffsetY, 1.0f))
				{
					Color = &CenterColor;
				}

				LinePoints[0] = FVector2D(0.0f, ImageOffsetY);
				LinePoints[1] = FVector2D(AllottedGeometry.GetLocalSize().X, ImageOffsetY);

				FSlateDrawElement::MakeLines(
					OutDrawElements,
					Layer,
					AllottedGeometry.ToPaintGeometry(),
					LinePoints,
					ESlateDrawEffect::NoGamma | ESlateDrawEffect::NoPixelSnapping,
					*Color,
					bAntialias);
			}
		}

		// Vertical bars
		for (int32 GridIndex = 0; ImageOffsetX < AllottedGeometry.GetLocalSize().X; ImageOffsetX += GridCellSize, ++
		     GridIndex)
		{
			if (ImageOffsetX >= 0.0f)
			{
				const bool bIsRuleLine = (GridIndex % RulePeriod) == 0;
				const int32 Layer = bIsRuleLine ? (DrawLayerId + 1) : DrawLayerId;

				const FLinearColor* Color = bIsRuleLine ? &RuleColor : &RegularColor;
				if (FMath::IsNearlyEqual(ZeroSpace.X, ImageOffsetX, 1.0f))
				{
					Color = &CenterColor;
				}

				LinePoints[0] = FVector2D(ImageOffsetX, 0.0f);
				LinePoints[1] = FVector2D(ImageOffsetX, AllottedGeometry.GetLocalSize().Y);
				FSlateDrawElement::MakeLines(
					OutDrawElements,
					Layer,
					AllottedGeometry.ToPaintGeometry(),
					LinePoints,
					ESlateDrawEffect::NoGamma | ESlateDrawEffect::NoPixelSnapping,
					*Color,
					bAntialias);
			}
		}
	}
	DrawLayerId += 2;
}



int32 SJointGraphPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	CachedAllottedGeometryScaledSize = AllottedGeometry.GetLocalSize() * AllottedGeometry.Scale;

	//Style used for objects that are the same between revisions
	FWidgetStyle FadedStyle = InWidgetStyle;
	//FadedStyle.BlendColorAndOpacityTint(FLinearColor(0.45f,0.45f,0.45f,0.45f));

	//const FSlateBrush* DefaultBackground = FEditorStyle::GetBrush(TEXT("Graph.Panel.SolidBackground"));
	const FSlateBrush* BackgroundImage = FJointEditorStyle::Get().GetBrush("JointUI.Image.GraphBackground");

	PaintBackground(BackgroundImage, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId);

	const float ZoomFactor = AllottedGeometry.Scale * GetZoomAmount();

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	ArrangeChildNodes(AllottedGeometry, ArrangedChildren);

	// Determine some 'global' settings based on current LOD
	const bool bDrawShadowsThisFrame = false; // GetCurrentLOD() > EGraphRenderingLOD::LowestDetail;

	// Because we paint multiple children, we must track the maximum layer id that they produced in case one of our parents
	// wants to an overlay for all of its contents.

	// Save LayerId for comment boxes to ensure they always appear below nodes & wires
	const int32 CommentNodeShadowLayerId = LayerId++;
	const int32 CommentNodeLayerId = LayerId++;

#if UE_VERSION_OLDER_THAN(5, 1, 0)

	
#else

	const int32 NodeDiffHighlightLayerID = LayerId++;

#endif
	

	// Save a LayerId for wires, which appear below nodes but above comments
	// We will draw them later, along with the arrows which appear above nodes.
	const int32 WireLayerId = LayerId++;

	const int32 NodeShadowsLayerId = LayerId;
	const int32 NodeLayerId = NodeShadowsLayerId + 1;
	int32 MaxLayerId = NodeLayerId;

	const FPaintArgs NewArgs = Args.WithNewParent(this);

	const FVector2D NodeShadowSize = GetDefault<UGraphEditorSettings>()->GetShadowDeltaSize();
	const UJointEdGraphSchema* Schema = Cast<UJointEdGraphSchema>(GraphObj->GetSchema());
	
	// Draw the child nodes


#if UE_VERSION_OLDER_THAN(5, 1, 0)


	// When drawing a marquee, need a preview of what the selection will be.
	const FGraphPanelSelectionSet* SelectionToVisualize = &(SelectionManager.SelectedNodes);
	FGraphPanelSelectionSet SelectionPreview;
	if (Marquee.IsValid())
	{
		ApplyMarqueeSelection(Marquee, SelectionManager.SelectedNodes, SelectionPreview);
		SelectionToVisualize = &SelectionPreview;
	}

	// Context for rendering node infos
	//FKismetNodeInfoContext Context(Joint_GraphObj);

	TArray<FGraphDiffControl::FNodeMatch> NodeMatches;
	for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
	{
		FArrangedWidget& CurWidget = ArrangedChildren[ChildIndex];
		TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);

		// Examine node to see what layers we should be drawing in
		int32 ShadowLayerId = NodeShadowsLayerId;
		int32 ChildLayerId = NodeLayerId;

		// If a comment node, draw in the dedicated comment slots
		{
			UObject* NodeObj = ChildNode->GetObjectBeingDisplayed();
			if (NodeObj && NodeObj->IsA(UEdGraphNode_Comment::StaticClass()))
			{
				ShadowLayerId = CommentNodeShadowLayerId;
				ChildLayerId = CommentNodeLayerId;
			}
		}


		const bool bNodeIsVisible = FSlateRect::DoRectanglesIntersect(CurWidget.Geometry.GetLayoutBoundingRect(),
		                                                              MyCullingRect);

		if (bNodeIsVisible)
		{
			const bool bSelected = SelectionToVisualize->Contains(
				StaticCastSharedRef<SNodePanel::SNode>(CurWidget.Widget)->GetObjectBeingDisplayed());

			UEdGraphNode* NodeObj = Cast<UEdGraphNode>(ChildNode->GetObjectBeingDisplayed());
			float Alpha = 1.0f;

			// Handle Node renaming once the node is visible
			if (bSelected && ChildNode->IsRenamePending())
			{
				// Only open a rename when the window has focus
				TSharedPtr<SWindow> OwnerWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
				if (!OwnerWindow.IsValid() || FSlateApplication::Get().HasFocusedDescendants(OwnerWindow.ToSharedRef()))
				{
					ChildNode->ApplyRename();
				}
			}

			// Draw the node's shadow.
			if (bDrawShadowsThisFrame || bSelected)
			{
				const FSlateBrush* ShadowBrush = ChildNode->GetShadowBrush(bSelected);
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					ShadowLayerId,
					CurWidget.Geometry.ToInflatedPaintGeometry(NodeShadowSize),
					ShadowBrush,
					ESlateDrawEffect::None,
					FLinearColor(1.0f, 1.0f, 1.0f, Alpha)
				);
			}

			// Draw the comments and information popups for this node, if it has any.
			{
				const SNodePanel::SNode::FNodeSlot* CommentSlot = ChildNode->GetSlot(ENodeZone::TopCenter);
				float CommentBubbleY = CommentSlot ? -CommentSlot->GetSlotOffset().Y : 0.f;
				//Context.bSelected = bSelected;
				TArray<FGraphInformationPopupInfo> Popups;

				{
					//ChildNode->GetNodeInfoPopups(&Context, /*out*/ Popups);
				}

				for (int32 PopupIndex = 0; PopupIndex < Popups.Num(); ++PopupIndex)
				{
					FGraphInformationPopupInfo& Popup = Popups[PopupIndex];
					PaintComment(Popup.Message, CurWidget.Geometry, MyCullingRect, OutDrawElements, ChildLayerId,
					             Popup.BackgroundColor, /*inout*/ CommentBubbleY, InWidgetStyle);
				}
			}

			int32 CurWidgetsMaxLayerId;
			{
				/** When diffing nodes, nodes that are different between revisions are opaque, nodes that have not changed are faded */
				FGraphDiffControl::FNodeMatch NodeMatch = FGraphDiffControl::FindNodeMatch(
					GraphObjToDiff, NodeObj, NodeMatches);
				if (NodeMatch.IsValid())
				{
					NodeMatches.Add(NodeMatch);
				}
				const bool bNodeIsDifferent = (!GraphObjToDiff || NodeMatch.
					Diff(FGraphDiffControl::FNodeDiffContext()));

				/* When dragging off a pin, we want to duck the alpha of some nodes */
				TSharedPtr<SGraphPin> OnlyStartPin = (1 == PreviewConnectorFromPins.Num())
					                                     ? PreviewConnectorFromPins[0].FindInGraphPanel(*this)
					                                     : TSharedPtr<SGraphPin>();
				const bool bNodeIsNotUsableInCurrentContext = Schema->FadeNodeWhenDraggingOffPin(
					NodeObj, OnlyStartPin.IsValid() ? OnlyStartPin.Get()->GetPinObj() : nullptr);

				const FWidgetStyle& NodeStyle = (bNodeIsDifferent && !bNodeIsNotUsableInCurrentContext)
					                                ? InWidgetStyle
					                                : FadedStyle;
				FWidgetStyle NodeStyleToUse = NodeStyle;
				NodeStyleToUse.BlendColorAndOpacityTint(FLinearColor(1.0f, 1.0f, 1.0f, Alpha));

				// Draw the node.O
				CurWidgetsMaxLayerId = CurWidget.Widget->Paint(NewArgs, CurWidget.Geometry, MyCullingRect,
				                                               OutDrawElements, ChildLayerId, NodeStyleToUse,
				                                               !Joint_DisplayAsReadOnly.Get() && ShouldBeEnabled(
					                                               bParentEnabled));
			}

			// Draw the node's overlay, if it has one.
			{
				// Get its size
				const FVector2D WidgetSize = CurWidget.Geometry.Size;

				{
					TArray<FOverlayBrushInfo> OverlayBrushes;
					ChildNode->GetOverlayBrushes(bSelected, WidgetSize, /*out*/ OverlayBrushes);

					for (int32 BrushIndex = 0; BrushIndex < OverlayBrushes.Num(); ++BrushIndex)
					{
						FOverlayBrushInfo& OverlayInfo = OverlayBrushes[BrushIndex];
						const FSlateBrush* OverlayBrush = OverlayInfo.Brush;
						if (OverlayBrush != nullptr)
						{
							FPaintGeometry BouncedGeometry = CurWidget.Geometry.ToPaintGeometry(
								OverlayInfo.OverlayOffset, OverlayBrush->ImageSize, 1.f);

							// Handle bouncing during PIE
							const float BounceValue = FMath::Sin(2.0f * PI * BounceCurve.GetLerp());
							BouncedGeometry.DrawPosition += FVector2f(
								OverlayInfo.AnimationEnvelope * BounceValue * ZoomFactor);

							FLinearColor FinalColorAndOpacity(
								InWidgetStyle.GetColorAndOpacityTint() * OverlayBrush->GetTint(InWidgetStyle));
							//FinalColorAndOpacity.A = Alpha;

							CurWidgetsMaxLayerId++;
							FSlateDrawElement::MakeBox(
								OutDrawElements,
								CurWidgetsMaxLayerId,
								BouncedGeometry,
								OverlayBrush,
								ESlateDrawEffect::NoGamma | ESlateDrawEffect::NoPixelSnapping,
								FinalColorAndOpacity
							);
						}
					}
				}

				{
					TArray<FOverlayWidgetInfo> OverlayWidgets = ChildNode->GetOverlayWidgets(bSelected, WidgetSize);

					for (int32 WidgetIndex = 0; WidgetIndex < OverlayWidgets.Num(); ++WidgetIndex)
					{
						FOverlayWidgetInfo& OverlayInfo = OverlayWidgets[WidgetIndex];
						if (SWidget* Widget = OverlayInfo.Widget.Get())
						{
							FSlateAttributeMetaData::UpdateOnlyVisibilityAttributes(
								*Widget,
								FSlateAttributeMetaData::EInvalidationPermission::AllowInvalidationIfConstructed);
							if (Widget->GetVisibility() == EVisibility::Visible)
							{
								// call SlatePrepass as these widgets are not in the 'normal' child hierarchy
								Widget->SlatePrepass(AllottedGeometry.GetAccumulatedLayoutTransform().GetScale());

								const FGeometry WidgetGeometry = CurWidget.Geometry.MakeChild(
									OverlayInfo.OverlayOffset, Widget->GetDesiredSize());

								Widget->Paint(NewArgs, WidgetGeometry, MyCullingRect, OutDrawElements,
								              CurWidgetsMaxLayerId, InWidgetStyle, bParentEnabled);
							}
						}
					}
				}
			}

			MaxLayerId = FMath::Max(MaxLayerId, CurWidgetsMaxLayerId + 1);
		}
	}


	MaxLayerId += 1;

	// Draw connections between pins 
	if (Children.Num() > 0)
	{
		FConnectionDrawingPolicy* ConnectionDrawingPolicy = nullptr;
		if (Joint_NodeFactory.IsValid())
		{
			ConnectionDrawingPolicy = Joint_NodeFactory->CreateConnectionPolicy(
				Schema, WireLayerId, MaxLayerId, ZoomFactor, MyCullingRect, OutDrawElements, GraphObj);
		}
		else
		{
			ConnectionDrawingPolicy = FNodeFactory::CreateConnectionPolicy(
				Schema, WireLayerId, MaxLayerId, ZoomFactor, MyCullingRect, OutDrawElements, GraphObj);
		}

		TArray<TSharedPtr<SGraphPin>> OverridePins;
		for (const FGraphPinHandle& Handle : PreviewConnectorFromPins)
		{
			TSharedPtr<SGraphPin> Pin = Handle.FindInGraphPanel(*this);
			if (Pin.IsValid() && Pin->GetPinObj())
			{
				OverridePins.Add(Pin);
			}
		}
		ConnectionDrawingPolicy->SetHoveredPins(CurrentHoveredPins, OverridePins, TimeWhenMouseEnteredPin);
		ConnectionDrawingPolicy->SetMarkedPin(MarkedPin);
		ConnectionDrawingPolicy->SetMousePosition(
			AllottedGeometry.LocalToAbsolute(SavedMousePosForOnPaintEventLocalSpace));

		// Get the set of pins for all children and synthesize geometry for culled out pins so lines can be drawn to them.
		TMap<TSharedRef<SWidget>, FArrangedWidget> PinGeometries;
		TSet<TSharedRef<SWidget>> VisiblePins;
		for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(Children[ChildIndex]);

			// If this is a culled node, approximate the pin geometry to the corner of the node it is within
			if (IsNodeCulled(ChildNode, AllottedGeometry) || ChildNode->IsHidingPinWidgets())
			{
				TArray<TSharedRef<SWidget>> NodePins;
				ChildNode->GetPins(NodePins);

				const FVector2D NodeLoc = ChildNode->GetPosition();
				const FGeometry SynthesizedNodeGeometry(GraphCoordToPanelCoord(NodeLoc) * AllottedGeometry.Scale,
				                                        FVector2D(AllottedGeometry.AbsolutePosition),
				                                        FVector2D::ZeroVector, 1.f);

				for (TArray<TSharedRef<SWidget>>::TConstIterator NodePinIterator(NodePins); NodePinIterator; ++
				     NodePinIterator)
				{
					const SGraphPin& PinWidget = static_cast<const SGraphPin&>((*NodePinIterator).Get());
					if (PinWidget.GetPinObj())
					{
						FVector2D PinLoc = NodeLoc + PinWidget.GetNodeOffset();

						const FGeometry SynthesizedPinGeometry(GraphCoordToPanelCoord(PinLoc) * AllottedGeometry.Scale,
						                                       FVector2D(AllottedGeometry.AbsolutePosition),
						                                       FVector2D::ZeroVector, 1.f);
						PinGeometries.Add(*NodePinIterator, FArrangedWidget(*NodePinIterator, SynthesizedPinGeometry));
					}
				}

				// Also add synthesized geometries for culled nodes
				ArrangedChildren.AddWidget(FArrangedWidget(ChildNode, SynthesizedNodeGeometry));
			}
			else
			{
				ChildNode->GetPins(VisiblePins);
			}
		}

		// Now get the pin geometry for all visible children and append it to the PinGeometries map
		TMap<TSharedRef<SWidget>, FArrangedWidget> VisiblePinGeometries;
		{
			this->FindChildGeometries(AllottedGeometry, VisiblePins, VisiblePinGeometries);
			PinGeometries.Append(VisiblePinGeometries);
		}

		// Draw preview connections (only connected on one end)
		if (PreviewConnectorFromPins.Num() > 0)
		{
			for (const FGraphPinHandle& Handle : PreviewConnectorFromPins)
			{
				TSharedPtr<SGraphPin> CurrentStartPin = Handle.FindInGraphPanel(*this);
				if (!CurrentStartPin.IsValid() || !CurrentStartPin->GetPinObj())
				{
					continue;
				}
				const FArrangedWidget* PinGeometry = PinGeometries.Find(CurrentStartPin.ToSharedRef());

				if (PinGeometry != nullptr)
				{
					FVector2D StartPoint;
					FVector2D EndPoint;

					if (CurrentStartPin->GetDirection() == EGPD_Input)
					{
						StartPoint = AllottedGeometry.LocalToAbsolute(PreviewConnectorEndpoint);
						EndPoint = FGeometryHelper::VerticalMiddleLeftOf(PinGeometry->Geometry) - FVector2D(
							ConnectionDrawingPolicy->ArrowRadius.X, 0);
					}
					else
					{
						StartPoint = FGeometryHelper::VerticalMiddleRightOf(PinGeometry->Geometry);
						EndPoint = AllottedGeometry.LocalToAbsolute(PreviewConnectorEndpoint);
					}

					ConnectionDrawingPolicy->DrawPreviewConnector(PinGeometry->Geometry, StartPoint, EndPoint,
					                                              CurrentStartPin.Get()->GetPinObj());
				}

				//@TODO: Re-evaluate this incompatible mojo; it's mutating every pin state every frame to accomplish a visual effect
				ConnectionDrawingPolicy->SetIncompatiblePinDrawState(CurrentStartPin, VisiblePins);
			}
		}
		else
		{
			//@TODO: Re-evaluate this incompatible mojo; it's mutating every pin state every frame to accomplish a visual effect
			ConnectionDrawingPolicy->ResetIncompatiblePinDrawState(VisiblePins);
		}

		// Draw all regular connections
		ConnectionDrawingPolicy->Draw(PinGeometries, ArrangedChildren);

		// Pull back data from the drawing policy
		{
			FGraphSplineOverlapResult OverlapData = ConnectionDrawingPolicy->SplineOverlapResult;

			if (OverlapData.IsValid())
			{
				OverlapData.ComputeBestPin();

				// Only allow spline overlaps when there is no node under the cursor (unless it is a comment box)
				const FVector2D PaintAbsoluteSpaceMousePos = AllottedGeometry.LocalToAbsolute(
					SavedMousePosForOnPaintEventLocalSpace);
				const int32 HoveredNodeIndex = SWidget::FindChildUnderPosition(
					ArrangedChildren, PaintAbsoluteSpaceMousePos);
				if (HoveredNodeIndex != INDEX_NONE)
				{
					TSharedRef<SGraphNode> HoveredNode = StaticCastSharedRef<SGraphNode>(
						ArrangedChildren[HoveredNodeIndex].Widget);
					UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(
						HoveredNode->GetObjectBeingDisplayed());
					if (CommentNode == nullptr)
					{
						// Wasn't a comment node, disallow the spline interaction
						OverlapData = FGraphSplineOverlapResult(OverlapData.GetCloseToSpline());
					}
				}
			}

			// Update the spline hover state
			const_cast<SJointGraphPanel*>(this)->Joint_OnSplineHoverStateChanged(OverlapData);
		}

		delete ConnectionDrawingPolicy;
	}

	// Draw a shadow overlay around the edges of the graph
	//++MaxLayerId;
	//PaintSurroundSunkenShadow(FEditorStyle::GetBrush(TEXT("Graph.Shadow")), AllottedGeometry, MyCullingRect, OutDrawElements, MaxLayerId);

	if (ShowGraphStateOverlay.Get())
	{
		const FSlateBrush* BorderBrush = nullptr;
		if ((GEditor->bIsSimulatingInEditor || GEditor->PlayWorld != nullptr))
		{
			// Draw a surrounding indicator when PIE is active, to make it clear that the graph is read-only, etc...
			BorderBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("Graph.PlayInEditor"));
		}
		else if (!IsEditable.Get())
		{
			// Draw a different border when we're not simulating but the graph is read-only
			BorderBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("Graph.ReadOnlyBorder"));
		}

		if (BorderBrush != nullptr)
		{
			// Actually draw the border
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				MaxLayerId,
				AllottedGeometry.ToPaintGeometry(),
				BorderBrush
			);
		}
	}

	// Draw the marquee selection rectangle
	PaintMarquee(AllottedGeometry, MyCullingRect, OutDrawElements, MaxLayerId);

	// Draw the software cursor
	++MaxLayerId;

	PaintSoftwareCursor(AllottedGeometry, MyCullingRect, OutDrawElements, MaxLayerId);

#else

		// If we were provided diff results, organize those by owner
	TMap<UEdGraphNode*, FDiffSingleResult> NodeDiffResults;
	TMap<UEdGraphPin*, FDiffSingleResult> PinDiffResults;
	if (DiffResults.IsValid())
	{
		// diffs with Node1/Pin1 get precedence so set those first
		for (const FDiffSingleResult& Result : *DiffResults)
		{
			if (Result.Pin1)
			{
				PinDiffResults.FindOrAdd(Result.Pin1, Result);

				// when zoomed out, make it easier to see diffed pins by also highlighting the node
				if(ZoomLevel <= 6)
				{
					NodeDiffResults.FindOrAdd(Result.Pin1->GetOwningNode(), Result);
				}
			}
			else if (Result.Node1)
			{
				NodeDiffResults.FindOrAdd(Result.Node1, Result);
			}
		}

		// only diffs with Node2/Pin2 if those nodes don't already have a diff result
		for (const FDiffSingleResult& Result : *DiffResults)
		{
			if (Result.Pin2)
			{
				PinDiffResults.FindOrAdd(Result.Pin2, Result);

				// when zoomed out, make it easier to see diffed pins by also highlighting the node
				if(ZoomLevel <= 6)
				{
					NodeDiffResults.FindOrAdd(Result.Pin2->GetOwningNode(), Result);
				}
			}
			else if (!Result.Pin1 && Result.Node2)
			{
				NodeDiffResults.FindOrAdd(Result.Node2, Result);
			}
		}
	}

	// Draw the child nodes
	{

#if UE_VERSION_OLDER_THAN(5, 3, 0)

		// When drawing a marquee, need a preview of what the selection will be.
		const FGraphPanelSelectionSet* SelectionToVisualize = &SelectionManager.SelectedNodes;
		decltype(SelectionManager.SelectedNodes) SelectionPreview;
		if ( Marquee.IsValid() )
		{			
			ApplyMarqueeSelection(Marquee, *SelectionToVisualize, SelectionPreview);
			SelectionToVisualize = &SelectionPreview;
		}
		
#else

		// When drawing a marquee, need a preview of what the selection will be.
		const FGraphPanelSelectionSet* SelectionToVisualize = &ObjectPtrDecay(SelectionManager.SelectedNodes);
		decltype(SelectionManager.SelectedNodes) SelectionPreview;
		if ( Marquee.IsValid() )
		{			
			ApplyMarqueeSelection(Marquee, ObjectPtrDecay(SelectionManager.SelectedNodes), SelectionPreview);
			SelectionToVisualize = &ObjectPtrDecay(SelectionPreview);
		}
		
#endif

		// Context for rendering node infos
		//FKismetNodeInfoContext Context(GraphObj);

		for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
		{
			FArrangedWidget& CurWidget = ArrangedChildren[ChildIndex];
			TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
			
			// Examine node to see what layers we should be drawing in
			int32 ShadowLayerId = NodeShadowsLayerId;
			int32 ChildLayerId = NodeLayerId;

			// If a comment node, draw in the dedicated comment slots
			{
				UObject* NodeObj = ChildNode->GetObjectBeingDisplayed();
				if (NodeObj && NodeObj->IsA(UEdGraphNode_Comment::StaticClass()))
				{
					ShadowLayerId = CommentNodeShadowLayerId;
					ChildLayerId = CommentNodeLayerId;
				}
			}


			const bool bNodeIsVisible = FSlateRect::DoRectanglesIntersect( CurWidget.Geometry.GetLayoutBoundingRect(), MyCullingRect );

			if (bNodeIsVisible)
			{
				const bool bSelected = SelectionToVisualize->Contains( StaticCastSharedRef<SNodePanel::SNode>(CurWidget.Widget)->GetObjectBeingDisplayed() );
				
				UEdGraphNode* NodeObj = Cast<UEdGraphNode>(ChildNode->GetObjectBeingDisplayed());
				float Alpha = 1.0f;

				// Handle Node renaming once the node is visible
				if( bSelected && ChildNode->IsRenamePending() )
				{
					// Only open a rename when the window has focus
					TSharedPtr<SWindow> OwnerWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
					if (!OwnerWindow.IsValid() || FSlateApplication::Get().HasFocusedDescendants(OwnerWindow.ToSharedRef()))
					{
						ChildNode->ApplyRename();
					}
				}

				/** if this graph is being diffed, highlight the changes in the graph */
				
				
				if(DiffResults.IsValid())
				{
					/*
					// When diffing nodes, color code shadow based on diff result 
					if (NodeDiffResults.Contains(NodeObj))
					{
						const FDiffSingleResult& DiffResult = NodeDiffResults[NodeObj];
						for (const SNodePanel::SNode::DiffHighlightInfo& Highlight : ChildNode->GetDiffHighlights(DiffResult))
						{
							FSlateDrawElement::MakeBox(
								OutDrawElements,
								NodeDiffHighlightLayerID,
								CurWidget.Geometry.ToInflatedPaintGeometry(NodeShadowSize),
								Highlight.Brush,
								ESlateDrawEffect::None,
								Highlight.Tint
								);
						}
					}
					*/
				}
				
				
				/** When diffing, set the backround of the differing pins to their diff colors */
				for (UEdGraphPin* Pin : NodeObj->Pins)
				{
					if (TSharedPtr<SGraphPin> PinWidget = ChildNode->FindWidgetForPin(Pin))
					{
						if (FDiffSingleResult* DiffResult = PinDiffResults.Find(Pin))
						{
							// if the diff result associated with this pin is focused, highlight the pin
							if (DiffResults.IsValid() && FocusedDiffResult.IsSet())
							{
								const int32 Index = FocusedDiffResult.Get();
								if (DiffResults->IsValidIndex(Index))
								{
									const FDiffSingleResult& Focused = (*DiffResults)[Index];
									PinWidget->SetDiffHighlighted(*DiffResult == Focused);
								}
							}
						
							FLinearColor PinDiffColor = DiffResult->GetDisplayColor();
							PinDiffColor.A = 0.7f;
							PinWidget->SetPinDiffColor(PinDiffColor);
							PinWidget->SetFadeConnections(false);
						}
						else
						{
							PinWidget->SetDiffHighlighted(false);
							PinWidget->SetPinDiffColor(TOptional<FLinearColor>());

							// when zoomed out, fade out pin connections that aren't involved in a diff
							PinWidget->SetFadeConnections(ZoomLevel <= 6 && (!NodeDiffResults.Contains(NodeObj) || NodeDiffResults[NodeObj].Pin1));
						}
					}
				}

				// Draw the node's shadow.
				if (bDrawShadowsThisFrame || bSelected)
				{
					const FSlateBrush* ShadowBrush = ChildNode->GetShadowBrush(bSelected);
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						ShadowLayerId,
						CurWidget.Geometry.ToInflatedPaintGeometry(NodeShadowSize),
						ShadowBrush,
						ESlateDrawEffect::None,
						FLinearColor(1.0f, 1.0f, 1.0f, Alpha)
						);
				}

				// Draw the comments and information popups for this node, if it has any.
				{
					const SNodePanel::SNode::FNodeSlot* CommentSlot = ChildNode->GetSlot( ENodeZone::TopCenter );
					float CommentBubbleY = CommentSlot ? -CommentSlot->GetSlotOffset().Y : 0.f;
					//Context.bSelected = bSelected;
					TArray<FGraphInformationPopupInfo> Popups;

					{
						//ChildNode->GetNodeInfoPopups(&Context, /*out*/ Popups);
					}

					for (int32 PopupIndex = 0; PopupIndex < Popups.Num(); ++PopupIndex)
					{
						FGraphInformationPopupInfo& Popup = Popups[PopupIndex];
						PaintComment(Popup.Message, CurWidget.Geometry, MyCullingRect, OutDrawElements, ChildLayerId, Popup.BackgroundColor, /*inout*/ CommentBubbleY, InWidgetStyle);
					}
				}

				int32 CurWidgetsMaxLayerId;
				{
					/* When dragging off a pin, we want to duck the alpha of some nodes */
					TSharedPtr< SGraphPin > OnlyStartPin = (1 == PreviewConnectorFromPins.Num()) ? PreviewConnectorFromPins[0].FindInGraphPanel(*this) : TSharedPtr< SGraphPin >();
					const bool bNodeIsNotUsableInCurrentContext = Schema->FadeNodeWhenDraggingOffPin(NodeObj, OnlyStartPin.IsValid() ? OnlyStartPin.Get()->GetPinObj() : nullptr);
					
					const bool bCleanDiff = DiffResults.IsValid() && !NodeDiffResults.Contains(NodeObj);
					
					FWidgetStyle NodeStyleToUse = InWidgetStyle;
					if (bNodeIsNotUsableInCurrentContext)
					{
						NodeStyleToUse = FadedStyle;
					}
					else if (ZoomLevel <= 6 && bCleanDiff)
					{
						NodeStyleToUse = FadedStyle;
					}
					NodeStyleToUse.BlendColorAndOpacityTint(FLinearColor(1.0f, 1.0f, 1.0f, Alpha));

					// Draw the node.
					CurWidgetsMaxLayerId = CurWidget.Widget->Paint(NewArgs, CurWidget.Geometry, MyCullingRect, OutDrawElements, ChildLayerId, NodeStyleToUse, !Joint_DisplayAsReadOnly.Get() && ShouldBeEnabled( bParentEnabled ) );
				}

				// Draw the node's overlay, if it has one.
				{
					// Get its size
					const FVector2D WidgetSize = CurWidget.Geometry.Size;

					{
						TArray<FOverlayBrushInfo> OverlayBrushes;
						ChildNode->GetOverlayBrushes(bSelected, WidgetSize, /*out*/ OverlayBrushes);

						for (int32 BrushIndex = 0; BrushIndex < OverlayBrushes.Num(); ++BrushIndex)
						{
							FOverlayBrushInfo& OverlayInfo = OverlayBrushes[BrushIndex];
							const FSlateBrush* OverlayBrush = OverlayInfo.Brush;
							if (OverlayBrush != nullptr)
							{
								FPaintGeometry BouncedGeometry = CurWidget.Geometry.ToPaintGeometry(OverlayBrush->ImageSize, FSlateLayoutTransform(OverlayInfo.OverlayOffset));

								// Handle bouncing during PIE
								const float BounceValue = FMath::Sin(2.0f * PI * BounceCurve.GetLerp());
								BouncedGeometry.DrawPosition += FVector2f(OverlayInfo.AnimationEnvelope * BounceValue * ZoomFactor);

								FLinearColor FinalColorAndOpacity(InWidgetStyle.GetColorAndOpacityTint()* OverlayBrush->GetTint(InWidgetStyle));
								//FinalColorAndOpacity.A = Alpha;

								CurWidgetsMaxLayerId++;
								FSlateDrawElement::MakeBox(
									OutDrawElements,
									CurWidgetsMaxLayerId,
									BouncedGeometry,
									OverlayBrush,
									ESlateDrawEffect::None,
									FinalColorAndOpacity
									);
							}

						}
					}

					{
						TArray<FOverlayWidgetInfo> OverlayWidgets = ChildNode->GetOverlayWidgets(bSelected, WidgetSize);

						for (int32 WidgetIndex = 0; WidgetIndex < OverlayWidgets.Num(); ++WidgetIndex)
						{
							FOverlayWidgetInfo& OverlayInfo = OverlayWidgets[WidgetIndex];
							if (SWidget* Widget = OverlayInfo.Widget.Get())
							{
								FSlateAttributeMetaData::UpdateOnlyVisibilityAttributes(*Widget, FSlateAttributeMetaData::EInvalidationPermission::AllowInvalidationIfConstructed);
								if (Widget->GetVisibility() == EVisibility::Visible)
								{
									// call SlatePrepass as these widgets are not in the 'normal' child hierarchy
									Widget->SlatePrepass(AllottedGeometry.GetAccumulatedLayoutTransform().GetScale());

									const FGeometry WidgetGeometry = CurWidget.Geometry.MakeChild(Widget->GetDesiredSize(), FSlateLayoutTransform(OverlayInfo.OverlayOffset));

									Widget->Paint(NewArgs, WidgetGeometry, MyCullingRect, OutDrawElements, CurWidgetsMaxLayerId, InWidgetStyle, bParentEnabled);
								}
							}
						}
					}
				}

				MaxLayerId = FMath::Max( MaxLayerId, CurWidgetsMaxLayerId + 1 );
			}
		}
	}

	MaxLayerId += 1;

	// Draw connections between pins 
	if (Children.Num() > 0 )
	{
		FConnectionDrawingPolicy* ConnectionDrawingPolicy = nullptr;
		if (Joint_NodeFactory.IsValid())
		{
			ConnectionDrawingPolicy = Joint_NodeFactory->CreateConnectionPolicy(Schema, WireLayerId, MaxLayerId, ZoomFactor, MyCullingRect, OutDrawElements, GraphObj);
		}
		else
		{
			ConnectionDrawingPolicy = FNodeFactory::CreateConnectionPolicy(Schema, WireLayerId, MaxLayerId, ZoomFactor, MyCullingRect, OutDrawElements, GraphObj);
		}

		//Don't allow caching.
		
		const bool bUseDrawStateCaching = false;
		
		TArray<TSharedPtr<SGraphPin>> OverridePins;
		for (const FGraphPinHandle& Handle : PreviewConnectorFromPins)
		{
			TSharedPtr<SGraphPin> Pin = Handle.FindInGraphPanel(*this);
			if (Pin.IsValid() && Pin->GetPinObj())
			{
				OverridePins.Add(Pin);
			}
		}
		ConnectionDrawingPolicy->SetHoveredPins(CurrentHoveredPins, OverridePins, TimeWhenMouseEnteredPin);
		ConnectionDrawingPolicy->SetMarkedPin(MarkedPin);
		ConnectionDrawingPolicy->SetMousePosition(AllottedGeometry.LocalToAbsolute(SavedMousePosForOnPaintEventLocalSpace));

		// Get the set of pins for all children and synthesize geometry for culled out pins so lines can be drawn to them.
		TMap<TSharedRef<SWidget>, FArrangedWidget> PinGeometries;
		TSet< TSharedRef<SWidget> > VisiblePins;
		for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(Children[ChildIndex]);

			// If this is a culled node, approximate the pin geometry to the corner of the node it is within
			if (IsNodeCulled(ChildNode, AllottedGeometry) || ChildNode->IsHidingPinWidgets())
			{
				TArray< TSharedRef<SWidget> > NodePins;
				ChildNode->GetPins(NodePins);

				const FVector2D NodeLoc = ChildNode->GetPosition();
				const FGeometry SynthesizedNodeGeometry(GraphCoordToPanelCoord(NodeLoc) * AllottedGeometry.Scale, FVector2D(AllottedGeometry.AbsolutePosition), FVector2D::ZeroVector, 1.f);

				for (TArray< TSharedRef<SWidget> >::TConstIterator NodePinIterator(NodePins); NodePinIterator; ++NodePinIterator)
				{
					const SGraphPin& PinWidget = static_cast<const SGraphPin&>((*NodePinIterator).Get());
					if (PinWidget.GetPinObj())
					{
						FVector2D PinLoc = NodeLoc + PinWidget.GetNodeOffset();

						const FGeometry SynthesizedPinGeometry(GraphCoordToPanelCoord(PinLoc) * AllottedGeometry.Scale, FVector2D(AllottedGeometry.AbsolutePosition), FVector2D::ZeroVector, 1.f);
						PinGeometries.Add(*NodePinIterator, FArrangedWidget(*NodePinIterator, SynthesizedPinGeometry));
					}
				}

				// Also add synthesized geometries for culled nodes
				ArrangedChildren.AddWidget( FArrangedWidget(ChildNode, SynthesizedNodeGeometry) );
			}
			else
			{
				ChildNode->GetPins(VisiblePins);
			}
		}

		// Now get the pin geometry for all visible children and append it to the PinGeometries map
		TMap<TSharedRef<SWidget>, FArrangedWidget> VisiblePinGeometries;
		{
			this->FindChildGeometries(AllottedGeometry, VisiblePins, VisiblePinGeometries);
			PinGeometries.Append(VisiblePinGeometries);
		}

		// Draw preview connections (only connected on one end)
		if (PreviewConnectorFromPins.Num() > 0)
		{
			for (const FGraphPinHandle& Handle : PreviewConnectorFromPins)
			{
				TSharedPtr< SGraphPin > CurrentStartPin = Handle.FindInGraphPanel(*this);
				if (!CurrentStartPin.IsValid() || !CurrentStartPin->GetPinObj())
				{
					continue;
				}
				const FArrangedWidget* PinGeometry = PinGeometries.Find( CurrentStartPin.ToSharedRef() );

				if (PinGeometry != nullptr)
				{
					FVector2D StartPoint;
					FVector2D EndPoint;

					if (CurrentStartPin->GetDirection() == EGPD_Input)
					{
						StartPoint = AllottedGeometry.LocalToAbsolute(PreviewConnectorEndpoint);
						EndPoint = FGeometryHelper::VerticalMiddleLeftOf( PinGeometry->Geometry ) - FVector2D(ConnectionDrawingPolicy->ArrowRadius.X, 0);
					}
					else
					{
						StartPoint = FGeometryHelper::VerticalMiddleRightOf( PinGeometry->Geometry );
						EndPoint = AllottedGeometry.LocalToAbsolute(PreviewConnectorEndpoint);
					}

					ConnectionDrawingPolicy->DrawPreviewConnector(PinGeometry->Geometry, StartPoint, EndPoint, CurrentStartPin.Get()->GetPinObj());
				}

				if (bUseDrawStateCaching)
				{
					//@TODO: Re-evaluate this incompatible mojo; it's mutating every pin state every frame to accomplish a visual effect
					ConnectionDrawingPolicy->SetIncompatiblePinDrawState(CurrentStartPin, VisiblePins);
				}
			}
		}
		else
		{
			//@TODO: Re-evaluate this incompatible mojo; it's mutating every pin state every frame to accomplish a visual effect
			ConnectionDrawingPolicy->ResetIncompatiblePinDrawState(VisiblePins);
		}

		// Draw all regular connections
		ConnectionDrawingPolicy->Draw(PinGeometries, ArrangedChildren);

		// Pull back data from the drawing policy
		{
			FGraphSplineOverlapResult OverlapData = ConnectionDrawingPolicy->SplineOverlapResult;

			if (OverlapData.IsValid())
			{
				OverlapData.ComputeBestPin();

				// Only allow spline overlaps when there is no node under the cursor (unless it is a comment box)
				const FVector2D PaintAbsoluteSpaceMousePos = AllottedGeometry.LocalToAbsolute(SavedMousePosForOnPaintEventLocalSpace);
				const int32 HoveredNodeIndex = SWidget::FindChildUnderPosition(ArrangedChildren, PaintAbsoluteSpaceMousePos);
				if (HoveredNodeIndex != INDEX_NONE)
				{
					TSharedRef<SGraphNode> HoveredNode = StaticCastSharedRef<SGraphNode>(ArrangedChildren[HoveredNodeIndex].Widget);
					UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(HoveredNode->GetObjectBeingDisplayed());
					if (CommentNode == nullptr)
					{
						// Wasn't a comment node, disallow the spline interaction
						OverlapData = FGraphSplineOverlapResult(OverlapData.GetCloseToSpline());
					}
				}
			}
			
			const_cast<SJointGraphPanel*>(this)->Joint_OnSplineHoverStateChanged(OverlapData);
			
		}

		delete ConnectionDrawingPolicy;
	}

	// Draw a shadow overlay around the edges of the graph
	//++MaxLayerId;
	//PaintSurroundSunkenShadow(FAppStyle::GetBrush(TEXT("Graph.Shadow")), AllottedGeometry, MyCullingRect, OutDrawElements, MaxLayerId);

	if (ShowGraphStateOverlay.Get())
	{
		const FSlateBrush* BorderBrush = nullptr;
		if ((GEditor->bIsSimulatingInEditor || GEditor->PlayWorld != nullptr))
		{
			// Draw a surrounding indicator when PIE is active, to make it clear that the graph is read-only, etc...
			BorderBrush = FAppStyle::GetBrush(TEXT("Graph.PlayInEditor"));
		}
		else if (!IsEditable.Get())
		{
			// Draw a different border when we're not simulating but the graph is read-only
			BorderBrush = FAppStyle::GetBrush(TEXT("Graph.ReadOnlyBorder"));
		}

		if (BorderBrush != nullptr)
		{
			// Actually draw the border
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				MaxLayerId,
				AllottedGeometry.ToPaintGeometry(),
				BorderBrush
				);
		}
	}

	// Draw the marquee selection rectangle
	PaintMarquee(AllottedGeometry, MyCullingRect, OutDrawElements, MaxLayerId);

	// Draw the software cursor
	++MaxLayerId;
	PaintSoftwareCursor(AllottedGeometry, MyCullingRect, OutDrawElements, MaxLayerId);

#endif

	return MaxLayerId;
}

void SJointGraphPanel::SetAllowContinousZoomInterpolation(bool bAllow)
{
	bAllowContinousZoomInterpolation = bAllow;
}

float SJointGraphPanel::Joint_FancyMod(float Value, float Size)
{
	return ((Value >= 0) ? 0.0f : Size) + FMath::Fmod(Value, Size);
}

void SJointGraphPanel::SetNodeFactory(const TSharedRef<FGraphNodeFactory>& NewNodeFactory)
{
	Joint_NodeFactory = NewNodeFactory;

	SGraphPanel::SetNodeFactory(NewNodeFactory);
}

void SJointGraphPanel::Joint_OnSplineHoverStateChanged(const FGraphSplineOverlapResult& NewSplineHoverState)
{
	TSharedPtr<SGraphPin> OldPinWidget = PreviousFrameSplineOverlap.GetBestPinWidget(*this);
	PreviousFrameSplineOverlap = NewSplineHoverState;
	TSharedPtr<SGraphPin> NewPinWidget = PreviousFrameSplineOverlap.GetBestPinWidget(*this);

	PreviousFrameSavedMousePosForSplineOverlap = SavedMousePosForOnPaintEventLocalSpace;

	// Handle mouse enter/leaves on the associated pin
	if (OldPinWidget != NewPinWidget)
	{
		if (OldPinWidget.IsValid())
		{
			OldPinWidget->OnMouseLeave(LastPointerEvent);
		}

		if (NewPinWidget.IsValid())
		{
			NewPinWidget->OnMouseEnter(LastPointerGeometry, LastPointerEvent);

			// Get the pin/wire glowing quicker, since it's a direct selection (this time was already set to 'now' as part of entering the pin)
			//@TODO: Source this parameter from the graph rendering settings once it is there (see code in ApplyHoverDeemphasis)
			TimeWhenMouseEnteredPin -= 0.75f;
		}
	}
}
