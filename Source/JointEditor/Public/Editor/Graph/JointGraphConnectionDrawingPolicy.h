//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConnectionDrawingPolicy.h"
#include "JointWiggleWireSimulator.h"

class FSlateWindowElementList;
class UEdGraph;

namespace JointGraphConnectionDrawingPolicyConstants
{
	static const float StartFudgeX(-5.0f);
	static const float EndFudgeX(1.5f);
}

enum class EJointGraphConnectionDrawingConnectionType
{
	Normal,
	Recursive,
	Preview,
	Self,
};


class JOINTEDITOR_API FJointGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy, public TSharedFromThis<FJointGraphConnectionDrawingPolicy>
{
protected:
	
	UEdGraph* GraphObj;
	TMap<UEdGraphNode*, int32> NodeWidgetMap;

public:
	
	FJointGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);
	
	// FConnectionDrawingPolicy interface 
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawPinGeometries(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;
	virtual void DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params) override;
	virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params) override;
	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual void ApplyHoverDeemphasis(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ float& Thickness, /*inout*/ FLinearColor& WireColor) override;
	virtual FVector2D ComputeStraightLineTangent(const FVector2D& Start, const FVector2D& End) const;
	// End of FConnectionDrawingPolicy interface

protected:

	virtual FVector2D ComputeJointSplineTangent(const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) const;


	TMap<UEdGraphNode*, int32> NodeHierarchyMap;

protected:

	void Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params);
	
	void CalHierarchyMap();


	FORCEINLINE bool CheckIsRecursive(const UEdGraphNode* FromNode, const UEdGraphNode* ToNode) const;

	FORCEINLINE bool CheckIsSame(const UEdGraphNode* FromNode, const UEdGraphNode* ToNode) const;

	FORCEINLINE bool CheckIsInActiveRoute(const UEdGraphNode* Node) const;

protected:

	FORCEINLINE const EJointGraphConnectionDrawingConnectionType GetConnectionType(const UEdGraphPin* OutputPin, const UEdGraphPin* InputPin);
	FORCEINLINE const EJointGraphConnectionDrawingConnectionType GetConnectionType(const FConnectionParams& Params);

private:

	//Wiggle Wire Simulator

	/** Map storing the simulation state for each active wiggle wire connection. */
	TMap<FGraphWireId, TSharedPtr<FWiggleWireSimulator>>& WireSimulators;
	
	/** Helper function to get or create a simulator for a given wire ID. */
	TSharedPtr<FWiggleWireSimulator> GetOrAddSimulator(const FGraphWireId& Id); 
	
	/** Time since the last prune check for dead simulators. */
	float TimeSinceLastPrune;

	/** How often (in seconds) to check for and remove dead simulators. */
	static constexpr float PRUNE_INTERVAL = 5.0f;

	/** Prunes simulators associated with connections that no longer exist. */
	void PruneDeadSimulators(const float DeltaTime);
	
	/**
	 * Notifies the drawing policy that the graph view has changed (e.g., zoom, pan).
	 * This should trigger activation of relevant wire simulations.
	 */
	void NotifyViewChanged() const;
	
protected:
	
	virtual void DrawWiggleConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params, const FWiggleWireConfig& Config);
	virtual void DrawTangentConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params);
	
	const bool& bUseWiggleWireForNormalConnection;
	const bool& bUseWiggleWireForRecursiveConnection;
	const bool& bUseWiggleWireForSelfConnection;
	const bool& bUseWiggleWireForPreviewConnection;

	const FWiggleWireConfig& NormalConnectionWiggleWireConfig;
	const FWiggleWireConfig& RecursiveConnectionWiggleWireConfig;
	const FWiggleWireConfig& SelfConnectionWiggleWireConfig;
	const FWiggleWireConfig& PreviewConnectionWiggleWireConfig;
	
};
