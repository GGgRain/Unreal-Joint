#pragma once

#include "CoreMinimal.h"
#include "Slate/WidgetRenderer.h"
#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"
#include "JointManagerThumbnailRenderer.generated.h"

class SJointGraphPreviewer;
class UJointManager;

// singleton cache for the thumbnail renderer
class FJointManagerThumbnailRendererResourcePool
{
public:
	
	FJointManagerThumbnailRendererResourcePool();
	~FJointManagerThumbnailRendererResourcePool();
	
public:
	
	/** 
	 * lock the graph previewer for the specified Joint manager.
	 * It will prevent any modification to the GraphPreviewerCache for the specified Joint manager.
	 */
	void LockFor(UJointManager* JointManager);
	void UnlockFor(UJointManager* JointManager);
	bool IsLockedFor(UJointManager* JointManager) const;
	
public:
	
	TSharedPtr<SJointGraphPreviewer> GetGraphPreviewerFor(UJointManager* JointManager);
	void SetGraphPreviewerFor(UJointManager* JointManager, TSharedPtr<SJointGraphPreviewer> GraphPreviewer);
	void ClearGraphPreviewerFor(UJointManager* JointManager);
	
public:
	
	void ClearCache();
	
private:
	
	TMap<TWeakObjectPtr<UJointManager>, TSharedPtr<SJointGraphPreviewer>> GraphPreviewerCache;
	
	TSet<TWeakObjectPtr<UJointManager>> LockedJointManagers;
	
public:
	
	static FJointManagerThumbnailRendererResourcePool& Get() { static FJointManagerThumbnailRendererResourcePool Instance; return Instance; }
};

UCLASS()
class UJointManagerThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
public:
	GENERATED_BODY()
public:
	
	UJointManagerThumbnailRenderer();
	~UJointManagerThumbnailRenderer();
	
	void ClearCache();
	
public:
	
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily) override;
	
	virtual bool CanVisualizeAsset(UObject* Object) override;
	
	virtual bool AllowsRealtimeThumbnails(UObject* Object) const override;

	// UObject implementation
	virtual void BeginDestroy() override;
	
private:
	
	/** Widget renderer used to render the graph preview for the thumbnail. */
	TUniquePtr<FWidgetRenderer> WidgetRenderer;
	
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> ThumbnailRenderTarget;
};
