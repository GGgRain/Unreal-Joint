//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointGraphConnectionDrawingPolicy.h"
#include "Rendering/DrawElements.h"
#include "GraphSplineOverlapResult.h"
#include "JointEdGraphNode_Connector.h"
#include "JointEdGraphNode_Manager.h"
#include "JointEdGraphSchema.h"
#include "JointEditorLogChannels.h"
#include "JointEditorSettings.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"

#include "Misc/EngineVersionComparison.h"


namespace JointWiggleWireGlobals
{
	TMap<FGuid, TMap<FGraphWireId, TSharedPtr<FWiggleWireSimulator>>> GraphWireSimulators;
}


FJointGraphConnectionDrawingPolicy::FJointGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID,
                                                                       float ZoomFactor,
                                                                       const FSlateRect& InClippingRect,
                                                                       FSlateWindowElementList& InDrawElements,
                                                                       UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements),
	  GraphObj(InGraphObj),

	  WireSimulators(JointWiggleWireGlobals::GraphWireSimulators.FindOrAdd(InGraphObj->GraphGuid)),
	  TimeSinceLastPrune(0.0f),

	  bUseWiggleWireForNormalConnection(UJointEditorSettings::Get()->bUseWiggleWireForNormalConnection),
	  bUseWiggleWireForRecursiveConnection(
		  UJointEditorSettings::Get()->bUseWiggleWireForRecursiveConnection),
	  bUseWiggleWireForSelfConnection(
		  UJointEditorSettings::Get()->bUseWiggleWireForSelfConnection),
	  bUseWiggleWireForPreviewConnection(
		  UJointEditorSettings::Get()->bUseWiggleWireForPreviewConnection),

	  NormalConnectionWiggleWireConfig(
		  UJointEditorSettings::Get()->NormalConnectionWiggleWireConfig),
	  RecursiveConnectionWiggleWireConfig(
		  UJointEditorSettings::Get()->RecursiveConnectionWiggleWireConfig),
	  SelfConnectionWiggleWireConfig(
		  UJointEditorSettings::Get()->SelfConnectionWiggleWireConfig),
	  PreviewConnectionWiggleWireConfig(
		  UJointEditorSettings::Get()->PreviewConnectionWiggleWireConfig)


{
	CalHierarchyMap();
}

const EJointGraphConnectionDrawingConnectionType FJointGraphConnectionDrawingPolicy::GetConnectionType(
	const UEdGraphPin* OutputPin, const UEdGraphPin* InputPin)
{
	if (UJointEditorSettings* InSettings = UJointEditorSettings::Get())
	{
		if (!OutputPin || !InputPin) return EJointGraphConnectionDrawingConnectionType::Preview;

		const UEdGraphNode* FromNode = OutputPin->GetOwningNodeUnchecked();
		const UEdGraphNode* ToNode = InputPin->GetOwningNodeUnchecked();

		if (CheckIsSame(FromNode, ToNode)) return EJointGraphConnectionDrawingConnectionType::Self;
		if (CheckIsRecursive(FromNode, ToNode)) return EJointGraphConnectionDrawingConnectionType::Recursive;

		return EJointGraphConnectionDrawingConnectionType::Normal;
	}

	return EJointGraphConnectionDrawingConnectionType::Normal;
}

const EJointGraphConnectionDrawingConnectionType FJointGraphConnectionDrawingPolicy::GetConnectionType(
	const FConnectionParams& Params)
{
	return GetConnectionType(Params.AssociatedPin1, Params.AssociatedPin2);
}

void FJointGraphConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/
                                                              FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;

	Params.WireThickness = UJointEditorSettings::Get()->PinConnectionThickness;

	const EJointGraphConnectionDrawingConnectionType& Type = GetConnectionType(Params);

	if (UJointEditorSettings* InSettings = UJointEditorSettings::Get())
	{
		const UEdGraphNode* FromNode = OutputPin ? OutputPin->GetOwningNodeUnchecked() : nullptr;
		const UEdGraphNode* ToNode = InputPin ? InputPin->GetOwningNodeUnchecked() : nullptr;

		switch (Type)
		{
		case EJointGraphConnectionDrawingConnectionType::Normal:
			Params.WireColor = InSettings->NormalConnectionColor;
			Params.WireColor.A = !InSettings->bDrawNormalConnection
				                     ? 0.1
				                     : !CheckIsInActiveRoute(FromNode)
				                     ? InSettings->NotReachableRouteConnectionOpacity
				                     : Params.WireColor.A;
			break;
		case EJointGraphConnectionDrawingConnectionType::Recursive:
			Params.WireColor = InSettings->RecursiveConnectionColor;
			Params.WireColor.A = !InSettings->bDrawRecursiveConnection
				                     ? 0.1
				                     : !CheckIsInActiveRoute(FromNode)
				                     ? InSettings->NotReachableRouteConnectionOpacity
				                     : Params.WireColor.A;
			break;
		case EJointGraphConnectionDrawingConnectionType::Preview:
			Params.WireColor = InSettings->PreviewConnectionColor;
			break;
		case EJointGraphConnectionDrawingConnectionType::Self:
			Params.WireColor = InSettings->SelfConnectionColor;
			Params.WireColor.A = !CheckIsInActiveRoute(FromNode)
				                     ? InSettings->NotReachableRouteConnectionOpacity
				                     : Params.WireColor.A;
			break;
		}
	}

	// Skip deemphasis if the connection is a preview connection: it will work not as expected
	if (Type != EJointGraphConnectionDrawingConnectionType::Preview && HoveredPins.Num() > 0) ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
}

bool FJointGraphConnectionDrawingPolicy::CheckIsRecursive(const UEdGraphNode* FromNode,
                                                          const UEdGraphNode* ToNode) const
{
	if (!FromNode || !ToNode) return false;

	if (!NodeHierarchyMap.Contains(FromNode) || !NodeHierarchyMap.Contains(ToNode)) return false;

	return NodeHierarchyMap[FromNode] >= NodeHierarchyMap[ToNode];
}

bool FJointGraphConnectionDrawingPolicy::CheckIsSame(const UEdGraphNode* FromNode, const UEdGraphNode* ToNode) const
{
	if (!FromNode || !ToNode) return false;

	return FromNode == ToNode;
}

bool FJointGraphConnectionDrawingPolicy::CheckIsInActiveRoute(const UEdGraphNode* Node) const
{
	return NodeHierarchyMap.Contains(Node);
}


void CollectHierarchyFrom(UEdGraphNode* Node, TMap<UEdGraphNode*, int32>& Map, int ParentIndex = -1)
{
	if (!Node) return;

	if (Map.Contains(Node)) return;

	Map.Add(Node, ParentIndex + 1);

	for (UEdGraphPin* Pin : Node->GetAllPins())
	{
		if (Pin->Direction != EEdGraphPinDirection::EGPD_Output) continue;

		for (UEdGraphPin* TestPin : Pin->LinkedTo)
		{
			CollectHierarchyFrom(TestPin->GetOwningNodeUnchecked(), Map, ParentIndex + 1);
		}
	}
}

void FJointGraphConnectionDrawingPolicy::CalHierarchyMap()
{
	if (!GraphObj) return;

	TArray<UJointEdGraphNode_Manager*> RootNodes;

	GraphObj->GetNodesOfClass<UJointEdGraphNode_Manager>(RootNodes);

	for (UJointEdGraphNode_Manager* Root : RootNodes)
	{
		CollectHierarchyFrom(Root, NodeHierarchyMap);
	}

	TArray<UJointEdGraphNode_Connector*> Connectors;

	GraphObj->GetNodesOfClass<UJointEdGraphNode_Connector>(Connectors);

	for (UJointEdGraphNode_Connector* Connector : Connectors)
	{
		CollectHierarchyFrom(Connector, NodeHierarchyMap);
	}
}

#include "Misc/EngineVersionComparison.h"

void FJointGraphConnectionDrawingPolicy::DrawTangentConnection(int32 LayerId, const FVector2D& Start,
                                                               const FVector2D& End, const FConnectionParams& Params)
{
	const FVector2D& P0 = Start;
	const FVector2D& P1 = End;

	const FVector2D SplineTangent = ComputeJointSplineTangent(P0, P1, Params);
	const FVector2D P0Tangent = (Params.StartDirection == EGPD_Output) ? SplineTangent : -SplineTangent;
	const FVector2D P1Tangent = (Params.EndDirection == EGPD_Input) ? SplineTangent : -SplineTangent;

	if (Settings->bTreatSplinesLikePins)
	{
		// Distance to consider as an overlap
		const float QueryDistanceTriggerThresholdSquared = FMath::Square(
			Settings->SplineHoverTolerance + Params.WireThickness * 0.5f);

		// Distance to pass the bounding box cull test (may want to expand this later on if we want to do 'closest pin' actions that don't require an exact hit)
		const float QueryDistanceToBoundingBoxSquared = QueryDistanceTriggerThresholdSquared;

		bool bCloseToSpline = false;
		{
			// The curve will include the endpoints but can extend out of a tight bounds because of the tangents
			// P0Tangent coefficient maximizes to 4/27 at a=1/3, and P1Tangent minimizes to -4/27 at a=2/3.
			const float MaximumTangentContribution = 4.0f / 27.0f;
			FBox2D Bounds(ForceInit);

			Bounds += FVector2D(P0);
			Bounds += FVector2D(P0 + MaximumTangentContribution * P0Tangent);
			Bounds += FVector2D(P1);
			Bounds += FVector2D(P1 - MaximumTangentContribution * P1Tangent);

			bCloseToSpline = Bounds.ComputeSquaredDistanceToPoint(LocalMousePosition) <
				QueryDistanceToBoundingBoxSquared;

			// Draw the bounding box for debugging
		}

		if (bCloseToSpline)
		{
			// Find the closest approach to the spline
			FVector2D ClosestPoint(ForceInit);
			float ClosestDistanceSquared = FLT_MAX;

			const int32 NumStepsToTest = 16;
			const float StepInterval = 1.0f / (float)NumStepsToTest;
			FVector2D Point1 = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, 0.0f);
			for (float TestAlpha = 0.0f; TestAlpha < 1.0f; TestAlpha += StepInterval)
			{
				const FVector2D Point2 = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, TestAlpha + StepInterval);

#if UE_VERSION_OLDER_THAN(5, 6, 0)
				const FVector2D ClosestPointToSegment = FMath::ClosestPointOnSegment2D(
					LocalMousePosition, Point1, Point2);
#else
				const FVector2D ClosestPointToSegment = FMath::ClosestPointOnSegment2D<double>(
					LocalMousePosition, Point1, Point2);
#endif

				const float DistanceSquared = (LocalMousePosition - ClosestPointToSegment).SizeSquared();

				if (DistanceSquared < ClosestDistanceSquared)
				{
					ClosestDistanceSquared = DistanceSquared;
					ClosestPoint = ClosestPointToSegment;
				}

				Point1 = Point2;
			}

			// Record the overlap
			if (ClosestDistanceSquared < QueryDistanceTriggerThresholdSquared)
			{
				if (ClosestDistanceSquared < SplineOverlapResult.GetDistanceSquared())
				{
					const float SquaredDistToPin1 = (Params.AssociatedPin1 != nullptr)
						                                ? (P0 - ClosestPoint).SizeSquared()
						                                : FLT_MAX;
					const float SquaredDistToPin2 = (Params.AssociatedPin2 != nullptr)
						                                ? (P1 - ClosestPoint).SizeSquared()
						                                : FLT_MAX;


#if UE_VERSION_OLDER_THAN(4, 28, 0)
					SplineOverlapResult = FGraphSplineOverlapResult(Params.AssociatedPin1, Params.AssociatedPin2, ClosestDistanceSquared, SquaredDistToPin1, SquaredDistToPin2);
#else
					SplineOverlapResult = FGraphSplineOverlapResult(Params.AssociatedPin1, Params.AssociatedPin2,
					                                                ClosestDistanceSquared, SquaredDistToPin1,
					                                                SquaredDistToPin2, false);
#endif
				}
			}
		}
	}

	// Draw the spline itself
	FSlateDrawElement::MakeDrawSpaceSpline(DrawElementsList, LayerId, P0, P0Tangent, P1, P1Tangent,
	                                       Params.WireThickness, ESlateDrawEffect::NoPixelSnapping, Params.WireColor);

	if (Params.bDrawBubbles || (MidpointImage != nullptr))
	{
		// This table maps distance along curve to alpha
		FInterpCurve<float> SplineReparamTable;
		const float SplineLength = MakeSplineReparamTable(P0, P0Tangent, P1, P1Tangent, SplineReparamTable);

		// Draw bubbles on the spline
		if (Params.bDrawBubbles)
		{
			const float BubbleSpacing = 64.f * ZoomFactor;
			const float BubbleSpeed = 192.f * ZoomFactor;
			const FVector2D BubbleSize = BubbleImage->ImageSize * ZoomFactor * 0.2f * Params.WireThickness;

			float Time = (FPlatformTime::Seconds() - GStartTime);
			const float BubbleOffset = FMath::Fmod(Time * BubbleSpeed, BubbleSpacing);
			const int32 NumBubbles = FMath::CeilToInt(SplineLength / BubbleSpacing);
			for (int32 i = 0; i < NumBubbles; ++i)
			{
				const float Distance = ((float)i * BubbleSpacing) + BubbleOffset;
				if (Distance < SplineLength)
				{
					const float Alpha = SplineReparamTable.Eval(Distance, 0.f);
					FVector2D BubblePos = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, Alpha);
					BubblePos -= (BubbleSize * 0.5f);

					FSlateDrawElement::MakeBox(DrawElementsList, LayerId,
					                           FPaintGeometry(BubblePos, BubbleSize, ZoomFactor), BubbleImage,
					                           ESlateDrawEffect::NoPixelSnapping, Params.WireColor);
				}
			}
		}

		// Draw the midpoint image
		if (MidpointImage != nullptr)
		{
			// Determine the spline position for the midpoint
			const float MidpointAlpha = SplineReparamTable.Eval(SplineLength * 0.5f, 0.f);
			const FVector2D Midpoint = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, MidpointAlpha);

			// Approximate the slope at the midpoint (to orient the midpoint image to the spline)
			const FVector2D MidpointPlusE = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent,
			                                                   MidpointAlpha + KINDA_SMALL_NUMBER);
			const FVector2D MidpointMinusE = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent,
			                                                    MidpointAlpha - KINDA_SMALL_NUMBER);
			const FVector2D SlopeUnnormalized = MidpointPlusE - MidpointMinusE;

			// Draw the arrow
			const FVector2D MidpointDrawPos = Midpoint - MidpointRadius;
			const float AngleInRadians = SlopeUnnormalized.IsNearlyZero()
				                             ? 0.0f
				                             : FMath::Atan2(SlopeUnnormalized.Y, SlopeUnnormalized.X);

			FSlateDrawElement::MakeRotatedBox(DrawElementsList, LayerId,
			                                  FPaintGeometry(MidpointDrawPos, MidpointImage->ImageSize * ZoomFactor,
			                                                 ZoomFactor), MidpointImage,
			                                  ESlateDrawEffect::NoPixelSnapping, AngleInRadians, TOptional<FVector2D>(),
			                                  FSlateDrawElement::RelativeToElement, Params.WireColor);
		}
	}
}

void FJointGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End,
                                                        const FConnectionParams& Params)
{
	const EJointGraphConnectionDrawingConnectionType& Type = GetConnectionType(Params);

	bool bUseWiggle = false;
	const FWiggleWireConfig* CurrentWiggleConfig = &NormalConnectionWiggleWireConfig;

	switch (Type)
	{
	case EJointGraphConnectionDrawingConnectionType::Normal:
		bUseWiggle = bUseWiggleWireForNormalConnection;
		CurrentWiggleConfig = &NormalConnectionWiggleWireConfig;
		break;
	case EJointGraphConnectionDrawingConnectionType::Recursive:
		bUseWiggle = bUseWiggleWireForRecursiveConnection;
		CurrentWiggleConfig = &RecursiveConnectionWiggleWireConfig;
		break;
	case EJointGraphConnectionDrawingConnectionType::Preview:
		bUseWiggle = bUseWiggleWireForPreviewConnection;
		CurrentWiggleConfig = &PreviewConnectionWiggleWireConfig;
		break;
	case EJointGraphConnectionDrawingConnectionType::Self:
		bUseWiggle = bUseWiggleWireForSelfConnection;
		CurrentWiggleConfig = &SelfConnectionWiggleWireConfig;
		break;
	}

	if (bUseWiggle)
	{
		DrawWiggleConnection(LayerId, Start, End, Params, *CurrentWiggleConfig);
	}
	else
	{
		DrawTangentConnection(LayerId, Start, End, Params);
	}
}

void FJointGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries,
                                              FArrangedChildren& ArrangedNodes)
{
	// Build an acceleration structure to quickly find geometry for the nodes
	NodeWidgetMap.Empty();
	for (int32 NodeIndex = 0; NodeIndex < ArrangedNodes.Num(); ++NodeIndex)
	{
		FArrangedWidget& CurWidget = ArrangedNodes[NodeIndex];
		TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
		NodeWidgetMap.Add(ChildNode->GetNodeObj(), NodeIndex);
	}

	// Now draw
	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

void FJointGraphConnectionDrawingPolicy::DrawPinGeometries(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries,
                                                           FArrangedChildren& ArrangedNodes)
{
	FConnectionDrawingPolicy::DrawPinGeometries(InPinGeometries, ArrangedNodes);
}

void FJointGraphConnectionDrawingPolicy::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint,
                                                              const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	FConnectionParams Params;
	Params.bDrawBubbles = true;

	DetermineWiringStyle(nullptr, Pin, Params);

	if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		DrawConnection(
			WireLayerID,
			FGeometryHelper::VerticalMiddleLeftOf(PinGeometry) - FVector2D(
				JointGraphConnectionDrawingPolicyConstants::StartFudgeX * ZoomFactor, 0.0f),
			EndPoint,
			Params);
	}
	else
	{
		DrawConnection(
			WireLayerID,
			FGeometryHelper::VerticalMiddleRightOf(PinGeometry) - FVector2D(
				ArrowRadius.X + JointGraphConnectionDrawingPolicyConstants::EndFudgeX * ZoomFactor, 0),
			StartPoint,
			Params);
	}
}

void FJointGraphConnectionDrawingPolicy::DrawSplineWithArrow(const FVector2D& StartAnchorPoint,
                                                             const FVector2D& EndAnchorPoint,
                                                             const FConnectionParams& Params)
{
	// bUserFlag1 indicates that we need to reverse the direction of connection (used by debugger)
	const FVector2D& P0 = Params.bUserFlag1 ? EndAnchorPoint : StartAnchorPoint;
	const FVector2D& P1 = Params.bUserFlag1 ? StartAnchorPoint : EndAnchorPoint;

	Internal_DrawLineWithArrow(P0, P1, Params);
}

void FJointGraphConnectionDrawingPolicy::Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint,
                                                                    const FVector2D& EndAnchorPoint,
                                                                    const FConnectionParams& Params)
{
	//@TODO: Should this be scaled by zoom factor?
	const float LineSeparationAmount = 4.5f;

	const FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
	const FVector2D UnitDelta = DeltaPos.GetSafeNormal();
	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	// Come up with the final start/end points
	const FVector2D StartPoint = StartAnchorPoint;
	const FVector2D EndPoint = EndAnchorPoint;

	// Draw a line/spline
	DrawConnection(WireLayerID, StartPoint, EndPoint, Params);

	// Draw the arrow
	const FVector2D ArrowDrawPos = EndPoint - ArrowRadius;
	const float AngleInRadians = FMath::Atan2(DeltaPos.Y, DeltaPos.X);

	FSlateDrawElement::MakeRotatedBox(DrawElementsList, ArrowLayerID,
	                                  FPaintGeometry(ArrowDrawPos, ArrowImage->ImageSize * ZoomFactor, ZoomFactor),
	                                  ArrowImage, ESlateDrawEffect::NoPixelSnapping, AngleInRadians,
	                                  TOptional<FVector2D>(), FSlateDrawElement::RelativeToElement, Params.WireColor);
}

void FJointGraphConnectionDrawingPolicy::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom,
                                                             const FConnectionParams& Params)
{
	const FVector2D StartPoint = FGeometryHelper::VerticalMiddleLeftOf(StartGeom) - FVector2D(
		JointGraphConnectionDrawingPolicyConstants::StartFudgeX * ZoomFactor, 0.0f);
	const FVector2D EndPoint = FGeometryHelper::VerticalMiddleRightOf(EndGeom) - FVector2D(
		ArrowRadius.X - JointGraphConnectionDrawingPolicyConstants::EndFudgeX * ZoomFactor, 0);

	DrawSplineWithArrow(StartPoint, EndPoint, Params);
}

void FJointGraphConnectionDrawingPolicy::ApplyHoverDeemphasis(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/
                                                              float& Thickness, /*inout*/ FLinearColor& WireColor)
{
	UJointEditorSettings* InSettings = UJointEditorSettings::Get();

	if (!InSettings) return;

	const float& ConnectionHighlightFadeBias = InSettings->ConnectionHighlightFadeBias;
	const float& ConnectionHighlightedFadeInPeriod = InSettings->ConnectionHighlightedFadeInPeriod;
	const float& PinConnectionThickness = InSettings->PinConnectionThickness;
	const float& HighlightedPinConnectionThickness = InSettings->HighlightedPinConnectionThickness;

	const FLinearColor& HighlightedConnectionColor = InSettings->HighlightedConnectionColor;

	const float& NotHighlightedConnectionOpacity = InSettings->NotHighlightedConnectionOpacity;


	//@TODO: Move these parameters into the settings object
	const float FadeInBias = ConnectionHighlightFadeBias; // Time in seconds before the fading starts to occur
	const float FadeInPeriod = ConnectionHighlightedFadeInPeriod;
	// Time in seconds after the bias before the fade is fully complete
	const float TimeFraction = FMath::SmoothStep(0.0f, FadeInPeriod,
	                                             (float)(FSlateApplication::Get().GetCurrentTime() - LastHoverTimeEvent
		                                             - FadeInBias));

	const bool bContainsBoth = HoveredPins.Contains(InputPin) && HoveredPins.Contains(OutputPin);
	const bool bContainsOutput = HoveredPins.Contains(OutputPin);
	const bool bEmphasize = bContainsBoth || (bContainsOutput && (InputPin == nullptr));

	if (bEmphasize)
	{
		Thickness = FMath::Lerp(PinConnectionThickness, HighlightedPinConnectionThickness, TimeFraction);

		WireColor = FMath::Lerp<FLinearColor>(WireColor, HighlightedConnectionColor, TimeFraction);
	}
	else
	{
		FLinearColor TargetColor = WireColor;

		TargetColor.A = NotHighlightedConnectionOpacity;

		WireColor = FMath::Lerp<FLinearColor>(WireColor, TargetColor, TimeFraction);
	}
}


FVector2D FJointGraphConnectionDrawingPolicy::ComputeStraightLineTangent(
	const FVector2D& Start, const FVector2D& End) const
{
	const FVector2D Delta = End - Start;
	const FVector2D NormDelta = Delta.GetSafeNormal();

	return NormDelta;
}

FVector2D FJointGraphConnectionDrawingPolicy::ComputeJointSplineTangent(const FVector2D& Start, const FVector2D& End,
                                                                        const FConnectionParams& Params) const
{
	const FVector2D DeltaPos = End - Start;
	const bool bGoingForward = DeltaPos.X >= 0.0f;

	bool bIsSelf = Params.AssociatedPin1
		&& Params.AssociatedPin2
		&& Params.AssociatedPin1->GetOwningNodeUnchecked() != nullptr
		&& Params.AssociatedPin1->GetOwningNodeUnchecked() == Params.AssociatedPin2->GetOwningNodeUnchecked();

	if (const UJointEditorSettings* InSettings = UJointEditorSettings::Get())
	{
		if (bIsSelf)
		{
			const float ClampedTensionX = FMath::Min<float>(FMath::Abs<float>(DeltaPos.X),
			                                                InSettings->SelfSplineHorizontalDeltaRange);
			const float ClampedTensionY = FMath::Min<float>(FMath::Abs<float>(DeltaPos.Y),
			                                                InSettings->SelfSplineVerticalDeltaRange);

			return (ClampedTensionX * InSettings->SelfSplineTangentFromHorizontalDelta) + (ClampedTensionY * InSettings
				->SelfSplineTangentFromVerticalDelta);
		}

		if (bGoingForward)
		{
			const float ClampedTensionX = FMath::Min<float>(FMath::Abs<float>(DeltaPos.X),
			                                                InSettings->ForwardSplineHorizontalDeltaRange);
			const float ClampedTensionY = FMath::Min<float>(FMath::Abs<float>(DeltaPos.Y),
			                                                InSettings->ForwardSplineHorizontalDeltaRange);

			return (ClampedTensionX * InSettings->ForwardSplineTangentFromHorizontalDelta) + (ClampedTensionY *
				InSettings->ForwardSplineTangentFromVerticalDelta);
		}
		else
		{
			const float ClampedTensionX = FMath::Min<float>(FMath::Abs<float>(DeltaPos.X),
			                                                InSettings->BackwardSplineHorizontalDeltaRange);
			const float ClampedTensionY = FMath::Min<float>(FMath::Abs<float>(DeltaPos.Y),
			                                                InSettings->BackwardSplineHorizontalDeltaRange);

			return (ClampedTensionX * InSettings->BackwardSplineTangentFromHorizontalDelta) + (ClampedTensionY *
				InSettings->BackwardSplineTangentFromVerticalDelta);
		}
	}

	return ComputeStraightLineTangent(Start, End);
}

TSharedPtr<FWiggleWireSimulator> FJointGraphConnectionDrawingPolicy::GetOrAddSimulator(const FGraphWireId& Id)
{
	/*
	if(WireSimulators.Contains(Id))
	{
		return WireSimulators[Id];
	}
	
	TSharedPtr<FWiggleWireSimulator> SimulatorPtrRef = FWiggleWireSimulator::MakeInstance();
	
	if (!SimulatorPtrRef.IsValid())
	{
		SimulatorPtrRef = FWiggleWireSimulator::MakeInstance();

		WireSimulators.Add(Id, SimulatorPtrRef);

		UE_LOG(LogJointEditor,Log,TEXT("New Connection for %s"), *FString::FromInt(GetTypeHash(Id)));
	}
	
	return SimulatorPtrRef;
	*/

	TSharedPtr<FWiggleWireSimulator>& SimulatorPtrRef = WireSimulators.FindOrAdd(Id);

	if (!SimulatorPtrRef.IsValid())
	{
		SimulatorPtrRef = FWiggleWireSimulator::MakeInstance();
	}

	return SimulatorPtrRef;
}

void FJointGraphConnectionDrawingPolicy::PruneDeadSimulators(const float DeltaTime)
{
	TimeSinceLastPrune += DeltaTime;
	if (TimeSinceLastPrune < PRUNE_INTERVAL)
	{
		return; // Only prune periodically
	}
	TimeSinceLastPrune = 0.0f;

	TArray<FGraphWireId> SimulatorsToRemove;
	SimulatorsToRemove.Reserve(WireSimulators.Num()); // Avoid reallocations
	for (auto It = WireSimulators.CreateConstIterator(); It; ++It)
	{
		const FGraphWireId& WireId = It.Key();
		const UEdGraphPin* Pin1 = WireId.StartPin;
		const UEdGraphPin* Pin2 = WireId.EndPin;

		bool bConnectionIsValid = false;
		if (Pin1 && Pin2)
		{
			// Basic check: are the pins still linked?
			if (Pin1->LinkedTo.Contains(Pin2))
			{
				// @note: Maybe a stricter check here? Do pins still belong to *this* graph? Uncomment if needed
				// (In cases if pins/nodes could somehow be moved between graphs?)
				// UEdGraph* Pin1Graph = Pin1->GetOwningNodeUnchecked() ? Pin1->GetOwningNode()->GetGraph() : nullptr;
				// UEdGraph* Pin2Graph = Pin2->GetOwningNodeUnchecked() ? Pin2->GetOwningNode()->GetGraph() : nullptr;
				// if (Pin1Graph == GraphObj && Pin2Graph == GraphObj)
				// {
				bConnectionIsValid = true;
				// }
			}
		}

		if (!bConnectionIsValid)
		{
			SimulatorsToRemove.Add(WireId);
		}
	}

	// Remove the dead simulators from the map
	if (SimulatorsToRemove.Num() > 0)
	{
		for (const FGraphWireId& IdToRemove : SimulatorsToRemove)
		{
			WireSimulators.Remove(IdToRemove);
		}
		const int32 RemovedCount = SimulatorsToRemove.Num();
		UE_LOG(LogJointEditor, Log, TEXT("Pruned %d dead wire simulators."), RemovedCount);
	}
}

void FJointGraphConnectionDrawingPolicy::DrawWiggleConnection(int32 LayerId, const FVector2D& Start,
                                                              const FVector2D& End, const FConnectionParams& Params,
                                                              const FWiggleWireConfig& Config)
{
	const FGraphWireId WireId(Params.AssociatedPin1, Params.AssociatedPin2);
	TSharedPtr<FWiggleWireSimulator> Simulator = GetOrAddSimulator(WireId);
	if (!Simulator.IsValid())
	{
		UE_LOG(LogJointEditor, Error, TEXT("DrawWiggleConnection: Failed to get or add simulator for wire ID!"));

		// Fallback draw
		DrawTangentConnection(LayerId, Start, End, Params);

		return;
	}

	// Get DeltaTime
	const float DeltaTime = FSlateApplication::Get().GetDeltaTime();

	// Update the simulation state (but don't use the return value directly as a point!)
	Simulator->Update(Start, End, Config, DeltaTime);

	// Now get the actual visual center point
	const FVector2D CenterPoint = Simulator->GetVisualCenter(Start, End);

	// Calculate tangents based on the simulated center point and configured TangentFactor
	const FVector2D P0Tangent = (CenterPoint - Start) * Config.TangentFactor;
	const FVector2D P1Tangent = (End - CenterPoint) * Config.TangentFactor;

	// --- Hover Detection Logic (Adapted from MetaSound) ---
	// Uses the base thickness from Params, potentially multiplied by a wiggle-specific factor (placeholder for now)
	constexpr float WiggleWireThicknessMultiplier = 1.0f;
	const float WireThickness = Params.WireThickness * WiggleWireThicknessMultiplier;
	{
		const float QueryDistanceTriggerThresholdSquared = FMath::Square(
			Settings->SplineHoverTolerance + WireThickness * 0.5f);
		const float QueryDistanceForCloseSquared = FMath::Square(
			FMath::Sqrt(QueryDistanceTriggerThresholdSquared) + Settings->SplineCloseTolerance);

		// Broad phase check using a bounding box
		FBox2D Bounds(ForceInit);
		Bounds += Start;
		Bounds += Start + (P0Tangent * (1.f / 3.f)); // Approx control point
		Bounds += End;
		Bounds += End - (P1Tangent * (1.f / 3.f)); // Approx control point

		if (Bounds.ComputeSquaredDistanceToPoint(LocalMousePosition) < QueryDistanceForCloseSquared)
		{
			// @note Fine phase check: iterate along segments or use a proper Bezier distance function? 
			float ClosestDistanceSquared = FLT_MAX;
			constexpr int32 NumStepsToTest = 16;
			constexpr float StepInterval = 1.f / NumStepsToTest;
			FVector2D Point1 = FMath::CubicInterp(Start, P0Tangent, End, P1Tangent, 0.f);
			FVector2D ClosestOverallSplinePoint = Point1; // Keep track of the closest point itself

			for (float TestAlpha = KINDA_SMALL_NUMBER; TestAlpha <= 1.0f; TestAlpha += StepInterval)
			{
				const FVector2D Point2 = FMath::CubicInterp(Start, P0Tangent, End, P1Tangent, TestAlpha);
#if UE_VERSION_OLDER_THAN(5, 6, 0)
				const FVector2D ClosestPointOnSegment = FMath::ClosestPointOnSegment2D(
					LocalMousePosition, Point1, Point2);
#else
				const FVector2D ClosestPointOnSegment = FMath::ClosestPointOnSegment2D<double>(LocalMousePosition, Point1, Point2);
#endif
				const float DistanceSquared = (LocalMousePosition - ClosestPointOnSegment).SizeSquared();
				if (DistanceSquared < ClosestDistanceSquared)
				{
					ClosestDistanceSquared = DistanceSquared;
					ClosestOverallSplinePoint = ClosestPointOnSegment; // Update the closest point found
				}
				Point1 = Point2;
			}

			if (ClosestDistanceSquared < QueryDistanceTriggerThresholdSquared)
			{
				// If the closest point is close enough for selection/hover
				if (ClosestDistanceSquared < SplineOverlapResult.GetDistanceSquared())
				{
					// @note Calculate distance along spline? might not be needed

					// Calculate squared distances from the *actual closest point on the spline* to the pins
					const float SquaredDistToPin1 = Params.AssociatedPin1
						                                ? (Start - ClosestOverallSplinePoint).SizeSquared()
						                                : FLT_MAX;
					const float SquaredDistToPin2 = Params.AssociatedPin2
						                                ? (End - ClosestOverallSplinePoint).SizeSquared()
						                                : FLT_MAX;
					SplineOverlapResult = FGraphSplineOverlapResult(Params.AssociatedPin1, Params.AssociatedPin2,
					                                                ClosestDistanceSquared, SquaredDistToPin1,
					                                                SquaredDistToPin2, true);
				}
			}
			else if (ClosestDistanceSquared < QueryDistanceForCloseSquared)
			{
				// If close, but not quite close enough for selection, mark as close for potential visual feedback
				SplineOverlapResult.SetCloseToSpline(true);
			}
		}
	}
	// --- End Hover Detection ---

	// Draw the final spline using the calculated geometry
	FSlateDrawElement::MakeDrawSpaceSpline(DrawElementsList, LayerId, Start, P0Tangent, End, P1Tangent,
	                                       WireThickness, ESlateDrawEffect::NoPixelSnapping, Params.WireColor);

	// --- Bubble Drawing (Adapted from default) ---
	if (Params.bDrawBubbles && BubbleImage != nullptr) // Ensure BubbleImage is valid
	{
		const float ApplicationScale = FSlateApplication::Get().GetApplicationScale();
		const FVector2D BubbleSize = BubbleImage->ImageSize * ZoomFactor * ApplicationScale * 0.2f * Params.
			WireThickness;
		if (BubbleSize.X > KINDA_SMALL_NUMBER && BubbleSize.Y > KINDA_SMALL_NUMBER)
		{
			// Need to recalculate spline length and reparam table for the *current* spline shape
			FInterpCurve<float> SplineReparamTable;
			const float SplineLength = MakeSplineReparamTable(Start, P0Tangent, End, P1Tangent, SplineReparamTable);
			if (SplineLength > 1.0f) // Avoid division by zero or tiny lengths
			{
				const float BubbleSpacing = 64.f * ZoomFactor * ApplicationScale;
				const float BubbleSpeed = 192.f * ZoomFactor * ApplicationScale;

				// If GStartTime not accessible, maybe use FApp::GetCurrentTime() with modulo?
				const float Time = (FPlatformTime::Seconds() - GStartTime);
				const float BubbleOffset = FMath::Fmod(Time * BubbleSpeed, BubbleSpacing);
				const int32 NumBubbles = FMath::CeilToInt(SplineLength / BubbleSpacing);
				for (int32 i = 0; i < NumBubbles; ++i)
				{
					const float Distance = (static_cast<float>(i) * BubbleSpacing) + BubbleOffset;
					if (Distance < SplineLength)
					{
						const float Alpha = SplineReparamTable.Eval(Distance, 0.f);
						FVector2D BubblePos = FMath::CubicInterp(Start, P0Tangent, End, P1Tangent, Alpha);
						BubblePos -= (BubbleSize * 0.5f); // Center the bubble image

						FSlateDrawElement::MakeBox(
							DrawElementsList,
							LayerId + 1, // Draw bubbles slightly above the wire
							FPaintGeometry(BubblePos, BubbleSize, ApplicationScale * ZoomFactor),
							BubbleImage,
							ESlateDrawEffect::None,
							Params.WireColor);
					}
				}
			}
		}
	}
	// --- End Bubble Drawing ---
}

void FJointGraphConnectionDrawingPolicy::NotifyViewChanged() const
{
	// Iterate through all active simulators and activate them

	for (const TTuple<FGraphWireId, TSharedPtr<FWiggleWireSimulator>>& WireSimulator : WireSimulators)
	{
		if (!WireSimulator.Value.IsValid()) continue;

		WireSimulator.Value.Get()->Activate();
	}
}
