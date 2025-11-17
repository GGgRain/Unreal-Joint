
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "VoltInterface.h"
#include "VoltModuleRunnable.h"
#include "VoltSubsystem.generated.h"

class IVoltInterface;
class UVoltAnimationManager;
/**
 * A subsystem for Volt.
 * It handles some of the fundamental and essential logic for the system.
 */
UCLASS()
class VOLTCORE_API UVoltSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	
	UVoltSubsystem();

public:

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Implement this for deinitialization of instances of the system */
	virtual void Deinitialize() override;


public:
	
	/**
	 * Assign the interface to the system. This is useful when you have a custom slate that has been derived from IVoltInterface itself.
	 * @param VoltInterfaceToAssign 
	 */
	UFUNCTION(BlueprintCallable, Category="VoltInterface")
	void AssignVoltInterface(TScriptInterface<IVoltInterface> VoltInterfaceToAssign);

	/**
	 * Try to find a volt interface with the provided slate.
	 * @param SlateToFind 
	 * @return Found Volt interface for the slate. return nullptr if not present.
	 */
	TScriptInterface<IVoltInterface> FindVoltInterfaceFor(const TWeakPtr<SWidget>& SlateToFind);
	
	/**
	 * Try to find a volt interface with the provided slate.
	 * If there was no volt interface object associated with the slate, it'll create a new UVoltProxy with the slate.
	 * @param SlateToFind 
	 * @return Found (or new) Volt interface for the slate.
	 */
	TScriptInterface<IVoltInterface> FindOrAssignVoltInterfaceFor(const TWeakPtr<SWidget>& SlateToFind);

	/**
	 * Check whether we already have the provided volt interface. (assigned or not.)
	 * @param VoltInterfaceToCheck target volt interface to check.
	 * @return whether we already have the provided volt interface.
	 */
	bool HasVoltInterface(const TScriptInterface<IVoltInterface>& VoltInterfaceToCheck) const;
	
	/**
	 * Check whether we already have a volt interface that contains provided slate.
	 * @param SlateToCheck Slate to test with.
	 * @return whether we already have a volt interface that contains provided slate.
	 */
	bool HasVoltInterfaceFor(const TWeakPtr<SWidget>& SlateToCheck) const;

	/**
	 * Return the whole number of volt interfaces. This only counts the volt interfaces that has been properly assigned to ths subsystem.
	 * @return the whole number of volt interfaces.
	 */
	int GetVoltInterfaceCount() const;
	
private:
	
	/**
	 * All available volt interfaces in the system at the moment. It only contains the assigned VoltInterface.
	 * The biggest reason for why we are providing this property is to provide querying features to the slates.
	 */
	UPROPERTY(VisibleAnywhere, Category="Animated Slates", SkipSerialization, DuplicateTransient, Transient)
	TMap<uint64,FVoltInterfaceElement> VoltInterfaces;

public:
	
	/**
	 * Register an animation manager on the subsystem.
	 * This action will prevent the animation manager to be GC'd.
	 * @param AnimationManager The animation manager to register.
	 */
	void RegisterAnimationManager(UVoltAnimationManager* AnimationManager);

	/**
	 * Unregister an animation manager on the subsystem.
	 * This action will prevent the animation manager to be GC'd.
	 * @param AnimationManager The animation manager to unregister.
	 */
	void UnregisterAnimationManager(UVoltAnimationManager* AnimationManager);

public:
	
	/**
	 * Check and conduct auto-destruction of the animation managers.
	 */
	void TryDiscardAbandonedInstancesByInterval();

	/**
	 * Check and conduct auto-destruction of the animation managers.
	 */
	void DestructUnnecessaryRegisteredAnimationManager();
	
	/**
	 * Clear-out some invalid volt interfaces from the system.
	 */
	void DiscardInvalidVoltInterfaces();

private:

	/**
	 * The time stamp we use to measure the interval between the last clean up
	 */
	double CleanUpLastTime = -1;

	/**
	 * The interval we use on each clean up. unit is second.
	 * TODO: Do we need to make it accessible or be modified by the users?
	 */
	double CleanUpInterval = 37;

	void CacheCleanUpInterval();
	
private:

	/**
	 * An array of the active animation manager instances.
	 * Any animation managers must be assigned on this array to prevent GC.
	 */
	UPROPERTY()
	TArray<TObjectPtr<UVoltAnimationManager>> RegisteredAnimationManager;
	
public:
	
	//Tick related.
	
	/**
	 * Tick function that relies on the Slate application internal tick function.
	 * @param DeltaTime interval time
	 */
	virtual void OnSlateApplicationPreTick(float DeltaTime);

	/**
	 * Bind delegate on the slate application's pre-tick delegate.
	 */
	void BindOnSlateApplicationPreTick();
	
	/**
	 * Unbind delegate on the slate application's pre-tick delegate.
	 */
	void UnbindOnSlateApplicationPreTick();

private:

	/**
	 * Whether the system is assigned to be ticked by the slate application.
	 */
	bool bIsTicking = false;

public:

	/**
	 * Update animations with the provided interval. This will be executed by OnSlateApplicationPreTick().
	 * If the system is utilizing multithreading, it will also let the module update thread to resume its work.
	 * @param DeltaTime interval
	 */
	void UpdateAnimations(float DeltaTime);

private:
	
	//Multithreading
	
	/**
	 * The thread instance for the module update.
	 * It will let the module calculation never interrupt or bottleneck the main thread.
	 */
	TUniquePtr<FVoltModuleRunnable> ModuleUpdateThread;

public:
	
	/**
	 * Return whether the module update thread is running to update the modules assigned to the thread.
	 * Actions that is accesing the tracks or other non-atomic properties need to check this first to prevent race condition and etc.
	 * @return whether the module thread is running.
	 */
	const bool IsModuleUpdateThreadWorking() const;

public:
	
	/**
	 * Add animation manager to the module update thread.
	 * Please notice that this action is asynchronous.
	 * @param AnimationManager Animation manager to add.
	 */
	void AddAnimationManagerOnModuleUpdateThread(UVoltAnimationManager* AnimationManager);

	/**
	 * Remove animation manager from the module update thread.
	 * Please notice that this action is asynchronous.
	 * @param AnimationManager Animation manager to remove.
	 */
	void RemoveAnimationManagerOnModuleUpdateThread(UVoltAnimationManager* AnimationManager);

	const bool IsUtilizingMultithreading() const;

private:
	
	void SetUtilizingMultithreading(const bool bNewMultithreading);

private:

	void PopulateModuleUpdateThreadIfNeeded();

	void ReleaseModuleUpdateThread();
	
	/**
	 * Whether the system is now utilizing the multithreaded module update feature.
	 */
	bool bIsUtilizingMultiThread = true;


public:

	/**
	 * Get shared animation manager instance for the system.
	 * @return SharedAnimationManager object for the system.
	 */
	static UVoltAnimationManager* GetSharedAnimationManager();

public:

	void ReleaseSharedAnimationManager();

private:

	/**
	 * Shared animation manager of the system that let the users use without implementing animation manager explicitly.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UVoltAnimationManager> SharedAnimationManager;
	
public:

	/**
	 * Get the singleton instance of Volt Subsystem. 
	 * @return The singleton instance of Volt Subsystem.
	 */
	static UVoltSubsystem* Get();
	
	
};
