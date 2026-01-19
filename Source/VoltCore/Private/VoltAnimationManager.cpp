//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "VoltAnimationManager.h"
#include "VoltAnimation.h"
#include "VoltAnimationTrack.h"
#include "VoltCoreLogChannels.h"
#include "VoltInterface.h"
#include "VoltModuleItem.h"
#include "VoltVariableBase.h"
#include "VoltVariableCollection.h"


FVoltAnimationTrack UVoltAnimationManager::PlayAnimationFor(TScriptInterface<IVoltInterface> TargetVoltInterface,
                                                            const UVoltAnimation* Animation)
{
	if (Animation == nullptr)
	{
#if WITH_EDITOR
		UE_LOG(LogVoltCore, Error, TEXT("An empty animation has been requested to be played. Reverted the action."));
#endif
		return FVoltAnimationTrack::NullTrack; // Revert of there is no animation to play.
	}

	if (TargetVoltInterface == nullptr || !TargetVoltInterface.GetInterface())
	{
#if WITH_EDITOR
		UE_LOG(LogVoltCore, Error,
		       TEXT("An invalid slate interface has been requested to play an animation for. Reverted the action."));
#endif
		return FVoltAnimationTrack::NullTrack; // Revert of there is no animation to play.
	}

	const FVoltAnimationTrack& Track = MakeTrackWith(TargetVoltInterface, Animation);

	EnqueueOnAddAnimationTrack(Track);

	return Track;
}

FVoltAnimationTrack UVoltAnimationManager::MakeTrackWith(TScriptInterface<IVoltInterface> VoltInterface,
                                                         const UVoltAnimation* Animation)
{
	if (Animation == nullptr || VoltInterface == nullptr)
	{
		return FVoltAnimationTrack::NullTrack;
	}

	UVoltAnimation* AnimationInstance = DuplicateObject<UVoltAnimation>(Animation, this);

	if (!AnimationInstance)
	{
		return FVoltAnimationTrack::NullTrack;
	}

	for (UVoltModuleItem* Module : AnimationInstance->Modules)
	{
		if (!Module) continue;

		Module->SetVoltSlate(VoltInterface);
	}

	return FVoltAnimationTrack(VoltInterface, AnimationInstance);
}

void UVoltAnimationManager::EnqueueOnAddAnimationTrack(const FVoltAnimationTrack& Track)
{
	if (!AddAnimationTrackQueue.Contains(Track))
	{
		AddAnimationTrackQueue.Add(Track);
	}
}

void UVoltAnimationManager::EnqueueOnDeleteAnimationTrack(const FVoltAnimationTrack& Track)
{
	if (!DeleteAnimationTrackQueue.Contains(Track))
	{
		DeleteAnimationTrackQueue.Add(Track);
	}
}

void UVoltAnimationManager::DequeueOnAddAnimationTrack(const FVoltAnimationTrack& Track)
{
	AddAnimationTrackQueue.Remove(Track);
}

void UVoltAnimationManager::DequeueOnDeleteAnimationTrack(const FVoltAnimationTrack& Track)
{
	DeleteAnimationTrackQueue.Remove(Track);
}

void UVoltAnimationManager::ApplyQueuedAnimationTrackRequests()
{
	for (FVoltAnimationTrack& DeletionQueue : DeleteAnimationTrackQueue)
	{
		ProcessDeleteAnimationTrack(DeletionQueue);
	}
	
	for (FVoltAnimationTrack& AdditionQueue : AddAnimationTrackQueue)
	{
		ProcessAddAnimationTrack(AdditionQueue);
	}

	AddAnimationTrackQueue.Empty();
	DeleteAnimationTrackQueue.Empty();
}

void UVoltAnimationManager::ProcessAddAnimationTrack(FVoltAnimationTrack& Track)
{
	if (OnTrackAdded.IsBound())
	{
		OnTrackAdded.Broadcast(this, Track);
	}

	if (OnTrackAdded_NonDynamic.IsBound())
	{
		OnTrackAdded_NonDynamic.Broadcast(this, Track);
	}

	//Broadcast the event.
	if (OnAnimationPlayed.IsBound())
	{
		OnAnimationPlayed.Broadcast(this, Track, Track.TargetAnimation && Track.TargetAnimation.IsValid() ? Track.TargetAnimation.Get() : nullptr);
	}

	if(OnAnimationPlayed_NonDynamic.IsBound())
	{
		OnAnimationPlayed_NonDynamic.Broadcast(this, Track, Track.TargetAnimation && Track.TargetAnimation.IsValid() ? Track.TargetAnimation.Get() : nullptr);
	}

	AnimationTracks.Add(Track);
	
}

void UVoltAnimationManager::ProcessDeleteAnimationTrack(FVoltAnimationTrack& Track)
{
	//Broadcast the event.
	if (OnAnimationEnded.IsBound())
	{
		OnAnimationEnded.Broadcast(this, Track, Track.TargetAnimation && Track.TargetAnimation.IsValid() ? Track.TargetAnimation.Get() : nullptr);
	}

	if(OnAnimationEnded_NonDynamic.IsBound())
	{
		OnAnimationEnded_NonDynamic.Broadcast(this, Track, Track.TargetAnimation && Track.TargetAnimation.IsValid() ? Track.TargetAnimation.Get() : nullptr);
	}
	
	//Broadcast the event.
	if (OnTrackRemoved.IsBound())
	{
		OnTrackRemoved.Broadcast(this, Track);
	}

	if (OnTrackRemoved_NonDynamic.IsBound())
	{
		OnTrackRemoved_NonDynamic.Broadcast(this, Track);
	}
	
	//Release its animation.
	if (Track.TargetAnimation)
	{
		Track.TargetAnimation = nullptr;
	}

	AnimationTracks.Remove(Track);
}


void UVoltAnimationManager::FlushUnnecessaryTrack()
{
	for (const FVoltAnimationTrack& AnimationTrack : AnimationTracks) {

		if (!AnimationTrack.TargetAnimation)
		{
			FlushTrack(AnimationTrack);
			continue;
		}

		if (!AnimationTrack.TargetAnimation->IsActive())
		{
			FlushTrack(AnimationTrack);
		}
		
	}
}


bool UVoltAnimationManager::HasTrack(const FVoltAnimationTrack& Track)
{
	return AddAnimationTrackQueue.Contains(Track) || AnimationTracks.Contains(Track);
}

void UVoltAnimationManager::FlushTrack(const FVoltAnimationTrack& Track)
{
	if(!HasTrack(Track)) return;
		
	//If add queue has this one, remove as well.
	DequeueOnAddAnimationTrack(Track);
	
	//Book that up for the deletion.
	EnqueueOnDeleteAnimationTrack(Track);
}

void UVoltAnimationManager::FlushTracksFor(TScriptInterface<IVoltInterface> TargetVoltInterface)
{
	const int num = AnimationTracks.Num();

	for (const FVoltAnimationTrack& AnimationTrack : AnimationTracks) {
		
		if (AnimationTrack.TargetSlateInterface.GetInterface() == TargetVoltInterface.GetInterface())
		{
			FlushTrack(AnimationTrack);
		}
	}
}


void UVoltAnimationManager::FlushAllTracks()
{
	const int num = AnimationTracks.Num();

	for (const FVoltAnimationTrack& AnimationTrack : AnimationTracks) {
		FlushTrack(AnimationTrack);
	}
}

const TSet<FVoltAnimationTrack>& UVoltAnimationManager::GetAnimationTracks()
{
	return AnimationTracks;
}

bool UVoltAnimationManager::IsPlayingAnimation()
{
	return !AnimationTracks.IsEmpty();
}

void UVoltAnimationManager::Tick(float DeltaTime)
{
	const UVoltSubsystem* Subsystem = UVoltSubsystem::Get();

	if (!Subsystem) return;
	
	// Manual update - for the case whether this object can not rely on external updates (thread)
	if (!Subsystem->IsUtilizingMultithreading())
	{
		ApplyQueuedAnimationTrackRequests();

		if(!AnimationTracks.IsEmpty()) ProcessModuleUpdate(DeltaTime);
	}

	//These actions must be atomic and done in only one thread.
	if (!Subsystem->IsModuleUpdateThreadWorking())
	{
		ApplyVariables();
		
		FlushUnnecessaryTrack();
	}

}


void UVoltAnimationManager::ApplyVariables()
{
	for (FVoltAnimationTrack& AnimationTrack : AnimationTracks)
	{
		if(!AnimationTrack.TargetSlateInterface) continue;
			
		if (UVoltVariableCollection* Collection = AnimationTrack.TargetSlateInterface->GetVoltVariableCollection())
		{
			//Process all the queued variable to the actual list.
			Collection->ProcessQueue();
			
			const TArray<UVoltVariableBase*>& Variables = Collection->GetVariables();
				
			const int IterNum = Variables.Num();
			
			TWeakPtr<SWidget> Slate = AnimationTrack.TargetSlateInterface->GetTargetSlate();

			if(!Slate.IsValid()) continue;

			for (int i = 0; i < IterNum; ++i)
			{
				if (!Variables.IsValidIndex(i)) continue;

				if(UVoltVariableBase* Variable = Variables[i])
				{
					Variable->ApplyVariable(Slate);
				}
			}
			
		}
		
	}
}

const TSet<FVoltInterfaceElement> UVoltAnimationManager::GetVoltInterfacesBeingAnimated()
{
	TSet<FVoltInterfaceElement> Set;

	for (FVoltAnimationTrack& AnimationTrack : AnimationTracks)
	{
		Set.Add(AnimationTrack.TargetSlateInterface);
	}

	return Set;
}


void UVoltAnimationManager::ProcessModuleUpdate(float DeltaTime)
{
	for (FVoltAnimationTrack& AnimationTrack : AnimationTracks)
	{
		if (!AnimationTrack.TargetAnimation)
		{
			continue;
		}

		if (!AnimationTrack.TargetSlateInterface)
		{
			continue;
		}

		for (UVoltModuleItem* Module : AnimationTrack.TargetAnimation->Modules)
		{
			if (!Module)
			{
#if WITH_EDITOR
				UE_LOG(LogVoltCore, Log, TEXT("There is a empty module slot in the %s. Please check out the asset.")
					   , *AnimationTrack.TargetAnimation.Get()->GetPathName());
#endif

				continue;
			}

			if (!AnimationTrack.TargetSlateInterface.GetObject())
			{
#if WITH_EDITOR
				UE_LOG(LogVoltCore, Log,
					   TEXT(
						   "VoltAnimationManager %s detected a slate interface that was not derived from UObject has been provided. Make sure to derive it from UObject."
					   ), *this->GetName());
#endif

				continue;
			}

			if (!Module->IsBegunPlay())
			{
				Module->BeginPlayModule();
			}
			
			if(Module->IsEndedPlay()) continue;

			if (!Module->IsActive())
			{
				Module->EndPlayModule();
				continue;
			}

			Module->ModifySlateVariable(DeltaTime, AnimationTrack.TargetSlateInterface);
		}
	}
}


void UVoltAnimationManager::ReleaseAll()
{
	//GC will handle the leftover for it.

	FlushAllTracks();

	SetOwnerVoltInterface(nullptr);
}

void UVoltAnimationManager::SetOwnerVoltInterface(const TScriptInterface<IVoltInterface> NewOwnerVoltInterface)
{
	OwnerVoltInterface = NewOwnerVoltInterface;
}

bool UVoltAnimationManager::CheckShouldDestruct() const
{
	if (const UVoltSubsystem* Subsystem = UVoltSubsystem::Get(); GetOuter() && GetOuter() != Subsystem)
	{
		return false;
	}

	if (IsRooted())
	{
		return false;
	}

	if (OwnerVoltInterface != nullptr && OwnerVoltInterface->GetTargetSlate().IsValid())
	{
		return false;
	}

	return true;
}

void UVoltAnimationManager::DestructSelf()
{
	ReleaseAll();

	MarkAsGarbage();
}
