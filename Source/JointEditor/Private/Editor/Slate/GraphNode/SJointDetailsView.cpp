#include "Editor/Slate/GraphNode/SJointDetailsView.h"

#include "JointEdGraphNodesCustomization.h"
#include "JointEditorSettings.h"
#include "JointEditorStyle.h"
#include "SGraphPanel.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GraphNode/SJointGraphNodeBase.h"
#include "Slate/WidgetRenderer.h"


class FJointRetainerWidgetRenderingResources : public FDeferredCleanupInterface, public FGCObject
{
public:
	FJointRetainerWidgetRenderingResources()
		: WidgetRenderer(nullptr)
		  , RenderTarget(nullptr)
	{
	}

	~FJointRetainerWidgetRenderingResources()
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


SJointDetailsView::SJointDetailsView()
	: Object(nullptr),
	  EdNode(nullptr),
	  RenderingResources(new FJointRetainerWidgetRenderingResources)
{
	// use custom prepass to ensure the widget is ready for rendering
	bHasCustomPrepass = true;
}

SJointDetailsView::~SJointDetailsView()
{
	// Begin deferred cleanup of rendering resources.  DO NOT delete here.  Will be deleted when safe
	BeginCleanup(RenderingResources);

	OwnerGraphNode.Reset();
	JointDetailViewRetainerWidget.Reset();
	DetailViewWidget.Reset();

	this->ChildSlot.DetachWidget();

	Object = nullptr;
	EdNode = nullptr;

	PropertyData.Empty();
	PropertiesToShow.Empty();
	
}

void SJointDetailsView::Construct(const FArguments& InArgs)
{
	this->Object = InArgs._Object;
	this->PropertyData = InArgs._PropertyData;
	this->EdNode = InArgs._EditorNodeObject;
	this->OwnerGraphNode = InArgs._OwnerGraphNode;

	if (!Object) return;
	if (this->PropertyData.IsEmpty()) return;
	if (!EdNode || !EdNode->GetCastedGraph()) return;

	InitializationDelay = static_cast<float>(EdNode->GetCastedGraph()->GetCachedJointGraphNodes().Num()) / static_cast<float>(UJointEditorSettings::Get()->SimplePropertyDisplayInitializationDelayStandard);
	Phase = UJointEditorSettings::Get()->LODRenderingForSimplePropertyDisplayRetainerPeriod;
	PhaseCount = FMath::Rand() % Phase;
	bUseLOD = UJointEditorSettings::Get()->bUseLODRenderingForSimplePropertyDisplay;
	
	SetVisibility(EVisibility::SelfHitTestInvisible);
	CachePropertiesToShow();
	PopulateSlate();
}

void SJointDetailsView::PopulateSlate()
{
	this->ChildSlot.DetachWidget();

	bInitialized = false;

	const float DelaySeconds = FMath::FRand() * InitializationDelay; // Set your delay

	RegisterActiveTimer(DelaySeconds,FWidgetActiveTimerDelegate::CreateSP(this, &SJointDetailsView::InitializationTimer));
}

EActiveTimerReturnType SJointDetailsView::InitializationTimer(double InCurrentTime, float InDeltaTime)
{
	AsyncTask(ENamedThreads::GameThread, [this, Object = this->Object]()
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(
				"PropertyEditor");

			FDetailsViewArgs DetailsViewArgs;
			DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
			DetailsViewArgs.bHideSelectionTip = true;
			DetailsViewArgs.bAllowSearch = false;
			//DetailsViewArgs.ViewIdentifier = FName(FGuid::NewGuid().ToString());
		
			DetailViewWidget = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

			DetailViewWidget->SetIsCustomRowVisibleDelegate(
				FIsCustomRowVisible::CreateSP(this, &SJointDetailsView::GetIsRowVisible));
			DetailViewWidget->SetIsPropertyVisibleDelegate(
				FIsPropertyVisible::CreateSP(this, &SJointDetailsView::GetIsPropertyVisible));

			//For the better control over expanding.
			DetailViewWidget->SetGenericLayoutDetailsDelegate(
				FOnGetDetailCustomizationInstance::CreateStatic(
					&FJointNodeInstanceSimpleDisplayCustomizationBase::MakeInstance));

			DetailViewWidget->SetObject(Object, false);
		
			this->ChildSlot.AttachWidget(DetailViewWidget.ToSharedRef());

			bInitialized = true;
		});

	return EActiveTimerReturnType::Stop;
}


bool SJointDetailsView::GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	if (PropertyData.IsEmpty()) return false;

	//Grab the parent-most one or itself.
	const FProperty* Property = PropertyAndParent.ParentProperties.Num() > 0
		                            ? PropertyAndParent.ParentProperties.Last()
		                            : &PropertyAndParent.Property;

	// Check against the parent property name
	return PropertiesToShow.Contains(Property->GetFName());
}

bool SJointDetailsView::GetIsRowVisible(FName InRowName, FName InParentName) const
{
	if (PropertyData.IsEmpty()) return false;

	//Watch out for the later usage. I didn't have time for this...
	if (InParentName == "Description") return false;

	return true;
}

void SJointDetailsView::CachePropertiesToShow()
{
	if (PropertyData.IsEmpty()) return;

	PropertiesToShow.Empty();
	PropertiesToShow.Reserve(PropertyData.Num());
	for (const FJointGraphNodePropertyData& Data : PropertyData)
	{
		PropertiesToShow.Emplace(Data.PropertyName);
	}
}


void SJointDetailsView::UpdatePropertyData(const TArray<FJointGraphNodePropertyData>& InPropertyData)
{
	PropertyData = InPropertyData;
}

void SJointDetailsView::SetOwnerGraphNode(TSharedPtr<SJointGraphNodeBase> InOwnerGraphNode)
{
	OwnerGraphNode = InOwnerGraphNode;
}

void SJointDetailsView::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	RegisterActiveTimer(UnhoveredCheckingInterval,
	                    FWidgetActiveTimerDelegate::CreateSP(this, &SJointDetailsView::CheckUnhovered));

	SetForceRedrawPerFrame(true);

	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
}


EActiveTimerReturnType SJointDetailsView::CheckUnhovered(double InCurrentTime, float InDeltaTime)
{
	//If it's still hovered, Continue.
	if (IsHovered())
	{
		return EActiveTimerReturnType::Continue;
	}

	SetForceRedrawPerFrame(false);

	return EActiveTimerReturnType::Stop;
}


bool SJointDetailsView::CalculatePhaseAndCheckItsTime()
{
	return ++PhaseCount >= Phase || bForceRedrawPerFrame;
}

bool SJointDetailsView::CustomPrepass(float LayoutScaleMultiplier)
{
	if (!bInitialized) return false;

	// If we are not in low detailed rendering mode, skip the prepass
	if (!UseLowDetailedRendering()) return true;

	// Allocate or update the render target if needed
	FIntPoint NewDesiredSize = DetailViewWidget->GetDesiredSize().IntPoint();

	NewDesiredSize.X = (NewDesiredSize.X == 0) ? 1 : NewDesiredSize.X;
	NewDesiredSize.Y = (NewDesiredSize.Y == 0) ? 1 : NewDesiredSize.Y;

	if (!RenderingResources->RenderTarget || CachedSize != NewDesiredSize)
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


	if (!RenderingResources->WidgetRenderer)
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
	if (bNeedsRedraw && DetailViewWidget.IsValid() && RenderingResources->RenderTarget)
	{
		RenderingResources->RenderTarget->UpdateResource();

		RenderingResources->WidgetRenderer->DrawWidget(
			RenderingResources->RenderTarget,
			DetailViewWidget.ToSharedRef(),
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
}

int32 SJointDetailsView::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!bInitialized) return LayerId;

	if (UseLowDetailedRendering())
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

		return LayerId + 1;
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                                bParentEnabled);
}

void SJointDetailsView::SetForceRedrawPerFrame(const bool bInForceRedraw)
{
	bForceRedrawPerFrame = bInForceRedraw;
}

bool SJointDetailsView::UseLowDetailedRendering() const
{
	if (!bUseLOD) return false;
	
	if (OwnerGraphNode.IsValid())
	{
		if (const TSharedPtr<SGraphPanel>& MyOwnerPanel = OwnerGraphNode.Pin()->GetOwnerPanel())
		{
			if (MyOwnerPanel.IsValid())
			{
				return MyOwnerPanel->GetCurrentLOD() <= EGraphRenderingLOD::LowDetail;
			}
		}
	}

	return false;
}
