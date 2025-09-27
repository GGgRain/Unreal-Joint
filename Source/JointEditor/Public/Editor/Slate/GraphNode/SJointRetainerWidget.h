#pragma once


#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"

class FJointRetainerWidgetRenderingResources : public FDeferredCleanupInterface, public FGCObject
{
public:
	FJointRetainerWidgetRenderingResources()
		: WidgetRenderer(nullptr)
		  , RenderTarget(nullptr)
	{
	}

	virtual ~FJointRetainerWidgetRenderingResources() override
	{
		// Note not using deferred cleanup for widget renderer here as it is already in deferred cleanup
		if (WidgetRenderer)
		{
			delete WidgetRenderer;
		}
	}

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(RenderTarget);
	}

	virtual FString GetReferencerName() const override
	{
		return TEXT("FRetainerWidgetRenderingResources");
	}

public:
	FWidgetRenderer* WidgetRenderer;
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;
};

class JOINTEDITOR_API SJointRetainerWidget : public SCompoundWidget
{

public:

	SLATE_BEGIN_ARGS(SJointRetainerWidget) :
		_Phase(5),
		_bForceRecaptureCachedSizeWhenCaptureRetainerRenderingIsFalse(true),
		_DisplayRetainerRendering(true),
		_CaptureRetainerRendering(true)
		{}
		SLATE_ARGUMENT(float, Phase)
		SLATE_ARGUMENT(bool, bForceRecaptureCachedSizeWhenCaptureRetainerRenderingIsFalse)
		// Whether to display the rendering of the child widget. If false, it will draw the child widget directly.
		SLATE_ATTRIBUTE(bool, DisplayRetainerRendering)
		// Whether to capture the rendering of the child widget. If false, it will not capture the rendering.
		SLATE_ATTRIBUTE(bool, CaptureRetainerRendering) 
		//slot
		SLATE_DEFAULT_SLOT(FArguments, ChildSlot)
	SLATE_END_ARGS()
	
public:

	void Construct(const FArguments& InArgs);

	SJointRetainerWidget();
	~SJointRetainerWidget();

public:

	//Get the child slot for the external widget
	FORCEINLINE SCompoundWidget::FCompoundWidgetOneChildSlot& GetChildSlot();

public:
	
	bool CalculatePhaseAndCheckItsTime();
	
	virtual bool CustomPrepass(float LayoutScaleMultiplier) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

public:

	void SetForceRedrawPerFrame(bool bInForceRedraw);
	
private:

	FJointRetainerWidgetRenderingResources* RenderingResources;
	
	FIntPoint CachedSize = FIntPoint::ZeroValue;
	
	mutable bool bNeedsRedraw = true;

	mutable FSlateBrush SurfaceBrush;

private:

	int32 Phase = 0;

	int32 PhaseCount = 0;

private:

	bool bForceRedrawPerFrame = false;

	bool bForceRecaptureCachedSizeWhenCaptureRetainerRenderingIsFalse = true;


public:
	
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
	
	EActiveTimerReturnType CheckUnhovered(double InCurrentTime, float InDeltaTime);

private:

	float UnhoveredCheckingInterval = 0.2f;

public:
	
	TAttribute<bool> DisplayRetainerRendering;
	TAttribute<bool> CaptureRetainerRendering;

};
