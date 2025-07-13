#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphNode.h"
#include "Node/JointNodeBase.h"
#include "Widgets/SCompoundWidget.h"


class FJointRetainerWidgetRenderingResources;

class JOINTEDITOR_API SJointDetailsView : public SCompoundWidget
{

public:

	SLATE_BEGIN_ARGS(SJointDetailsView) :
		_OwnerGraphNode(nullptr),
		_Object(nullptr),
		_EditorNodeObject(nullptr)
	{} 
	SLATE_ARGUMENT(TSharedPtr<SJointGraphNodeBase>, OwnerGraphNode)
	SLATE_ARGUMENT(UJointNodeBase*, Object)
	SLATE_ARGUMENT(UJointEdGraphNode*, EditorNodeObject)
	SLATE_ARGUMENT(TArray<FJointGraphNodePropertyData>, PropertyData)

	SLATE_END_ARGS()
	
public:

	void Construct(const FArguments& InArgs);

	SJointDetailsView();

	~SJointDetailsView();

public:

	void PopulateSlate();

public:
	
	bool GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	
	bool GetIsRowVisible(FName InRowName, FName InParentName) const;

public:
	
	void CachePropertiesToShow();

public:
	
	UJointNodeBase* Object;
	UJointEdGraphNode* EdNode;
	TArray<FJointGraphNodePropertyData> PropertyData;

public:

	void UpdatePropertyData(const TArray<FJointGraphNodePropertyData>& PropertyData);
	
	void SetOwnerGraphNode(TSharedPtr<SJointGraphNodeBase> InOwnerGraphNode);

private:
	
	TArray<FName> PropertiesToShow;

public:

	TSharedPtr<SRetainerWidget> JointDetailViewRetainerWidget;

public:
	
	TWeakPtr<SJointGraphNodeBase> OwnerGraphNode;

public:
	
	bool CalculatePhaseAndCheckItsTime();
	
	virtual bool CustomPrepass(float LayoutScaleMultiplier) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

public:

	void SetForceRedrawPerFrame(bool bInForceRedraw);
	
private:
	
	TSharedPtr<IDetailsView> DetailViewWidget;

	FJointRetainerWidgetRenderingResources* RenderingResources;
	
	FIntPoint CachedSize = FIntPoint::ZeroValue;
	
	mutable bool bNeedsRedraw = true;

	mutable FSlateBrush SurfaceBrush;

private:

	int32 Phase = 0;

	int32 PhaseCount = 0;

	float InitializationDelay = 5;

private:

	bool bForceRedrawPerFrame = false;

public:

	//LOD
	bool UseLowDetailedRendering() const;

public:
	
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:

	EActiveTimerReturnType InitializationTimer(double InCurrentTime, float InDeltaTime);
	
	EActiveTimerReturnType CheckUnhovered(double InCurrentTime, float InDeltaTime);

private:

	float UnhoveredCheckingInterval = 0.2f;

private:

	bool bInitialized = false;

	bool bUseLOD = true;

};
