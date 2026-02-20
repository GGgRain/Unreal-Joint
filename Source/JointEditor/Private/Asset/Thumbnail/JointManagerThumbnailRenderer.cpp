
#include "Thumbnail/JointManagerThumbnailRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "JointEdUtils.h"
#include "JointManager.h"
#include "ObjectTools.h"
#include "EditorWidget/SJointGraphPanel.h"
#include "EditorWidget/SJointGraphPreviewer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"

FJointManagerThumbnailRendererResourcePool::FJointManagerThumbnailRendererResourcePool()
{
	FCoreDelegates::OnEnginePreExit.AddRaw(this, &FJointManagerThumbnailRendererResourcePool::ClearCache);
}

FJointManagerThumbnailRendererResourcePool::~FJointManagerThumbnailRendererResourcePool()
{
	ClearCache();
}

void FJointManagerThumbnailRendererResourcePool::LockFor(UJointManager* JointManager)
{
	LockedJointManagers.Add(JointManager);
}

void FJointManagerThumbnailRendererResourcePool::UnlockFor(UJointManager* JointManager)
{
	LockedJointManagers.Remove(JointManager);
}

bool FJointManagerThumbnailRendererResourcePool::IsLockedFor(UJointManager* JointManager) const
{
	return LockedJointManagers.Contains(JointManager);
}

TSharedPtr<SJointGraphPreviewer> FJointManagerThumbnailRendererResourcePool::GetGraphPreviewerFor(UJointManager* JointManager)
{
	if (GraphPreviewerCache.Contains(JointManager))
	{
		return GraphPreviewerCache[JointManager];
	}
	
	return nullptr;
}

void FJointManagerThumbnailRendererResourcePool::SetGraphPreviewerFor(UJointManager* JointManager, TSharedPtr<SJointGraphPreviewer> GraphPreviewer)
{
	if (!IsLockedFor(JointManager))
	{
		GraphPreviewerCache.Add(JointManager, GraphPreviewer);
	}
}

void FJointManagerThumbnailRendererResourcePool::ClearGraphPreviewerFor(UJointManager* JointManager)
{
	if (!IsLockedFor(JointManager))
	{
		GraphPreviewerCache.Remove(JointManager);
	}
}

void FJointManagerThumbnailRendererResourcePool::ClearCache()
{
	GraphPreviewerCache.Empty();
	LockedJointManagers.Empty();
}

UJointManagerThumbnailRenderer::UJointManagerThumbnailRenderer()
{
	// Set the default thumbnail size for Joint Manager assets.
	DefaultSizeX = 512;
	DefaultSizeY = 512;
	
	WidgetRenderer = MakeUnique<FWidgetRenderer>();
	WidgetRenderer->SetIsPrepassNeeded(true);
	WidgetRenderer->SetUseGammaCorrection(false);
	
	ThumbnailRenderTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("ThumbnailRenderTarget"));
	ThumbnailRenderTarget->InitAutoFormat(DefaultSizeX, DefaultSizeY);
	ThumbnailRenderTarget->UpdateResourceImmediate();
	
	// bind event on engine close to clear the cache, to avoid keeping unnecessary references to the Joint manager assets and their graphs.
	FCoreDelegates::OnPreExit.AddUObject(this, &UJointManagerThumbnailRenderer::ClearCache);
}

UJointManagerThumbnailRenderer::~UJointManagerThumbnailRenderer()
{
	ClearCache();
}

void UJointManagerThumbnailRenderer::ClearCache()
{
	WidgetRenderer.Reset();
	ThumbnailRenderTarget = nullptr;
	FJointManagerThumbnailRendererResourcePool::Get().ClearCache();
}

void UJointManagerThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	// 1. 기본 썸네일 먼저 그리기
	//Super::Draw(Object, X, Y, Width, Height, Viewport, Canvas, bAdditionalViewFamily);
	
	if (Object == nullptr || !Object->IsA<UJointManager>()) return;

	UJointManager* JointManager = Cast<UJointManager>(Object);
	
	if (JointManager->JointGraph)
	{
		TSharedPtr<SJointGraphPreviewer> GraphPreviewer = FJointManagerThumbnailRendererResourcePool::Get().GetGraphPreviewerFor(JointManager);
		
		if (!GraphPreviewer)
		{
			if (!FJointManagerThumbnailRendererResourcePool::Get().IsLockedFor(JointManager))
			{
				GraphPreviewer = SNew(SJointGraphPreviewer, JointManager->GetJointGraph())
					.Visibility(EVisibility::SelfHitTestInvisible);
				FJointManagerThumbnailRendererResourcePool::Get().SetGraphPreviewerFor(JointManager, GraphPreviewer);
			}
		}
		
		if (!GraphPreviewer) return;
		
		WidgetRenderer->DrawWidget(
			ThumbnailRenderTarget,
			GraphPreviewer.ToSharedRef(),
			FVector2D(DefaultSizeX, DefaultSizeY),
			GetTime().GetDeltaRealTimeSeconds()
		);
		
		FCanvasTileItem TileItem(
			FVector2D(X, Y),
			ThumbnailRenderTarget->GetResource(),
			FVector2D(Width, Height),
			FLinearColor::White
		);
		
		Canvas->DrawItem(TileItem);
		
		return;
	}
	
}

bool UJointManagerThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	return true;
}

bool UJointManagerThumbnailRenderer::AllowsRealtimeThumbnails(UObject* Object) const
{
	if (Object == nullptr || !Object->IsA<UJointManager>()) return false;

	UJointManager* JointManager = Cast<UJointManager>(Object);
	
	// if there is an active toolkit for this JM, skip rendering the thumbnail here.
	FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(JointManager, false, false);
		
	if (Toolkit) return false;
	
	return true;
}

void UJointManagerThumbnailRenderer::BeginDestroy()
{
	ClearCache();
	
	Super::BeginDestroy();
}
