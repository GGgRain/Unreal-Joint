//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "VoltModuleRunnable.h"

#include "VoltAnimationManager.h"

FVoltModuleRunnable::FVoltModuleRunnable()
{
	ThreadRunEvent_Semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	Thread = FRunnableThread::Create(this, TEXT("VoltModuleUpdateThread"));
}

FVoltModuleRunnable::~FVoltModuleRunnable()
{
	StopRunBlocking();

	// Kill the running thread.
	if (Thread != nullptr)
	{
		Thread->Kill(true);

		delete Thread;
		Thread = nullptr;
	}

	//clean up the semaphore
	if(ThreadRunEvent_Semaphore)
	{
		FGenericPlatformProcess::ReturnSynchEventToPool(ThreadRunEvent_Semaphore);
		ThreadRunEvent_Semaphore = nullptr;
	}
	
}

uint32 FVoltModuleRunnable::Run()
{
	while (!IsPendingKill())
	{
		// If we are already working, or if we are in commandlet mode, do nothing.
		// This is to prevent re-entrance and processing during commandlet runs - it seems like the packaging process want all the existing threads to be joined to finish the process.
		// if the semaphore is null, we are probably shutting down - exit.
		if(IsWorking() || IsRunningCommandlet() || !ThreadRunEvent_Semaphore) return 0;
		
		// Work only if we have something to do.
		if (!AnimationManagers.IsEmpty())
		{
			MarkAsWorking();

			// Critical section - we are now working.
			
			for (UVoltAnimationManager* AnimationManager : AnimationManagers)
			{
				if (AnimationManager == nullptr) continue;
			
				AnimationManager->ProcessModuleUpdate(GetThreadWorkDeltaTime());
			}

			// End of critical section - we are done working.

			UnmarkAsWorking();
		}
			
		// wait for the next trigger ( for the next frame - FVoltModuleRunnable::Run() can be triggered only one time per frame. )
		if (ThreadRunEvent_Semaphore) ThreadRunEvent_Semaphore->Wait();
	}
	
	return 0;
}

void FVoltModuleRunnable::StopRunBlocking()
{
	// Mark it for kill - we will exit the thread loop now.
	Stop();

	// blocking call; ensure we wait for the thread to finish - wait for the Run() to exit.
	UnlockSemaphore();
	
	if (Thread) Thread->WaitForCompletion();
}

void FVoltModuleRunnable::Stop()
{
	MarkAsPendingKill();
}

void FVoltModuleRunnable::Exit()
{
	// This function will be called when Run is finished (returned 0) - we don't need to do anything special here.
	FRunnable::Exit();
}

bool FVoltModuleRunnable::Init()
{
	return FRunnable::Init();
}

void FVoltModuleRunnable::UnlockSemaphore()
{
	if(ThreadRunEvent_Semaphore) ThreadRunEvent_Semaphore->Trigger();
}

void FVoltModuleRunnable::TriggerTask(float DeltaTime)
{
	if(IsWorking())
	{
		//If thread is already working, it will simply pile up the delta time, and do nothing.

		ThreadWorkDeltaTime = ThreadWorkDeltaTime + DeltaTime;
	}else
	{
		//If thread is not working, we can trigger it to work.
		
		//Synchronize the list.
		ProcessBufferedAnimationManagersRequest();

		ProcessAnimationManagerTrackQueue();
	
		ThreadWorkDeltaTime = DeltaTime;

		// Only unlock if we have something to do.
		if (!AnimationManagers.IsEmpty())
		{
			//Unlock the thread
			UnlockSemaphore();
		}
	}
}

void FVoltModuleRunnable::AddAnimationManager(UVoltAnimationManager* AnimationManager)
{
	AssignBufferAnimationManagers.Add(AnimationManager);
}

void FVoltModuleRunnable::AddAnimationManagers(const TArray<UVoltAnimationManager*>& AnimationManagersArr)
{
	AssignBufferAnimationManagers.Append(AnimationManagersArr);
}

void FVoltModuleRunnable::RemoveAnimationManager(UVoltAnimationManager* AnimationManager)
{
	DeletionBufferAnimationManagers.Add(AnimationManager);
}

void FVoltModuleRunnable::RemoveAnimationManagers(const TArray<UVoltAnimationManager*>& AnimationManagersArr)
{
	DeletionBufferAnimationManagers.Append(AnimationManagersArr);
}

void FVoltModuleRunnable::ProcessBufferedAnimationManagersRequest()
{
	// If we are working, or if we are pending kill, do nothing.
	if (IsWorking() || IsPendingKill()) return;
	
	for (UVoltAnimationManager* AssignBuffer : AssignBufferAnimationManagers)
	{
		if(!AssignBuffer) continue;

		AnimationManagers.Add(AssignBuffer);
	}

	for (UVoltAnimationManager* DeletionBuffer : DeletionBufferAnimationManagers)
	{
		if(!DeletionBuffer) continue;

		AnimationManagers.Remove(DeletionBuffer);
	}
	
	AssignBufferAnimationManagers.Empty();
	DeletionBufferAnimationManagers.Empty();
	
	AnimationManagers.RemoveAll([] (const UVoltAnimationManager* Manager)
	{
		return !Manager;
	});
}

void FVoltModuleRunnable::ProcessAnimationManagerTrackQueue()
{
	for (UVoltAnimationManager* AnimationManager : AnimationManagers)
	{
		if(AnimationManager && AnimationManager->IsValidLowLevel()) AnimationManager->ApplyQueuedAnimationTrackRequests();
	}
}


void FVoltModuleRunnable::MarkAsWorking()
{
	bIsWorking = true;
}

void FVoltModuleRunnable::UnmarkAsWorking()
{
	bIsWorking = false;
}

const bool FVoltModuleRunnable::IsWorking() const
{
	return bIsWorking;
}

void FVoltModuleRunnable::MarkAsPendingKill()
{
	bPendingKill = true;
}

const bool FVoltModuleRunnable::IsPendingKill() const
{
	return bPendingKill;
}

const float& FVoltModuleRunnable::GetThreadWorkDeltaTime() const
{
	return ThreadWorkDeltaTime;
}
