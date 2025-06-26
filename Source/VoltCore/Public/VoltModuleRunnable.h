//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/RunnableThread.h"
#include "HAL/Runnable.h"
#include "GenericPlatform/GenericPlatformProcess.h"

/**
 * A thread for the module execution. This doesn't update the slate itself.
 */

class UVoltAnimationManager;

class FVoltModuleRunnableManagerUnit : public TSharedFromThis<FVoltModuleRunnableManagerUnit>
{
	
public:
	
	UVoltAnimationManager* AnimationManager;

	TArray<FGuid> KnownTrackToProcess;

public:

	void UpdateKnownTrackToPrecess();
	
	
};

class FVoltModuleRunnable : public FRunnable
{
public:
	
	FVoltModuleRunnable();
	~FVoltModuleRunnable();

public:
	
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	virtual bool Init() override;


public:
	
	void ProcessAnimationManagerTrackQueue();
	
	void TriggerTask(float DeltaTime);

private:

	/**
	 * Animation managers that are queued to be handled on the thread.
	 */
	TArray<UVoltAnimationManager*> AdditionQueueAnimationManagers;
	
	/**
	 * Animation managers that are queued to be excluded on the thread.
	 */
	TArray<UVoltAnimationManager*> DeletionQueueAnimationManagers;

public:


	void AddAnimationManager(UVoltAnimationManager* AnimationManagersArr);

	void RemoveAnimationManager(UVoltAnimationManager* AnimationManagersArr);
	

	void AddAnimationManagers(const TArray<UVoltAnimationManager*>& AnimationManagersArr);

	void RemoveAnimationManagers(const TArray<UVoltAnimationManager*>& AnimationManagersArr);

private:
	
	void ProcessQueuedAnimationManagersRequest();

public:

	TArray<UVoltAnimationManager*> AnimationManagers;

public:

	FORCEINLINE void MarkAsWorking();

	FORCEINLINE void UnmarkAsWorking();
	
	const bool IsWorking() const;

private:

	bool bWorking = false;

public:

	FORCEINLINE void MarkAsPendingKill();

	FORCEINLINE const bool IsPendingKill() const;
	
private:
	
	bool bPendingKill = false;

private:

	FEvent* ThreadRunEvent_Semaphore;
	
	FRunnableThread* Thread = nullptr;

public:

	FORCEINLINE const float& GetThreadWorkDeltaTime() const;

private:

	float ThreadWorkDeltaTime = 0;
	
};
