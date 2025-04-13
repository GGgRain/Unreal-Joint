//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "SubSystem/JointSubsystem.h"

#include "JointActor.h"
#include "JointManager.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectIterator.h"
#include "Engine/GameInstance.h"

#include "EngineUtils.h"
#include "GameplayTagContainer.h"
#include "Joint.h"

#include "TimerManager.h"


/*
AJointActor* UJointSubsystem::CreateJoint(
	UObject* WorldContextObject,
	UJointManager* JointAssetToPlay,
	TSubclassOf<AJointActor> JointInstanceSubclass)
{
	AJointActor* JointInstanceActor = nullptr;

	if (JointAssetToPlay)
	{
		JointInstanceActor = WorldContextObject->GetWorld()->SpawnActor<AJointActor>(JointInstanceSubclass.Get() ? JointInstanceSubclass : AJointActor::StaticClass());

		UJointManager* TransientJointManager = nullptr;

		TransientJointManager = DuplicateObject<UJointManager>(JointAssetToPlay, JointInstanceActor);

		if (JointInstanceActor)
		{
			JointInstanceActor->SetJointManager(TransientJointManager);

#if WITH_EDITOR
			//For debugging purpose.

			JointInstanceActor->OriginalJointManager = JointAssetToPlay;
#endif
		}
	}

	return JointInstanceActor;
}
*/


AJointActor* UJointSubsystem::CreateJoint(
	UObject* WorldContextObject,
	UJointManager* JointAssetToPlay,
	TSubclassOf<AJointActor> OptionalJointInstanceSubclass)
{
	if (!WorldContextObject || !WorldContextObject->GetWorld() || JointAssetToPlay == nullptr || !JointAssetToPlay->IsValidLowLevel())
		return nullptr;

	if(AJointActor* JointActor = WorldContextObject->GetWorld()->SpawnActor<AJointActor>(OptionalJointInstanceSubclass.Get() ? OptionalJointInstanceSubclass.Get() : AJointActor::StaticClass()))
	{
		JointActor->RequestSetJointManager(JointAssetToPlay);
		
		return JointActor;
	}

	return nullptr;
}

AJointActor* UJointSubsystem::FindJoint(UObject* WorldContextObject, FGuid JointGuid)
{
	if (WorldContextObject != nullptr && WorldContextObject->GetWorld())
	{
		for (TActorIterator<AJointActor> ActorItr(WorldContextObject->GetWorld()); ActorItr; ++ActorItr)
		{
			if (!(*ActorItr)->IsValidLowLevel()) { continue; }

			if ((*ActorItr)->JointGuid == JointGuid) return *ActorItr;
		}
	}

	return nullptr;
}

TArray<class AJointActor*> UJointSubsystem::GetAllJoints(UObject* WorldContextObject)
{
	TArray<AJointActor*> Array;

	if (WorldContextObject != nullptr && WorldContextObject->GetWorld())
	{
		for (TActorIterator<AJointActor> ActorItr(WorldContextObject->GetWorld()); ActorItr; ++ActorItr)
		{
			if (!(*ActorItr)->IsValidLowLevel()) { continue; }

			Array.Add(*ActorItr);
		}
	}
	return Array;
}


UJointSubsystem* UJointSubsystem::Get(UObject* WorldContextObject)
{
	if (WorldContextObject != nullptr)
		if (UWorld* WorldRef = WorldContextObject->GetWorld())
			if (UGameInstance* GI = WorldRef->GetGameInstance())
				if (UJointSubsystem* SS = GI->GetSubsystem<UJointSubsystem>())
					return SS;

	return nullptr;
}

TArray<FGuid> UJointSubsystem::GetJointsGuidStartedOnThisFrame(UObject* WorldContextObject)
{
	if (UJointSubsystem* Subsystem = Get(WorldContextObject)){
		return Subsystem->CachedJointBeginOnFrame;
	}
	
	TArray<FGuid> Array;
	return Array;
}

TArray<FGuid> UJointSubsystem::GetJointsGuidEndedOnThisFrame(UObject* WorldContextObject)
{
	if (UJointSubsystem* Subsystem = Get(WorldContextObject)){
		return Subsystem->CachedJointEndOnFrame;
	}
	
	TArray<FGuid> Array;
	return Array;
}


void UJointSubsystem::OnJointStarted(AJointActor* Actor)
{
	if (Actor == nullptr) return;

	if (!Actor->IsValidLowLevel()) return;

	if (Actor->IsJointStarted()) return;

	AddStartedJointToCaches(Actor);

	BroadcastOnJointStarted(Actor, Actor->JointGuid);

	RequestFrameCachesClearOnNextFrame();

	CachedTime = GetWorld()->GetTimeSeconds();
}

void UJointSubsystem::OnJointEnded(AJointActor* Actor)
{
	if (Actor == nullptr) return;

	if (!Actor->IsValidLowLevel()) return;

	if (Actor->IsJointEnded()) return;

	AddEndedJointToCaches(Actor);

	BroadcastOnJointEnded(Actor, Actor->JointGuid);

	RequestFrameCachesClearOnNextFrame();

	CachedTime = GetWorld()->GetTimeSeconds();
}

void UJointSubsystem::AddStartedJointToCaches(AJointActor* Actor)
{
	if (GetWorld() == nullptr) return;

	//Clear cache immediately since we see the cache has been stored in the previous frame.
	if (CachedTime != GetWorld()->GetTimeSeconds()) ClearCachedJointFrameData();

	if (Actor == nullptr) return;

	CachedJointBeginOnFrame.Add(Actor->JointGuid);

	CachedTime = GetWorld()->GetTimeSeconds();
}

void UJointSubsystem::AddEndedJointToCaches(AJointActor* Actor)
{
	if (GetWorld() == nullptr) return;

	//Clear cache immediately since we see the cache has been stored in the previous frame.
	if (CachedTime != GetWorld()->GetTimeSeconds()) ClearCachedJointFrameData();

	if (Actor == nullptr) return;

	CachedJointEndOnFrame.Add(Actor->JointGuid);

	CachedTime = GetWorld()->GetTimeSeconds();
}


void UJointSubsystem::RequestFrameCachesClearOnNextFrame()
{
	if (bCacheClearRequested) return;

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		this->ClearCachedJointFrameData();
	});

	bCacheClearRequested = true;
}


void UJointSubsystem::ClearCachedJointFrameData()
{
	if (!CachedJointBeginOnFrame.IsEmpty()) CachedJointBeginOnFrame.Empty();
	if (!CachedJointEndOnFrame.IsEmpty()) CachedJointEndOnFrame.Empty();

	bCacheClearRequested = false;
}

void UJointSubsystem::BroadcastOnJointStarted(AJointActor* Actor, FGuid JointGuid)
{
	if (OnJointBeginDelegate.IsBound()) OnJointBeginDelegate.Broadcast(Actor, JointGuid);

#if WITH_EDITOR

	if (FJointModule* Module = &FModuleManager::GetModuleChecked<FJointModule>("Joint"); Module != nullptr && Module->
		OnJointDebuggerMentionJointBeginPlay.IsBound())
	{
		Module->OnJointDebuggerMentionJointBeginPlay.ExecuteIfBound(Actor, JointGuid);
	}

#endif
}

void UJointSubsystem::BroadcastOnJointEnded(AJointActor* Actor, FGuid JointGuid)
{
	if (OnJointEndDelegate.IsBound()) OnJointEndDelegate.Broadcast(Actor, JointGuid);

#if WITH_EDITOR

	if (FJointModule* Module = &FModuleManager::GetModuleChecked<FJointModule>("Joint"); Module != nullptr && Module->
		OnJointDebuggerMentionJointEndPlay.IsBound())
	{
		Module->OnJointDebuggerMentionJointEndPlay.ExecuteIfBound(Actor, JointGuid);
	}

#endif
}

UWorld* UJointSubsystem::GetWorld() const { return GetGameInstance()->GetWorld(); }
