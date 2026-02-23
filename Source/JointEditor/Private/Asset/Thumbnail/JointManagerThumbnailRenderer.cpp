
#include "Thumbnail/JointManagerThumbnailRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "JointManager.h"
#include "ObjectTools.h"
#include "EditorWidget/SJointGraphPanel.h"
#include "EditorWidget/SJointGraphPreviewer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Widgets/Images/SThrobber.h"

//FGCObject
FJointManagerThumbnailRendererResourcePoolElement::~FJointManagerThumbnailRendererResourcePoolElement()
{
}

FJointManagerThumbnailRendererResourcePoolElement::FJointManagerThumbnailRendererResourcePoolElement(UJointManager* InJointManager): JointManager(InJointManager)
{
}

void FJointManagerThumbnailRendererResourcePoolElement::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (ThumbnailRenderTarget)
	{
		Collector.AddReferencedObject(ThumbnailRenderTarget);
	}
}

FString FJointManagerThumbnailRendererResourcePoolElement::GetReferencerName() const
{
	return TEXT("FJointManagerThumbnailRendererResourcePoolElement");
}

void FJointManagerThumbnailRendererResourcePoolElement::Lock()
{
	bIsLocked = true;
}

void FJointManagerThumbnailRendererResourcePoolElement::Unlock()
{
	bIsLocked = false;
}

bool FJointManagerThumbnailRendererResourcePoolElement::IsLocked() const
{
	return bIsLocked;
}

void FJointManagerThumbnailRendererResourcePoolElement::CreateRenderTargetIfNeeded()
{
	if (ThumbnailRenderTarget) return;
	
	ThumbnailRenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	ThumbnailRenderTarget->InitAutoFormat(512, 512);
	ThumbnailRenderTarget->ClearColor = FLinearColor::Transparent;
	ThumbnailRenderTarget->UpdateResourceImmediate();
}

UTextureRenderTarget2D* FJointManagerThumbnailRendererResourcePoolElement::GetRenderTarget() const
{
	return ThumbnailRenderTarget;
}

void FJointManagerThumbnailRendererResourcePoolElement::SetRenderTarget(UTextureRenderTarget2D* RenderTarget)
{
	ThumbnailRenderTarget = RenderTarget;
}

void FJointManagerThumbnailRendererResourcePoolElement::SetGraphPreviewer(TSharedPtr<SJointGraphPreviewer> InGraphPreviewer)
{
	this->GraphPreviewer = InGraphPreviewer;
}

TSharedPtr<SJointGraphPreviewer> FJointManagerThumbnailRendererResourcePoolElement::GetGraphPreviewer() const
{
	return GraphPreviewer;
}

void FJointManagerThumbnailRendererResourcePoolElement::ClearGraphPreviewer()
{
	GraphPreviewer.Reset();
	ResetGraphPreviewerUpdate();
}

void FJointManagerThumbnailRendererResourcePoolElement::RequestUpdateGraphPreviewer()
{
	GraphUpdateRemainingFrames = MaxGraphUpdateFrames;
}

bool FJointManagerThumbnailRendererResourcePoolElement::ShouldUpdateGraphPreviewer() const
{
	return GraphUpdateRemainingFrames >= 0;
}

bool FJointManagerThumbnailRendererResourcePoolElement::IsUpdateJustFinished() const
{
	return GraphUpdateRemainingFrames == 0;
}

void FJointManagerThumbnailRendererResourcePoolElement::TickGraphPreviewerUpdate()
{
	if (ShouldUpdateGraphPreviewer())
	{
		--GraphUpdateRemainingFrames;
	}
}

void FJointManagerThumbnailRendererResourcePoolElement::ResetGraphPreviewerUpdate()
{
	GraphUpdateRemainingFrames = -1;
}




FJointManagerThumbnailRendererResourceManager::FJointManagerThumbnailRendererResourceManager()
{
	WidgetRenderer = MakeUnique<FWidgetRenderer>();
	WidgetRenderer->SetIsPrepassNeeded(true);
	WidgetRenderer->SetUseGammaCorrection(false);
	WidgetRenderer->SetShouldClearTarget(false);
	FCoreDelegates::OnEnginePreExit.AddRaw(this, &FJointManagerThumbnailRendererResourceManager::ClearCache);
}

FJointManagerThumbnailRendererResourceManager::~FJointManagerThumbnailRendererResourceManager()
{
	ClearCache();
}

void FJointManagerThumbnailRendererResourceManager::ClearCache()
{
	ResourcePool.Reset();
}

TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement> FJointManagerThumbnailRendererResourceManager::FindOrAddResourcePoolElementFor(UJointManager* JointManager)
{
	const uint32 Hash = GetTypeHash(JointManager);
	
	if (TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement>* FoundElement = ResourcePool.FindByHash(Hash, JointManager))
	{
		return *FoundElement;
	}
	
	// Create a new resource pool element for this Joint Manager and add it to the pool.
	const TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement> NewElement = MakeShared<FJointManagerThumbnailRendererResourcePoolElement>(JointManager);
	
	ResourcePool.Add( NewElement );
	
	return NewElement;
}

TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement> FJointManagerThumbnailRendererResourceManager::GetResourcePoolElementFor(UJointManager* JointManager)
{	
	const uint32 Hash = GetTypeHash(JointManager);
	
	if (TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement>* FoundElement = ResourcePool.FindByHash(Hash, JointManager))
	{
		return *FoundElement;
	}
	
	return nullptr;
}

void FJointManagerThumbnailRendererResourceManager::RemoveResourcePoolElementFor(UJointManager* JointManager)
{
	const uint32 Hash = GetTypeHash(JointManager);
	
	ResourcePool.RemoveByHash(Hash, JointManager);
}