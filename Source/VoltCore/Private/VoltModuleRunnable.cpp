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
	if (ThreadRunEvent_Semaphore)
	{
		//Cleanup the FEvent
		FGenericPlatformProcess::ReturnSynchEventToPool(ThreadRunEvent_Semaphore);
		ThreadRunEvent_Semaphore = nullptr;
	}

	if (Thread)
	{
		// Kill() is a blocking call, it waits for the thread to finish.
		Thread->Kill();
		delete Thread;
	}
}

uint32 FVoltModuleRunnable::Run()
{
	while (!IsPendingKill())
	{
		if(IsWorking() || IsRunningCommandlet()) return 0;
		
		if(AnimationManagers.IsEmpty()) ThreadRunEvent_Semaphore->Wait();
		
		MarkAsWorking();
		
		for (UVoltAnimationManager* AnimationManager : AnimationManagers)
		{
			if (AnimationManager == nullptr) continue;
			
			AnimationManager->ProcessModuleUpdate(GetThreadWorkDeltaTime());

		}

		UnmarkAsWorking();

		if(ThreadRunEvent_Semaphore) ThreadRunEvent_Semaphore->Wait();
	}
	
	return 0;
}

void FVoltModuleRunnable::Stop()
{
	MarkAsPendingKill();
	
	if(ThreadRunEvent_Semaphore) ThreadRunEvent_Semaphore->Trigger();
}

void FVoltModuleRunnable::TriggerTask(float DeltaTime)
{
	//If thread is already working, it will simply pile up the delta time, and do nothing.
	if(IsWorking())
	{
		ThreadWorkDeltaTime = ThreadWorkDeltaTime + DeltaTime;

		return;
	}

	//Synchronize the list.
	ProcessQueuedAnimationManagersRequest();

	ProcessAnimationManagerTrackQueue();
	
	ThreadWorkDeltaTime = DeltaTime;

	//Unlock the thread
	if(ThreadRunEvent_Semaphore) ThreadRunEvent_Semaphore->Trigger();
}

void FVoltModuleRunnable::AddAnimationManager(UVoltAnimationManager* AnimationManager)
{
	AdditionQueueAnimationManagers.Add(AnimationManager);
}

void FVoltModuleRunnable::RemoveAnimationManager(UVoltAnimationManager* AnimationManager)
{
	DeletionQueueAnimationManagers.Add(AnimationManager);
}

void FVoltModuleRunnable::AddAnimationManagers(const TArray<UVoltAnimationManager*>& AnimationManagersArr)
{
	AdditionQueueAnimationManagers.Append(AnimationManagersArr);
}

void FVoltModuleRunnable::RemoveAnimationManagers(const TArray<UVoltAnimationManager*>& AnimationManagersArr)
{
	DeletionQueueAnimationManagers.Append(AnimationManagersArr);
}

void FVoltModuleRunnable::ProcessQueuedAnimationManagersRequest()
{
	for (UVoltAnimationManager* AdditionQueueAnimationManager : AdditionQueueAnimationManagers)
	{
		if(!AdditionQueueAnimationManager) continue;

		AnimationManagers.Add(AdditionQueueAnimationManager);
	}

	for (UVoltAnimationManager* DeletionQueueAnimationManager : DeletionQueueAnimationManagers)
	{
		if(!DeletionQueueAnimationManager) continue;

		AnimationManagers.Remove(DeletionQueueAnimationManager);
	}
	
	AdditionQueueAnimationManagers.Empty();
	DeletionQueueAnimationManagers.Empty();
	
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

void FVoltModuleRunnable::Exit()
{
	FRunnable::Exit();
}

bool FVoltModuleRunnable::Init()
{
	return FRunnable::Init();
}

void FVoltModuleRunnable::MarkAsWorking()
{
	bWorking = true;
}

void FVoltModuleRunnable::UnmarkAsWorking()
{
	bWorking = false;
}

const bool FVoltModuleRunnable::IsWorking() const
{
	return bWorking;
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
