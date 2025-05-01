//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointActor.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JointSubsystem.generated.h"

class AJointActor;
/**
 * 
 */
class UJointManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJointBegin, AJointActor*, JointInstance, FGuid, JointGuid);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJointEnd, AJointActor*, JointInstance, FGuid, JointGuid);


UCLASS()
class JOINT_API UJointSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	//SDS2
	//TODO: Provide a more fancy way to start from a specific part of the node.
	//maybe provide a 'jump to' function to the Joint instance or manager.
	//Well, changing the start node in the manager before the StartJoint() execution will catch up the node to start from.

	//SDS2
	//Move all the functions that can be static to the blueprint function library.
	//If needed, implement the core part of each function on somewhere (I don't know, it might be in this class) and make a 'interface' for the blueprint for that function and let c++ use that function with static access.
	//

	
	/**
	 * Create a new Joint instance with provided argument. This function is a sort of helper to make it easy to create an instance, so you don't need to call it if you want.
	 * See the body of the function and use that to create your own Joint actor by your own if you need.
	 * Need to call StartJoint() manually to actually start off the Joint.
	 * @param WorldContextObject An object that this function will grab the world from. You can provide the subsystem itself. (In Blueprint, it will be automatically filled out.)
	 * @param JointAssetToPlay A Joint manager to use on the new Joint actor.
	 * @param OptionalJointInstanceSubclass A subclass of the Joint actor to create. If none specified, it will use AJointActor.
	 * @return A new Joint actor that has been newly created.
	 */
	UFUNCTION(BlueprintCallable, Category = "Joint", meta=(WorldContext="WorldContextObject"))
	static AJointActor* CreateJoint(
	 	UObject* WorldContextObject,
	 	UJointManager* JointAssetToPlay,
		TSubclassOf<AJointActor> OptionalJointInstanceSubclass
	);
	
	
	/**
	 * Find and return the Joint for the provided Joint ID.
	 * @param WorldContextObject An object that this function will grab the world from. You can provide the subsystem itself. (In Blueprint, it will be automatically filled out.)
	 * @param JointGuid Specific Guid to the Joint actor to find.
	 * @return Found Joint Actor instance. nullptr if not present.
	 */
	UFUNCTION(BlueprintCallable, Category = "Joint", meta=(WorldContext="WorldContextObject"))
	static class AJointActor* FindJoint(
		UObject* WorldContextObject,
		FGuid JointGuid
	);
	
	/**
	 * Get all the Joints in the world.
	 * @return An array of the Joints in the world.
	 */
	UFUNCTION(BlueprintCallable, Category = "Joint", meta=(WorldContext="WorldContextObject"))
	static TArray<class AJointActor*> GetAllJoints(
		UObject* WorldContextObject
	);

	
	/**
	 * Get singleton instance of the subsystem.
	 * @return UJointSubsystem instance.
	 */
	static UJointSubsystem* Get(UObject* WorldContextObject);

public:


	/**
	 * Get all the Joints that has been started on this frame prior to the execution of this function.
	 * Usually this function is useful when the world just has been started, and an object assigned the OnJointBeginDelegate on this subsystem but there is another object that already started a Joint prior to that.
	 * @return An array of the Joints that were started before this function executions.
	 */
	UFUNCTION(BlueprintCallable, Category = "Joint", meta=(WorldContext="WorldContextObject"))
	static TArray<FGuid> GetJointsGuidStartedOnThisFrame(
		UObject* WorldContextObject
	);

	/**
	 * Get all the Joints that has been started on this frame prior to the execution of this function.
	 * @return An array of the Joints that were ended before this function executions.
	 */
	UFUNCTION(BlueprintCallable, Category = "Joint", meta=(WorldContext="WorldContextObject"))
	static TArray<FGuid> GetJointsGuidEndedOnThisFrame(
		UObject* WorldContextObject
	);


private:
	
	/**
	 * Start off the Joint.
	 */
	void OnJointStarted(AJointActor* Actor);

	/**
	 * End the Joint.
	 */
	void OnJointEnded(AJointActor* Actor);

	friend AJointActor;

public:

	/**
	 * A Delegate that is executed whenever a Joint node has been started to play.
	 */
	UPROPERTY(BlueprintAssignable, VisibleAnywhere, Category="Joint Delegate")
	FOnJointBegin OnJointBeginDelegate;

	/**
	 * A Delegate that is executed whenever a Joint node has been ended to play.
	 */
	UPROPERTY(BlueprintAssignable, VisibleAnywhere, Category="Joint Delegate")
	FOnJointEnd OnJointEndDelegate;

public:
	/**
	 * Cache related properties for the missed Joint supports 
	 */
	UPROPERTY()
	TArray<FGuid> CachedJointBeginOnFrame;

	UPROPERTY()
	TArray<FGuid> CachedJointEndOnFrame;
	
	UPROPERTY()
	float CachedTime;

	UPROPERTY()
	float bCacheClearRequested = false;
	
private:

	void RequestFrameCachesClearOnNextFrame();
	
	void ClearCachedJointFrameData();

	void AddStartedJointToCaches(AJointActor* Actor);
	
	void AddEndedJointToCaches(AJointActor* Actor);
	
private:
	
	void BroadcastOnJointStarted(AJointActor* Actor, FGuid JointGuid);

	void BroadcastOnJointEnded(AJointActor* Actor, FGuid JointGuid);
	
public:
	//world ref related
	virtual UWorld* GetWorld() const override;

};
