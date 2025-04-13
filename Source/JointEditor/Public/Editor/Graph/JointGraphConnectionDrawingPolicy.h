//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConnectionDrawingPolicy.h"

class FSlateWindowElementList;
class UEdGraph;

namespace JointGraphConnectionDrawingPolicyConstants
{
	static const float StartFudgeX(-5.0f);
	static const float EndFudgeX(1.5f);
}

class JOINTEDITOR_API FJointGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
protected:
	
	UEdGraph* GraphObj;
	TMap<UEdGraphNode*, int32> NodeWidgetMap;

public:
	FJointGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	// FConnectionDrawingPolicy interface 
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
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


};
