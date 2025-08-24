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
	
	/**
	 * Trigger the update task (Unlock Semaphore) with the provided delta time.
	 * Use this to trigger the thread to work if it's waiting for the next trigger.
	 */
	void TriggerTask(float DeltaTime);
	
	void AddAnimationManager(UVoltAnimationManager* AnimationManagersArr);
	
	void AddAnimationManagers(const TArray<UVoltAnimationManager*>& AnimationManagersArr);
	
	void RemoveAnimationManager(UVoltAnimationManager* AnimationManagersArr);
	
	void RemoveAnimationManagers(const TArray<UVoltAnimationManager*>& AnimationManagersArr);
	
	const bool IsWorking() const;
	
	FORCEINLINE void MarkAsPendingKill();

	FORCEINLINE const bool IsPendingKill() const;

private:

	void ProcessAnimationManagerTrackQueue();

	void ProcessBufferedAnimationManagersRequest();
	
	// (blocking call) Stop the thread run and wait until it's fully stopped.
	void StopRunBlocking();

	// unlock the semaphore to let the thread run.
	void UnlockSemaphore();

	FORCEINLINE void MarkAsWorking();

	FORCEINLINE void UnmarkAsWorking();

public:

	TArray<UVoltAnimationManager*> AnimationManagers;
	
private:

	/**
	 * Animation managers that are queued to be handled on the thread.
	 */
	TArray<UVoltAnimationManager*> AssignBufferAnimationManagers;
	
	/**
	 * Animation managers that are queued to be excluded on the thread.
	 */
	TArray<UVoltAnimationManager*> DeletionBufferAnimationManagers;
	
	// Whether the thread is currently processing tasks - when Run() is executing.
	bool bIsWorking = false;
	
	bool bPendingKill = false;
	
	FEvent* ThreadRunEvent_Semaphore;
	
	FRunnableThread* Thread = nullptr;

	float ThreadWorkDeltaTime = 0;

public:

	FORCEINLINE const float& GetThreadWorkDeltaTime() const;
	
};
