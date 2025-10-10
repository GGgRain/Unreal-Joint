//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "JointActor.generated.h"


/**
 * An actor class that handles all the Joint actions.
 * Can be replicated through the network to provide a network-wise Joint playback.
 * some of the functionalities that has been marked to be played on the server side only will not be triggered when a client has the ownership of this actor.
 */

/**
 * Note to the SDS 1 users.
 * Joint actor doesn't hold any references to the Joint widget instances.
 * You must iterate the objects to find the widgets with this Joint actor.
 * Some of the Joint events already find the widgets in this way so go and check out how it accesses to the widgets.
 */

/**
 * Initially, this class is not designed to be override-able due to its action over the system. This is designed to be a sort of player for the system and many things work on this class specifically.
 * Since many things related to the action of the Joint can be created in the Joint manager and all the following assets.
 * But due to the GAS and any other modification with subclassing, We decided to open this class as override-able.
 */


class UJointManager;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJointStarted, AJointActor*, JointInstance);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJointBaseNodePlayed, AJointActor*, JointInstance,
                                             UJointNodeBase*, Node);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJointEnded, AJointActor*, JointInstance);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJointNodeRequestBeginPlay, AJointActor*, JointInstance,
                                             UJointNodeBase*, Node);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJointNodeRequestEndPlay, AJointActor*, JointInstance,
                                             UJointNodeBase*, Node);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJointNodeRequestPending, AJointActor*, JointInstance,
											 UJointNodeBase*, Node);


class UJointSubsystem;
class UJointNodeBase;


UCLASS()
class JOINT_API AJointActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AJointActor();

public:
	/**
	 * A component for the GAS implementation.
	 * A Joint instance actor can have gameplay ability by itself, and it can be used in multiple situations.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

public:
	UFUNCTION(BlueprintPure, Category="GAS")
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	/**
	 * Actual code where the node instance use to implement a default AbilitySystemComponent sub object.
	 * Override this function to implement the AbilitySystemComponent with the subclass you desire.
	 */
	virtual void ImplementAbilitySystemComponent();

public:
	/**
	 * The Guid of the Joint instance. Used to identify the instance on the world.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Joint")
	FGuid JointGuid;

	/**
	 * The Joint manager this Joint actor plays.
	 * It holds the copy of the original Joint manager.
	 * This actor has the Joint manager's ownership on runtime.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Joint")
	TObjectPtr<UJointManager> JointManager;

private:
#if WITH_EDITORONLY_DATA

	/**
	 * The original Joint manager of Joint manager this instance uses.
	 */
	UPROPERTY()
	TObjectPtr<UJointManager> OriginalJointManager;

	//Only debugger class in the editor module can access this value.
	friend class UJointDebugger;
	friend class FJointEdUtils;
	friend class FJointEditorToolkit;

#endif

private:
	/**
	 * The Joint node that this Joint actor is currently playing.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Joint")
	TObjectPtr<UJointNodeBase> PlayingJointNode;

public:

	/**
	 * List of known active nodes under the instance.
	 * It will be used on the clean-up action.
	 * This value is not replicated.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint", Transient)
	TArray<TObjectPtr<UJointNodeBase>> KnownActiveNodes;

public:
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	
	/**
	 * Get the node this Joint instance is currently playing.
	 * The reference to the PlayingJointNode itself will be replicated so it will not point different object from the server,
	 * but notice only particial data of this node will be replicated with the server.
	 * @return the node this Joint instance is currently playing.
	 */
	UFUNCTION(BlueprintPure, Category = "Joint")
	class UJointNodeBase* GetPlayingJointNode();

protected:
	
	/**
	 * Whether this Joint has been started or not.
	 * This value is not replicated.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint", Transient)
	bool bIsJointStarted = false;

	/**
	 * Whether this Joint has been ended or not.
	 * This value is not replicated.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint", Transient)
	bool bIsJointEnded = false;

private:
	/**
	 * Cached Joint nodes for the replication.
	 */
	UPROPERTY(Transient, Replicated, ReplicatedUsing = OnRep_CachedAllNodesForNetworking)
	TArray<TObjectPtr<UJointNodeBase>> CachedAllNodesForNetworking;

	UFUNCTION()
	void OnRep_CachedAllNodesForNetworking(const TArray<UJointNodeBase*>& PreviousCachedAllNodesForNetworking);


public:
	/**
	 * Set a new Joint manager this instance will use on its action.
	 * @param NewJointManager The new Joint manager to use.
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category="Joint Playback")
	void RequestSetJointManager(UJointManager* NewJointManager);

	virtual void RequestSetJointManager_Implementation(UJointManager* NewJointManager);
	
public:
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Joint Playback")
	virtual UJointManager* GetJointManager();

private:

	/**
	 * Callback event where we get the last node end to become pending. This will make the node to end.
	 */
	UFUNCTION()
	void OnNotifiedCurrentNodePending(UJointNodeBase* Node);

	
	/**
	 * Callback event where we get the last node end to end. This will provoke PlayNextNode event.
	 */
	UFUNCTION()
	void OnNotifiedCurrentNodeEnded(UJointNodeBase* Node);

private:

	/**
	 * Release current node's reference and possible delegate binding with the node.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void ReleaseEventsFromPlayingJointNode();

	void ReleaseEventsFromPlayingJointNode_Implementation();
	
	UFUNCTION(NetMulticast, Reliable)
	void BindEventsOnPlayingJointNode();

	void BindEventsOnPlayingJointNode_Implementation();

	UFUNCTION(NetMulticast, Reliable)
	void SetPlayingJointNode(UJointNodeBase* NewPlayingJointNode);
	
	void SetPlayingJointNode_Implementation(UJointNodeBase* NewPlayingJointNode);

private:

	/**
	 * Release current node's reference and possible delegate binding with the node.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void BeginPlayPlayingJointNode();

	void BeginPlayPlayingJointNode_Implementation();
	
	UFUNCTION(NetMulticast, Reliable)
	void EndPlayPlayingJointNode();

	void EndPlayPlayingJointNode_Implementation();

private:
	
	UFUNCTION()
	UJointNodeBase* PickUpNewNodeFrom(const TArray<UJointNodeBase*>& TestTargetNodes);

public:
	UFUNCTION(BlueprintPure, Category="Joint Playback")
	bool IsJointStarted();

	UFUNCTION(BlueprintPure, Category="Joint Playback")
	bool IsJointEnded();

public:
	/**
	 * Start the Joint.
	 * Prepare the variables that must be handled with the authority and replicate the variables if we need.
	 * After that, actually node related action will begin at ProcessStartJoint().
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Joint Playback")
	void StartJoint();

	void StartJoint_Implementation();

	/**
	 * End the Joint.
	 * Prepare the variables that must be handled with the authority and replicate the variables if we need.
	 * After that, actually node related action will begin at ProcessEndJoint().
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Joint Playback")
	void EndJoint();

	void EndJoint_Implementation();
	
	void ForceEndAllKnownActiveNodes();

	/**
	 * Discard the Joint actor instance with needed actions along with it.
	 * Before it gets destroyed, it will simultaneously end all the nodes that are still active. (not ended)
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category="Joint Playback")
	void DiscardJoint();

	void DiscardJoint_Implementation();

	/**
	 * Destroy the Joint actor instance.
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category="Joint Playback")
	void DestroyJoint();

	void DestroyJoint_Implementation();
	
	void DestroyInstance();

private:
	/**
	 * Multicasted implementation of StartJoint().
	 * Handle the replicated variables and actually start off the Joint 
	 */
	UFUNCTION(NetMulticast, Reliable, Category="Joint Playback")
	void ProcessStartJoint();

	void ProcessStartJoint_Implementation();

	/**
	 * Multicasted implementation of EndJoint().
	 * Handle the replicated variables and actually end the Joint 
	 */
	UFUNCTION(NetMulticast, Reliable, Category="Joint Playback")
	void ProcessEndJoint();

	void ProcessEndJoint_Implementation();

	UFUNCTION()
	void MarkAsStarted();

	UFUNCTION()
	void MarkAsEnded();


	/**
	 * Notify the beginning of the Joint.
	 */
	void NotifyStartJoint();

	/**
	 * Notify the end of the Joint.
	 */
	void NotifyEndJoint();

	friend UJointSubsystem;

public:
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Joint Playback")
	void PlayNextNode();

	void PlayNextNode_Implementation();

	UFUNCTION(NetMulticast, Reliable)
	void ProcessPlayNextNode();

	void ProcessPlayNextNode_Implementation();

public:
	
	UFUNCTION(BlueprintCallable, Category="Joint Playback")
	void RequestNodeBeginPlay(UJointNodeBase* InNode);
	
	UFUNCTION(BlueprintCallable, Category="Joint Playback")
	void RequestNodeEndPlay(UJointNodeBase* InNode);

	UFUNCTION(BlueprintCallable, Category="Joint Playback")
	void RequestMarkNodeAsPending(UJointNodeBase* InNode);

	UFUNCTION(BlueprintCallable, Category="Joint Playback")
	void RequestReloadNode(UJointNodeBase* InNode, const bool bPropagateToSubNodes = true, const bool bAllowPropagationEvenParentFails = true);

public:

	void NotifyNodeBeginPlay(UJointNodeBase* InNode);

	void NotifyNodeEndPlay(UJointNodeBase* InNode);
	
	void NotifyNodePending(UJointNodeBase* InNode);

public:
	void BeginManagerFragments();

	void EndManagerFragments();
public:
	/**
	 * Networking of the Joint instance actor.
	 * Joint instance will replicate only following 3 properties by itself:
	 * JointGuid, JointManager, PlayingJointNode.
	 *
	 * The most important one is the Playing Joint node. Instance doesn't replicate the begin play event of the other nodes, but it replicates the change on the base node and its play action.
	 */

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

private:

	//Don't call this in out of initialization.
	void CacheNodesForNetworking();

public:

	void AddNodeForNetworking(class UJointNodeBase* InNode);
	
	void RemoveNodeForNetworking(class UJointNodeBase* InNode);

private:
#if WITH_EDITOR

	/**
	 * Check whether the debugger must stop the process on the begin play of certain node.
	 * This will also return false when there is already an active debugging session for the instance.
	 * 
	 * The debugger will store all the nodes that has been checked with this function (requested to be played while the debugging was activated), and trigger it again when the debugging session has been released and the simulation has been resumed.
	 * 
	 * @param InNode Node to provide and check whether the debugger must start debugging with.
	 * @return Whether the debugging has been started.
	 */
	bool CheckDebuggerCanProceed(UJointNodeBase* InNode);

#endif

public:
	UPROPERTY(BlueprintAssignable, Category = "Joint", DisplayName="On Joint Started")
	FOnJointStarted OnJointStartedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Joint", DisplayName="On Joint Ended")
	FOnJointEnded OnJointEndedDelegate;

	//Executed when a new base node is played. Doesn't be executed on the sub node's play action.
	UPROPERTY(BlueprintAssignable, Category = "Joint", DisplayName="On Joint Base Node Played")
	FOnJointBaseNodePlayed OnJointBaseNodePlayedDelegate;

public:
	//Executed when a new node has been requested to be begin played.
	UPROPERTY(BlueprintAssignable, Category = "Joint", DisplayName="On Joint Node BeginPlayed")
	FOnJointNodeRequestBeginPlay OnJointNodeBeginPlayDelegate;

	//Executed when a new node has been requested to be end played.
	UPROPERTY(BlueprintAssignable, Category = "Joint", DisplayName="On Joint Node End Play")
	FOnJointNodeRequestEndPlay OnJointNodeEndPlayDelegate;

	//Executed when a new node has been requested to be pending.
	UPROPERTY(BlueprintAssignable, Category = "Joint", DisplayName="On Joint Node Pending")
	FOnJointNodeRequestPending OnJointNodePendingDelegate;

public:
	
};
