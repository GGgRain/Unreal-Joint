#pragma once

#include "CoreMinimal.h"
#include "Slate/WidgetRenderer.h"
#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"

class SJointGraphPreviewer;
class UJointManager;

class FJointManagerThumbnailRendererResourcePoolElement 
	: public TSharedFromThis<FJointManagerThumbnailRendererResourcePoolElement>
	, public FGCObject
{
public:
	
	FJointManagerThumbnailRendererResourcePoolElement() = default;
	virtual ~FJointManagerThumbnailRendererResourcePoolElement() override;

	FJointManagerThumbnailRendererResourcePoolElement(UJointManager* InJointManager);

public:
	
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

public:
	
	void Lock();
	void Unlock();
	bool IsLocked() const;
	
public:
	
	void CreateRenderTargetIfNeeded();
	UTextureRenderTarget2D* GetRenderTarget() const;
	void SetRenderTarget(UTextureRenderTarget2D* RenderTarget);
	
public:
	
	void SetGraphPreviewer(TSharedPtr<SJointGraphPreviewer> InGraphPreviewer);
	TSharedPtr<SJointGraphPreviewer> GetGraphPreviewer() const;
	void ClearGraphPreviewer();

public:
	
	TWeakObjectPtr<UJointManager> JointManager;
	
private:
	
	TSharedPtr<SJointGraphPreviewer> GraphPreviewer;
	UTextureRenderTarget2D* ThumbnailRenderTarget = nullptr;
	
private:
	
	bool bIsLocked = false;
	
public:
	
	void RequestUpdateGraphPreviewer();
	bool ShouldUpdateGraphPreviewer() const;
	bool IsUpdateJustFinished() const;
	void TickGraphPreviewerUpdate();
	void ResetGraphPreviewerUpdate();

private:
	
	int GraphUpdateRemainingFrames = IdleGraphUpdateFrames;
	
	const static int MaxGraphUpdateFrames = 6;
	const static int IdleGraphUpdateFrames = -1;
	
public:
	
	bool operator== (const FJointManagerThumbnailRendererResourcePoolElement& Other) const
	{
		return JointManager == Other.JointManager;
	}
	
	bool operator!= (const FJointManagerThumbnailRendererResourcePoolElement& Other) const
	{
		return !(*this == Other);
	}
};

FORCEINLINE uint32 GetTypeHash(const FJointManagerThumbnailRendererResourcePoolElement& Element)
{
	return GetTypeHash(Element.JointManager);
}

FORCEINLINE bool operator==(const TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement>& A, const TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement>& B)
{
	if (A.IsValid() && B.IsValid())
	{
		return *A == *B;
	}
	
	return A.IsValid() == B.IsValid();
}

struct FJointManagerThumbnailPoolElementKeyFuncs :
	BaseKeyFuncs<
		TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement>,
		UJointManager*
	>
{
	// Get the key from the given element. The key is used for indexing in the set.
	static FORCEINLINE UJointManager* GetSetKey(TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement> Element)
	{
		return Element->JointManager.Get();
	}
	
	// Check if the keys match. This is used to find an element in the set based on the key.
	static FORCEINLINE bool Matches(UJointManager* A, UJointManager* B)
	{		
		return A == B;
	}
	
	// Calculate the hash for the given key. This is used for indexing in the set.
	static FORCEINLINE uint32 GetKeyHash(UJointManager* Key) 
	{
		return GetTypeHash(Key);
	}
};

// singleton cache for the thumbnail renderer
class FJointManagerThumbnailRendererResourceManager
{
public:
	
	FJointManagerThumbnailRendererResourceManager();
	~FJointManagerThumbnailRendererResourceManager();

public:
	
	void ClearCache();
	
public:

	TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement> FindOrAddResourcePoolElementFor(UJointManager* JointManager);
	TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement> GetResourcePoolElementFor(UJointManager* JointManager);
	void RemoveResourcePoolElementFor(UJointManager* JointManager);
	
private:
	
	TSet<TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement>, FJointManagerThumbnailPoolElementKeyFuncs> ResourcePool;
	
	TUniquePtr<FWidgetRenderer> WidgetRenderer = nullptr;

public:
	
	static FWidgetRenderer* GetWidgetRenderer() 
	{
		return Get().WidgetRenderer.Get();
	}
	
	static FJointManagerThumbnailRendererResourceManager& Get() {
		static FJointManagerThumbnailRendererResourceManager Instance;
		return Instance;
	}
	
};
