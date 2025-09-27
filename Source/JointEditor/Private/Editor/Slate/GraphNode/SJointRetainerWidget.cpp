#include "GraphNode/SJointRetainerWidget.h"

#include "JointEditorStyle.h"
#include "SGraphPanel.h"
#include "Async/Async.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GraphNode/SJointGraphNodeBase.h"
#include "Slate/WidgetRenderer.h"


SJointRetainerWidget::SJointRetainerWidget()
	: RenderingResources(new FJointRetainerWidgetRenderingResources)
{
	// use custom prepass to ensure the widget is ready for rendering
	bHasCustomPrepass = true;
}

SJointRetainerWidget::~SJointRetainerWidget()
{
	// Begin deferred cleanup of rendering resources.  DO NOT delete here.  Will be deleted when safe

	BeginCleanup(RenderingResources);
}

SCompoundWidget::FCompoundWidgetOneChildSlot& SJointRetainerWidget::GetChildSlot()
{
	return this->ChildSlot;
}

void SJointRetainerWidget::Construct(const FArguments& InArgs)
{
	
	Phase = InArgs._Phase;
	PhaseCount = FMath::Rand() % Phase;
	bForceRecaptureCachedSizeWhenCaptureRetainerRenderingIsFalse = InArgs._bForceRecaptureCachedSizeWhenCaptureRetainerRenderingIsFalse;
	DisplayRetainerRendering = InArgs._DisplayRetainerRendering;
	CaptureRetainerRendering = InArgs._CaptureRetainerRendering;

	SetVisibility(EVisibility::SelfHitTestInvisible);

	this->ChildSlot
	[
		InArgs._ChildSlot.Widget
	];
}


void SJointRetainerWidget::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	RegisterActiveTimer(UnhoveredCheckingInterval,
	                    FWidgetActiveTimerDelegate::CreateSP(this, &SJointRetainerWidget::CheckUnhovered));

	SetForceRedrawPerFrame(true);

	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
}


EActiveTimerReturnType SJointRetainerWidget::CheckUnhovered(double InCurrentTime, float InDeltaTime)
{
	//If it's still hovered, Continue.
	if (IsHovered())
	{
		return EActiveTimerReturnType::Continue;
	}

	SetForceRedrawPerFrame(false);

	return EActiveTimerReturnType::Stop;
}


bool SJointRetainerWidget::CalculatePhaseAndCheckItsTime()
{
	return ++PhaseCount >= Phase || bForceRedrawPerFrame;
}

bool SJointRetainerWidget::CustomPrepass(float LayoutScaleMultiplier)
{
	// If we are not in low detailed rendering mode, skip the prepass
	if (CaptureRetainerRendering.Get())
	{
		TSharedPtr<SWidget> ChildWidget = this->ChildSlot.GetWidget();
	
		if (!ChildWidget) return false;
	
		// Allocate or update the render target if needed
		FIntPoint NewDesiredSize = ChildWidget->GetDesiredSize().IntPoint();

		NewDesiredSize.X = (NewDesiredSize.X == 0) ? 1 : NewDesiredSize.X;
		NewDesiredSize.Y = (NewDesiredSize.Y == 0) ? 1 : NewDesiredSize.Y;

		if (RenderingResources && (!RenderingResources->RenderTarget || CachedSize != NewDesiredSize))
		{
			if (RenderingResources->RenderTarget)
			{
				RenderingResources->RenderTarget->RemoveFromRoot();
				RenderingResources->RenderTarget = nullptr;
			}

			if (NewDesiredSize.X > 0 && NewDesiredSize.Y > 0)
			{
				UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();

				const bool bWriteContentInGammaSpace = true;

				RenderingResources->RenderTarget = RenderTarget;
				RenderTarget->AddToRoot();
				RenderTarget->InitAutoFormat(NewDesiredSize.X, NewDesiredSize.Y);
				RenderTarget->ClearColor = FJointEditorStyle::Color_Hover;
				RenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
				RenderTarget->bAutoGenerateMips = false;
				RenderTarget->bCanCreateUAV = true;

				RenderTarget->TargetGamma = !bWriteContentInGammaSpace ? 0.0f : 1.0;
				RenderTarget->SRGB = !bWriteContentInGammaSpace;

				RenderTarget->UpdateResource();


				bNeedsRedraw = true;

				SurfaceBrush.SetResourceObject(RenderTarget);
				SurfaceBrush.ImageSize = NewDesiredSize;

				CachedSize = NewDesiredSize;
			}
		}

		bNeedsRedraw |= CalculatePhaseAndCheckItsTime();


		if (RenderingResources && !RenderingResources->WidgetRenderer)
		{
			// We can't write out linear.  If we write out linear, then we end up with premultiplied alpha
			// in linear space, which blending with gamma space later is difficult...impossible? to get right
			// since the rest of slate does blending in gamma space.
			const bool bWriteContentInGammaSpace = true;

			FWidgetRenderer* WidgetRenderer = new FWidgetRenderer(bWriteContentInGammaSpace);

			RenderingResources->WidgetRenderer = WidgetRenderer;
			WidgetRenderer->SetUseGammaCorrection(bWriteContentInGammaSpace);
			// This will be handled by the main slate rendering pass
			WidgetRenderer->SetApplyColorDeficiencyCorrection(false);

			WidgetRenderer->SetIsPrepassNeeded(false);
			WidgetRenderer->SetClearHitTestGrid(false);
			WidgetRenderer->SetShouldClearTarget(false);
		}


		const FIntPoint NULLSIZE = FIntPoint(1, 1);

		if (CachedSize == NULLSIZE)
		{
			bNeedsRedraw = true;
		}

		bool bRedrawed = false;

		// Mark for redraw if needed
		if (bNeedsRedraw && ChildWidget.IsValid() && RenderingResources && RenderingResources->RenderTarget)
		{
			RenderingResources->RenderTarget->UpdateResource();

			RenderingResources->WidgetRenderer->DrawWidget(
				RenderingResources->RenderTarget,
				ChildWidget.ToSharedRef(),
				CachedSize,
				0.f, // DeltaTime
				false
			);
			bNeedsRedraw = false;
			bRedrawed = true;

			// Reset the phase counter
			PhaseCount = 0;
		}

		return bRedrawed; // Continue prepass for children
	}else
	{
		if (bForceRecaptureCachedSizeWhenCaptureRetainerRenderingIsFalse)
		{
			TSharedPtr<SWidget> ChildWidget = this->ChildSlot.GetWidget();
	
			if (!ChildWidget) return false;
	
			// Allocate or update the render target if needed
			FIntPoint NewDesiredSize = ChildWidget->GetDesiredSize().IntPoint();

			NewDesiredSize.X = (NewDesiredSize.X == 0) ? 1 : NewDesiredSize.X;
			NewDesiredSize.Y = (NewDesiredSize.Y == 0) ? 1 : NewDesiredSize.Y;

			if (RenderingResources && (!RenderingResources->RenderTarget || CachedSize != NewDesiredSize))
			{
				if (NewDesiredSize.X > 0 && NewDesiredSize.Y > 0)
				{
					CachedSize = NewDesiredSize;
				}
			}
		}
		return true; // Continue prepass for children
	}

	return true;
}

int32 SJointRetainerWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (DisplayRetainerRendering.Get())
	{
		if (RenderingResources)
		{
			UTextureRenderTarget2D* RenderTarget = RenderingResources->RenderTarget;

			// Draw the render target as an image
			if (RenderTarget && RenderTarget->GetSurfaceWidth() >= 1 && RenderTarget->GetSurfaceHeight() >= 1)
			{
				RenderTarget->UpdateResourceImmediate(false);

				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId,
					AllottedGeometry.ToPaintGeometry(),
					&SurfaceBrush,
					ESlateDrawEffect::None | ESlateDrawEffect::NoGamma
				);
			}
		}

		return LayerId + 1;
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                                bParentEnabled);
}

void SJointRetainerWidget::SetForceRedrawPerFrame(const bool bInForceRedraw)
{
	bForceRedrawPerFrame = bInForceRedraw;
}
