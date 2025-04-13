//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltAnimationTrack.h"
#include "VoltInterface.h"
#include "VoltSubsystem.h"
#include "Misc/Attribute.h"
#include "Templates/SubclassOf.h"
#include "Engine/Engine.h"
#include "VoltAnimationManager.generated.h"

class UVoltSubsystem;
class UVoltProxy;
class UVoltAnimation;
class UVoltAnimationManager;

/**
 * IMPORTANT : VOLT 1.1 now supports tick action on every situation. Just use it as you want!
 *
 * Any animation that works on Volt will not be played on the debug mode of the editor & slate application by default.
 * This class is not belong to the slate application architecture, and this is the limitation of the engine, because they hard-coded the refresh action of the slates in while-loop while not triggering any other engine tick events.
 * But we can still make it work on the debug state by executing tick action manually on the slate's tick event.
 *
 * For example, we can do something like this:
 *
 * void SContextTextStyler::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
 * {
 *		if(!UVoltAnimationManager::CheckCanTickAutomatically()) VoltAnimManager->Tick(InDeltaTime);
 *		
 *		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
 *	}
 */

/**
 * Volt animation manager uses its unique way to be marked as garbage by itself.
 * 
 * Every Volt animation manager will be self-destructed if it doesn't fulfill any of the requirements.
 * 
 * The requirements that the animation manager requires is like as following:
 * 1. If it has any specific outer other than the VoltSubsystem, then it will ignore the self-destruction.
 * 2. If it has any valid slates assigned on the animation manager, then it will ignore the self-destruction.
 *
 * So, you have to assign at least one valid slate to make it persistent.
 * If you are going to use Volt on animating a layout slate, then we recommend to assign the layout slate itself to prevent GC.
 * Like this:
 * 
 *  VOLT_IMPLEMENT_MANAGER(&AnimationManager);
 *	//To prevent GC.
 *	AnimationManager->AssignSlate(this->AsShared());
 *	
 */




DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAnimationPlayed,
                                               UVoltAnimationManager*, Manager,
                                               const FVoltAnimationTrack&, Track,
                                               const UVoltAnimation*, Animation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAnimationEnded,
                                               UVoltAnimationManager*, Manager,
                                               const FVoltAnimationTrack&, Track,
                                               const UVoltAnimation*, Animation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTrackAdded,
                                             UVoltAnimationManager*, Manager,
                                             const FVoltAnimationTrack&, Track);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTrackRemoved,
                                             UVoltAnimationManager*, Manager,
                                             const FVoltAnimationTrack&, Track);

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAnimationPlayed_NonDynamic,
											   UVoltAnimationManager*,
											   const FVoltAnimationTrack&,
											   const UVoltAnimation*);

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAnimationEnded_NonDynamic,
                                               UVoltAnimationManager*,
                                               const FVoltAnimationTrack&,
                                               const UVoltAnimation*);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTrackAdded_NonDynamic,
											   UVoltAnimationManager*,
											   const FVoltAnimationTrack&);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTrackRemoved_NonDynamic,
											   UVoltAnimationManager*,
											   const FVoltAnimationTrack&);
UCLASS()
class VOLTCORE_API UVoltAnimationManager : public UObject
{
	GENERATED_BODY()
	
public:
	
	/**
	 * Play a certain animation with the specified Volt Interface, and return the animation track for the animation.
	 * If you're looking for the functions that can stop the animations, look for the function with "track" on its name.
	 * @param TargetVoltInterface Target Volt Interface to play animation with.
	 * @param Animation The animation to play.
	 * @return The track of the animation. if something went wrong, it will return FVoltAnimationTrack::NullTrack instead.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	FVoltAnimationTrack PlayAnimationFor(TScriptInterface<IVoltInterface> TargetVoltInterface,
										 const UVoltAnimation* Animation);

private:
	
	/**
	 * Make and return a new track with the provided interfaces.
	 * This action includes duplicating the animation and with assigning it on the new track while assigning TargetVoltInterface to the modules of the animation.
	 * @param VoltInterface Volt Interface to animate in this track. 
	 * @param Animation Animation to play.
	 * @return The newly created track.
	 */
	FVoltAnimationTrack MakeTrackWith(TScriptInterface<IVoltInterface> VoltInterface, const UVoltAnimation* Animation);

public:
	
	/**
	 * Flush specific animation track.
	 * @param Track The track to flush.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void FlushTrack(const FVoltAnimationTrack& Track);
	
	/**
	 * Flush all the animation tracks for specific volt interface.
	 * @param TargetVoltInterface specific volt interface that it will flush animation tracks for.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void FlushTracksFor(TScriptInterface<IVoltInterface> TargetVoltInterface);

	/**
	 * Flush all the tracks.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void FlushAllTracks();
	
	/**
	 * Flush all the unnecessary track.
	 * Iterate the tracks and pop them if needed.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void FlushUnnecessaryTrack();

public:

	/**
	 * Check whether it has the specified animation track. 
	 * @param Track The track to check 
	 * @return whether it has the specified animation track.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	bool HasTrack(const FVoltAnimationTrack& Track);
	
	/**
	 * Get all animation tracks.
	 * @return array of the tracks
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	const TSet<FVoltAnimationTrack>& GetAnimationTracks();

public:

	/**
	 * Return whether this manager is playing any animation. If it has any valid track, it will return true.
	 * @return whether this manager is playing any animation.
	 */
	bool IsPlayingAnimation();

private:

	//Animations for the slates.
	UPROPERTY(VisibleAnywhere, Category="Animated Slates", SkipSerialization, DuplicateTransient, Transient)
	TSet<FVoltAnimationTrack> AnimationTracks;

private:

	/**
	 * Volt 1.1 : Now track addition & deletion actions are asynchronous. We queue all the requests in the queues for each action, and handle them when the volt update thread is ready to go. (sync with it.)
	 * Don't touch it if you don't fully understand the code.
	 */
	
	TSet<FVoltAnimationTrack> AddAnimationTrackQueue;
	
	TSet<FVoltAnimationTrack> DeleteAnimationTrackQueue;

	FORCEINLINE void EnqueueOnAddAnimationTrack(const FVoltAnimationTrack& Track);

	FORCEINLINE void EnqueueOnDeleteAnimationTrack(const FVoltAnimationTrack& Track);

	FORCEINLINE void DequeueOnAddAnimationTrack(const FVoltAnimationTrack& Track);

	FORCEINLINE void DequeueOnDeleteAnimationTrack(const FVoltAnimationTrack& Track);
	
	void ProcessAddAnimationTrack(FVoltAnimationTrack& Track);
	
	void ProcessDeleteAnimationTrack(FVoltAnimationTrack& Track);
	
	void ApplyQueuedAnimationTrackRequests();

	friend FVoltModuleRunnable;

public:
	
	/**
	 * Get the whole volt interfaces that are being animated by this animation manager.
	 * @return Found Volt Interfaces that are being animated.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	const TSet<FVoltInterfaceElement> GetVoltInterfacesBeingAnimated();


public:
	
	/**
	 * Tick the object. This will update the slates' animation.
	 * Execute this function to manually trigger animation update.
	 */
	void Tick(float DeltaTime);

	/**
	 * Process the module calculation of the module actions.
	 * @param DeltaTime Delta time from the last update.
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void ProcessModuleUpdate(float DeltaTime);
	
	/**
	 * Update all variables on the track to the real slate representation.
	 * This function is not yet designed to be executed outside the game thread.
	 */
	virtual void ApplyVariables();

public:

	/**
	 * Set owner volt interface of the animation manager.
	 * @param NewOwnerVoltInterface New owner volt interface that this animation will rely on.
	 */
	void SetOwnerVoltInterface(const TScriptInterface<IVoltInterface> NewOwnerVoltInterface);

private:

	/**
	 * The owner VoltInterface of the animation manager.
	 */
	TScriptInterface<IVoltInterface> OwnerVoltInterface;
	
private:

	/**
	 * Check whether this animation manager fulfill any of the requirements so can ignore the self-destruction.
	 * Every Volt animation manager will be self-destructed if it doesn't fulfill any of the requirements.
	 * 
	 * If any of the following requirements has been fulfilled, animation manager will stay:
	 * 
	 * 1. If it has any specific outer other than the VoltSubsystem
	 * 2. If it has a valid owner slate
	 * 3. If the object itself is rooted
	 */
	bool CheckShouldDestruct() const;

	/**
	 * self-destruct the object instance.
	 * This class NEVER call this function by itself.
	 */
	void DestructSelf();

	friend UVoltSubsystem;

public:
	
	/**
	 * Release all data this animation manager has.
	 */
	UFUNCTION(BlueprintCallable, Category="lifecycle")
	void ReleaseAll();

public:
	
	UPROPERTY(BlueprintAssignable, Category="Manager Action")
	FOnAnimationPlayed OnAnimationPlayed;

	UPROPERTY(BlueprintAssignable, Category="Manager Action")
	FOnAnimationEnded OnAnimationEnded;

	UPROPERTY(BlueprintAssignable, Category="Manager Action")
	FOnTrackAdded OnTrackAdded;

	UPROPERTY(BlueprintAssignable, Category="Manager Action")
	FOnTrackRemoved OnTrackRemoved;

public:

	FOnAnimationPlayed_NonDynamic OnAnimationPlayed_NonDynamic;
	
	FOnAnimationEnded_NonDynamic OnAnimationEnded_NonDynamic;
	
	FOnTrackAdded_NonDynamic OnTrackAdded_NonDynamic;
	
	FOnTrackRemoved_NonDynamic OnTrackRemoved_NonDynamic;
	
};