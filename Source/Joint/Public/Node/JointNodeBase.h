//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SharedType/JointSharedTypes.h"
#include "SharedType/JointBuildPreset.h"
#include "SharedType/JointEdSharedTypes.h"

#include "BlueprintUtilities.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"

#include "JointNodeBase.generated.h"


class UJointManager;
class FReply;
class UJointFragment;

/**
 * Joint node is a most basic form of the node that can be placed on the Joint manager graph.
 *
 * Joint nodes and fragments are the amalgam of the executable action (function) and storage.
 * Each node can contain the data you need for the graph and can be accessed from the other nodes.
 * And also can be used to implement some special action on the graph that will be executed when this node started to being played.
 * Notice that the nodes are executed only when the Joint flow met the node. but at the same time, you can still access any nodes in the graph if you want and use its variables.
 * 
 * You can override this class (more specifically UJointFragment) to represent some additional features for your graph or variables for the graph.
 */

/**
 * WHEN IMPLEMENTING A NEW NODE FOR THE PLUGIN, DO NOT OVERRIDE FROM DERIVED NODES.
 * ONLY USE UJointFragment, UJointNodeBase.
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJointNodeBegin, UJointNodeBase*, Node);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJointNodeEnd, UJointNodeBase*, Node);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJointNodeMarkedAsPending, UJointNodeBase*, Node);

DECLARE_DYNAMIC_DELEGATE_RetVal(TArray<FJointEdPinData>, FOnRequestJointPinData);

UCLASS(Abstract, Blueprintable, BlueprintType, Category = "Joint")
class JOINT_API UJointNodeBase : public UObject, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	
	UJointNodeBase();

public:
	/**
	 * The Guid of the node. Use this value to find specific node on the graph.
	 */
	UPROPERTY(AdvancedDisplay, BlueprintReadOnly, VisibleAnywhere, Category = "Node Base Data")
	FGuid NodeGuid;

public:
	/**
	 * The parent node of this node.
	 */
	UPROPERTY(AdvancedDisplay, BlueprintReadOnly, VisibleAnywhere, Category = "Node Base Data")
	TObjectPtr<UJointNodeBase> ParentNode = nullptr;


	/**
	 * Child nodes that this node contains.
	 */
	UPROPERTY(AdvancedDisplay, BlueprintReadOnly, VisibleAnywhere, Category = "Node Base Data")
	TArray<TObjectPtr<UJointNodeBase>> SubNodes;

public:
	/**
	 * The gameplay tags that this node has.
	 *
	 * Use this property to fill out some additional data for the node.
	 * This also can be used to find this node on the graph.
	 * 
	 * This is useful when you want to put some special nodes on the graph that must be accessed directly from the outside.
	 * (ex, when you want to get the node that contains the array of player controller than can interact with the fragment with widgets or etc...)
	 *
	 * NOTE: We don't provide a feature that can iterate the whole Joint manager to find a specific node because it can harm the performance really badly, and also it doesn't quite fit well with the system itself.
	 * The best expected use-case will be using it on the Joint manager fragments and marking them with their roles on the graph, And basically that is what we intended the manager fragment to be.
	 * 
	 * If you really need to find a specific node on the random location on the graph, then try to make a manager fragment that hold the Joint node pointer to that node and utilize that if possible.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Tag")
	FGameplayTagContainer NodeTags;

public:
	/**
	 * A delegate that will be broadcast when this node has begun and before executing OnNodeBeginPlay()
	 */
	UPROPERTY(BlueprintAssignable, Category = "Joint")
	FOnJointNodeBegin OnJointNodeBeginDelegate;

	/**
	 * A delegate that will be broadcast when this node has ended and before executing OnNodeEndPlay()
	 */
	UPROPERTY(BlueprintAssignable, Category = "Joint")
	FOnJointNodeEnd OnJointNodeEndDelegate;

	/**
	 * A delegate that will be broadcast when this node has become pending.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Joint")
	FOnJointNodeMarkedAsPending OnJointNodeMarkedAsPendingDelegate;

public:
	//Check if this node is a sub node of another node. It will return true if the node is manager fragment.
	UFUNCTION(BlueprintPure, Category = "SubNode")
	bool IsSubNode();

	//Return the parent node if this node is sub node. Return nullptr if this node doesn't have parent.
	UFUNCTION(BlueprintPure, Category = "SubNode")
	UJointNodeBase* GetParentNode() const;

	/**
	 * Return the parent-most node.(base node in most case, except for the manager fragments. they return themselves.)
	 * Return self if this node is the base node.
	 */
	UFUNCTION(BlueprintPure, Category = "SubNode")
	UJointNodeBase* GetParentmostNode();


	/**
	 * Get all the parent nodes on the hierarchy. Closer ones will come first.
	 * Joint 2.4.0 : It returns ancestor nodes on the subtree of the base node.
	 * @return An array of found parent nodes.
	 */
	UFUNCTION(BlueprintPure, Category = "SubNode")
	TArray<UJointNodeBase*> GetParentNodesOnHierarchy() const;

public:
	/**
	 * Return the Joint manager that has this node instance.
	 */
	UFUNCTION(BlueprintPure, Category = "Node")
	UJointManager* GetJointManager() const;

public:
	/**
	 * Return the guid of the node.
	 */
	UFUNCTION(BlueprintPure, Category = "Node")
	const FGuid& GetNodeGuid() const;

public:
	//Begin of IGameplayTagAssetInterface implementation
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	//End of IGameplayTagAssetInterface implementation

public:
	/**
	 * Find a fragment by the provided tag while iterating through the lower hierarchy.
	 * @param InNodeTag The tag to search.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	UJointFragment* FindFragmentWithTagOnLowerHierarchy(FGameplayTag InNodeTag, const bool bExact = false);

	/**
	 * Find fragments by the provided tag while iterating through the lower hierarchy.
	 * @param InNodeTag The tag to search.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<UJointFragment*> FindFragmentsWithTagOnLowerHierarchy(FGameplayTag InNodeTag, const bool bExact = false);

	/**
	 * Find a fragment by the provided tags while iterating through the lower hierarchy.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	UJointFragment* FindFragmentWithAnyTagsOnLowerHierarchy(FGameplayTagContainer InNodeTagContainer,
	                                                           const bool bExact = false);

	/**
	 * Find fragments by the provided tags while iterating through the lower hierarchy.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<UJointFragment*> FindFragmentsWithAnyTagsOnLowerHierarchy(FGameplayTagContainer InNodeTagContainer,
	                                                                    const bool bExact = false);

	/**
	 * Find a fragment by the provided tags while iterating through the lower hierarchy.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	UJointFragment* FindFragmentWithAllTagsOnLowerHierarchy(FGameplayTagContainer InNodeTagContainer,
	                                                           const bool bExact = false);

	/**
	 * Find fragments by the provided tags while iterating through the lower hierarchy.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has all of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<UJointFragment*> FindFragmentsWithAllTagsOnLowerHierarchy(FGameplayTagContainer InNodeTagContainer,
	                                                                    const bool bExact = false);


	/**
	 * Find a fragment by the provided class while iterating through the lower hierarchy.
	 * @param InNodeGuid The fragment Guid to search.
	 * @return Found fragment for the class.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	UJointFragment* FindFragmentWithGuidOnLowerHierarchy(FGuid InNodeGuid);

	/**
	 * Find a fragment by the provided class while iterating through the lower hierarchy.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment", meta=(DeterminesOutputType="FragmentClass"))
	UJointFragment* FindFragmentByClassOnLowerHierarchy(TSubclassOf<UJointFragment> FragmentClass);

	/**
	 * Find all fragments by the provided class while iterating through the lower hierarchy.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment", meta=(DeterminesOutputType="FragmentClass"))
	TArray<UJointFragment*> FindFragmentsByClassOnLowerHierarchy(TSubclassOf<UJointFragment> FragmentClass);

	/**
	 * Get all fragments attached on this node while iterating through the lower hierarchy.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<UJointFragment*> GetAllFragmentsOnLowerHierarchy();

public:
	/**
	 * Find a fragment by the provided tag.
	 * @param InNodeTag The tag to search.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	UJointFragment* FindFragmentWithTag(FGameplayTag InNodeTag, const bool bExact = false);

	/**
	 * Find fragments by the provided tag.
	 * @param InNodeTag The tag to search.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<UJointFragment*> FindFragmentsWithTag(FGameplayTag InNodeTag, const bool bExact = false);

	/**
	 * Find a fragment by the provided tags.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	UJointFragment* FindFragmentWithAnyTags(FGameplayTagContainer InNodeTagContainer,
	                                           const bool bExact = false);

	/**
	 * Find fragments by the provided tags.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<UJointFragment*> FindFragmentsWithAnyTags(FGameplayTagContainer InNodeTagContainer,
	                                                    const bool bExact = false);

	/**
	 * Find a fragment by the provided tags.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	UJointFragment* FindFragmentWithAllTags(FGameplayTagContainer InNodeTagContainer,
	                                           const bool bExact = false);

	/**
	 * Find fragments by the provided tags.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has all of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<UJointFragment*> FindFragmentsWithAllTags(FGameplayTagContainer InNodeTagContainer,
	                                                    const bool bExact = false);

public:
	/**
	 * Find a fragment with a given node Guid. It only searches the children nodes that are directly attached to this node.
	 * Notice it will not search through the sub node's sub node.
	 * @param InNodeGuid The fragment Guid to search.
	 * @return Found fragment for the class.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	UJointFragment* FindFragmentWithGuid(FGuid InNodeGuid) const;

	/**
	 * Find a fragment with by class. It only searches the children nodes that are directly attached to this node.
	 * Notice it will not search through the sub node's sub node.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment", meta=(DeterminesOutputType="FragmentClass"))
	UJointFragment* FindFragmentByClass(TSubclassOf<UJointFragment> FragmentClass) const;

	/**
	 * Find all fragments with its class. It only searches the children nodes that are directly attached to this node.
	 * Notice it will not search through the sub node's sub node.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment", meta=(DeterminesOutputType="FragmentClass"))
	TArray<UJointFragment*> FindFragmentsByClass(TSubclassOf<UJointFragment> FragmentClass) const;

	/**
	 * Get all fragments attached on this node. It only returns the children nodes that are directly attached to this node.
	 * Notice it will not search through the sub node's sub node.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<UJointFragment*> GetAllFragments() const;

public:
	/**
	 * Return the next nodes to play if any present when this node finished playing. It is useful on expressing branching features.
	 * Override this function to implement needed actions for your node.
	 *
	 * By default, It will iterate and check sub node's SelectNextNodes() results and return it when if it is not empty.
	 * Notice if all the nodes on one base node returns nothing, the Joint will be finished.
	 *
	 * Note: The reason we provides array instead of the node instance is because we wanted to open the possible implementation using array in the logic.
	 * 
	 * @param InHostingJointInstance The Joint instance that is hosting this node.
	 * @return An Array of the nodes to play on the next run. Only the first node will be selected.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Node")
	TArray<UJointNodeBase*> SelectNextNodes(class AJointActor* InHostingJointInstance);

	virtual TArray<UJointNodeBase*> SelectNextNodes_Implementation(class AJointActor* InHostingJointInstance);

private:


	/**
	 * Reload the node's state to make it able to be played again.
	 * 
	 * Joint 2.10.0 : Now this function is marked to be public and can be called from the outside.
	 * Please notice that it is not able to call it directly from the other class object.
	 * Use the Joint instance (AJointActor) for this node instead to call the other node's being play action, or execute RequestReloadNode() instead.
	 */
	void ReloadNode();
	
	/**
	 * Play this node's playback. Please notice that it is not able to call it directly from the other class object.
	 * Use the Joint instance (AJointActor) for this node instead to call the other node's being play action, or execute RequestNodeBeginPlay() instead.
	 * Joint 2.12: Deprecated. We're using ProcessPreNodeBeginPlay() and ProcessPostNodeBeginPlay() instead.
	 */
	//void NodeBeginPlay();

	/**
	 * End this node's playback. Please notice that it is not able to call it directly from the other class object.
	 * Use the Joint instance (AJointActor) for this node instead to call the other node's end play action, or execute RequestNodeEndPlay() instead.
	 * Joint 2.12: Deprecated. We're using ProcessPreNodeEndPlay() and ProcessPostNodeEndPlay() instead.
	 */
	//void NodeEndPlay();

	/**
	 * Mark this node as pending. Please notice that it is not able to call it directly from the other class object.
	 * Use the Joint instance (AJointActor) for this node instead to call the other node's end play action, or execute RequestMarkNodeAsPending() instead.
	 * Joint 2.12: Deprecated. We're using ProcessPreMarkNodePending() and ProcessPostMarkNodePending() instead.
	 */
	//void MarkNodePending();
	
private:

	void ProcessPreNodeBeginPlay();
	void ProcessPostNodeBeginPlay();
	void ProcessPreNodeEndPlay();
	void ProcessPostNodeEndPlay();
	void ProcessPreMarkNodePending();
	void ProcessPostMarkNodePending();
	
private:
	
	/**
	 * Joint 2.12.0 : We decided to deprecate NodeBeginPlay / NodeEndPlay / MarkNodePending functions because we want to separate Pre / Post actions' entrance.
	 * This created an issue that the system don't know if the pre - post action has been executed with pair, or maybe some cheaters are trying to call only one of them.
	 * So we're storing the execution id for the actions to verify the matching end play / pending action.
	 * TODO: Or should we just separate bIsNodeBegunPlay into two variables for Pre / Post action, and verify them instead?
	 */ 
	
	UPROPERTY()
	FGuid BeginPlayExecutionId;
	



	friend AJointActor;

public:

	UFUNCTION(BlueprintPure, Category = "Lifecycle")
	const bool& CanReloadNode() const;

public:

	/**
	 * Whether this node can be reloaded.
	 * We intended this to be used for the nodes that are not intended to be played multiple times, especially for security reasons.
	 * So make sure to understand the possible issues and concerns before handling this variable.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lifecycle")
	bool bCanReloadNode = true;

public:
	/**
	 * Play this node's sub nodes' playback.
	 */
	UFUNCTION(BlueprintCallable, Category = "Node")
	void SubNodesBeginPlay();

	/**
	 * End this node's sub nodes' playback.
	 */
	UFUNCTION(BlueprintCallable, Category = "Node")
	void SubNodesEndPlay();

public:
	/**
	 * Request the hosting Joint instance to play this node's playback.
	 */
	UFUNCTION(BlueprintCallable, Category = "Node")
	void RequestNodeBeginPlay(AJointActor* InHostingJointInstance);
	
	/**
	 * Request the hosting Joint instance to end this node's playback.
	 */
	UFUNCTION(BlueprintCallable, Category = "Node")
	void RequestNodeEndPlay();

	/**
	 * Reload the node's state to make it able to be played again.
	 * 
	 * Joint 2.10.0 : Now this function is marked to be public and can be called from the outside.
	 * But please understand that this function is not intended to be used in the normal flow of the Joint.
	 */
	UFUNCTION(BlueprintCallable, Category = "Node")
	void RequestReloadNode(const bool bPropagateToSubNodes = true);

protected:

	/**
	 * Pre-defined action of this node when it has begun play.
	 * This event will be executed before other events and delegate.
	 * Override this function to implement the action you desire.
	 * By default, It does nothing.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Node")
	void PreNodeBeginPlay();

	/**
	 * Note: If you want to execute Super::PreNodeBeginPlay() in C++, use Super::PreNodeBeginPlay_Implementation() instead.
	 */
	virtual void PreNodeBeginPlay_Implementation();
	
	/**
	 * Pre-defined action of this node when it has begun play. If you need to reset the variables for the node action, you can put those logic here. (Notice you also have to take care about the case when the node hit twice in the Joint by recursive flow.)
	 * Override this function to implement the action you desire.
	 * 
	 * By the class default, It will execute SubNodesBeginPlay() if this node has any sub node, else then it will end play itself.(To prevent this, you must override this function and implement the features you want for this node type.)
	 * Or it will just execute K2_OnNodeBeginPlay if this class is derived from Blueprint.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Node")
	void PostNodeBeginPlay();

	/**
	 * Note: If you want to execute Super::PostNodeBeginPlay_Implementation() in C++, use Super::PostNodeBeginPlay_Implementation() instead.
	 */
	virtual void PostNodeBeginPlay_Implementation();

	/**
	 * Pre-defined action of this node when it finished end playing.
	 * This event will be executed before other events and delegate.
	 * Override this function to implement the action you desire.
	 * By default, it will do nothing.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Node")
	void PreNodeEndPlay();

	/**
	 * Note: If you want to execute Super::PreNodeEndPlay_Implementation() in C++, use Super::PreNodeEndPlay_Implementation() instead.
	 */
	virtual void PreNodeEndPlay_Implementation();
	

	/**
	 * Pre-defined action of this node when it finished end playing.
	 * Override this function to implement the action you desire.
	 * By the class default, it will execute K2_OnNodeEndPlay if this class is derived from Blueprint otherwise call SubNodesEndPlay().
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Node")
	void PostNodeEndPlay();

	/**
	 * Note: If you want to execute Super::PostNodeEndPlay_Implementation() in C++, use Super::PostNodeEndPlay_Implementation() instead.
	 */
	virtual void PostNodeEndPlay_Implementation();

	/**
	 * Pre-defined action of this node when it becomes Pending.
	 * Override this function to implement the action you desire.
	 * By default, It does nothing.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Node")
	void PreNodeMarkedAsPending();

	/**
	 * Note: If you want to execute Super::PreNodeMarkedAsPending_Implementation() in C++, use Super::PreNodeMarkedAsPending_Implementation() instead.
	 */
	virtual void PreNodeMarkedAsPending_Implementation();

	/**
	 * Pre-defined action of this node when it becomes Pending.
	 * This event will be executed before other events and delegate.
	 * Override this function to implement the action you desire.
	 * By default, It does nothing.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Node")
	void PostNodeMarkedAsPending();

	/**
	 * Note: If you want to execute Super::PostNodeMarkedAsPending_Implementation() in C++, use Super::PostNodeMarkedAsPending_Implementation() instead.
	 */
	virtual void PostNodeMarkedAsPending_Implementation();

public:
	/**
	 * Mark this node pending.
	 */
	UFUNCTION(BlueprintCallable, Category = "Node")
	void MarkNodePendingByForce();

	/**
	 * Mark this node pending if the CheckCanMarkNodeAsPending() is true at the moment.
	 * This function will be executed whenever any of the sub nodes became pending.
	 */
	UFUNCTION(BlueprintCallable, Category = "Node")
	void MarkNodePendingIfNeeded();

	/**
	 * Check whether this node can be pending or not. This function will be used in the MarkNodePendingIfNeeded().
	 * By default, the node will be pending when all the sub nodes are pending.
	 * You can override this function to prevent the default action and define own way to mark this node as pending.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Node")
	bool CheckCanMarkNodeAsPending();

	virtual bool CheckCanMarkNodeAsPending_Implementation();

public:
	/**
	 * Check if the node is begun play
	 */
	UFUNCTION(BlueprintPure, Category = "Node")
	bool IsNodeBegunPlay() const;

	/**
	 * Check if the node is awaiting other node's deactivation.
	 */
	UFUNCTION(BlueprintPure, Category = "Node")
	bool IsNodeEndedPlay() const;

	/**
	 * Check if the node is on pending
	 */
	UFUNCTION(BlueprintPure, Category = "Node")
	bool IsNodePending() const;

	/**
	 * Check if the node is active.
	 * Return true when IsNodeBegunPlay is true, and IsNodeEndedPlay is false.
	 */
	UFUNCTION(BlueprintPure, Category = "Node")
	bool IsNodeActive() const;

private:
	/**
	 * True if the node is begun played.
	 */
	UPROPERTY(BlueprintGetter=IsNodeBegunPlay, Category = "Data", Transient)
	bool bIsNodeBegunPlay = false;

	/**
	 * True if the node is begun played.
	 */
	UPROPERTY(BlueprintGetter=IsNodeEndedPlay, Category = "Data", Transient)
	bool bIsNodeEndedPlay = false;

	/**
	 * True if the node is awaiting other node's deactivation.
	 * 
	 * Each Joint node will be automatically marked as pending when every sub nodes are marked as pending.
	 * Joint actor will finish playing the base node when the base node is on pending state. (including end played nodes, end played nodes internally marked as pending.)
	 * 
	 * Pending state can arise even when the node is still active.
	 * This will tell the Joint manager that this node does not want to hold the Joint playback and is ready to be deactivated at any moment when every other nodes are on pending state.
	 */
	UPROPERTY(BlueprintGetter=IsNodePending, Category = "Data", Transient)
	bool bIsNodePending = false;


	/**
	 * Dev note for the basic concept for the life cycle and pending state.
	 * (it is translated via chatgpt, so it might not be accurate. Pardon me, I didn't have that much time to write it down manually.)
	 * 
	 * BeginPlay literally means starting the playback of a node, activating its activity, and executing its functionality.
	 * The default BeginPlay of a Joint node propagates BeginPlay to its child sub-nodes.
	 * Therefore, when the top-level node starts playing, its child sub-nodes will play together unless they implement different functionalities.
	 * 
	 * EndPlay literally means ending the playback of a node, deactivating its activity.
	 * Similarly, the default EndPlay of a Joint node propagates EndPlay to its child sub-nodes.
	 * When the playback of the top-level node ends, its child sub-nodes must also end playback together, and their activities are turned off.
	 * Unlike BeginPlay, there's a slight difference with EndPlay because once the playback of the top-level node ends, the Joint proceeds to the next node anyway. The functionalities of a sub-node should strictly operate within that node.
	 * 
	 * These functions are helpful for controlling the node externally or for checking the node's lifecycle. If EndPlay or BeginPlay is called externally, it forcibly plays or stops the child nodes, which greatly aids in external node control.
	 * The synchronization between the execution of parent nodes and child nodes is crucial because, in most cases, child nodes depend on parent nodes or complement the functionality of parent nodes. Hence, it's preferred to follow the same lifecycle.
	 * However, when ending the playback of a node internally, the following points should be considered:
	 * 1. The parent node should not end if the child nodes still have pending tasks.
	 * 2. The parent node can only end when all child nodes are ready to be ended.
	 * 
	 * To address these rules, PendingEndPlay was introduced. If the PendingEndPlay flag is active, it indicates that the child nodes are ready to end their tasks at any time.
	 * 
	 * In the Joint Node's default class, MarkNodePendingIfNeeded, marks itself as PendingEndPlay when all child nodes are marked as PendingEndPlay.
	 * When a child node is marked as PendingEndPlay, it calls this on its parent node.
	 * Unless overridden to implement different functionalities, the parent node will also be marked as PendingEndPlay when all its sub-nodes are marked as PendingEndPlay.
	 * When the top-level sub-node is marked as PendingEndPlay, the Joint node enters the EndPlay state.
	 * In essence, PendingEndPlay is a flag added for internal playback control, unlike EndPlay and BeginPlay, which are for external control.
	 */

private:
	
	UPROPERTY(transient)
	mutable TWeakObjectPtr<AJointActor> HostingJointInstance;

public:
	
	void SetHostingJointInstance(const TWeakObjectPtr<AJointActor>& InHostingJointInstance);

	/**
	 * Get the Joint instance that is hosting this node.
	 * This is safe to call even when the node is not being played; if the node is originated from a runtime instance of Joint Manager that has Joint Actor instance as its outer, then it will return the Joint Actor instance. (a little bit slower than the cached one.)
	 * TODO: Is this safe?
	 * @return The Joint instance that is hosting this node.
	 */
	UFUNCTION(BlueprintPure, Category = "Joint")
	AJointActor* GetHostingJointInstance() const;

public:
	//Grab the world object reference from the Joint instance and return it.
	virtual UWorld* GetWorld() const override;

private:
	//Networking & RPC related.

	/**
	 * Whether this node replicates through the network. This variable must be true to replicate any property with this node.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Networking")
	bool bReplicates = false;
	
	/**
	 * Joint 2.11.0
	 * Whether to use the player controller's function callspace for the RPC function calls.
	 * If you set this to true, then it will use the Player controller's function callspace instead of the Joint actor's one, allowing the RPC functions to be executed on the player controller side.
	 * Which means that you don't need to implement some 'workaround' functions on the controller side to execute the RPC functions on the node instance.
	 *
	 * This is not enabled by default, because it can cause some unexpected issues we haven't foreseen yet. So please use this option with caution.
	 * (If you experience any issues regarding this option, please report them to us so we can fix them asap.)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Networking|Advanced (Experimental)")
	bool bUsePlayerControllerAsRPCFunctionCallspace = false;
	
public:
	
	/**
	 * Set whether to replicate the node.
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category="Networking")
	void SetReplicates(const bool bNewReplicates);

	void SetReplicates_Implementation(const bool bNewReplicates);


	/**
	 * Set whether to replicate the node.
	 */
	UFUNCTION(BlueprintPure, Category="Networking")
	const bool GetReplicates() const;
	
	
	virtual bool IsSupportedForNetworking() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) override;

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

public:
#if WITH_EDITOR

	virtual void PreEditChange(class FProperty* PropertyAboutToChange) override;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void NotifyParentGraph();

	virtual void PostEditImport() override;

#endif

public:

#if WITH_EDITORONLY_DATA
	
	/**
	 * Build Preset of the node instance.
	 * Whether this node will be excluded or included on the packaged build on specific build condition will be decided by the preset setting.
	 * Please notice that if parent node get excluded, then sub nodes will be excluded as well for that build condition. 
	 */
	UPROPERTY(EditAnywhere, Category="Bulid Target")
	TSoftObjectPtr<class UJointBuildPreset> BuildPreset = nullptr;

	UPROPERTY(Transient)
	FName LastSeenBuildTarget;

#endif
	
	/**
	 * Get the build preset.
	 * This will return nullptr everytime on the packaged build.
	 */
	UFUNCTION(BlueprintCallable, Category="Bulid Target")
	TSoftObjectPtr<class UJointBuildPreset> GetBuildPreset();

	/**
	 * Set the build target preset with the provided build preset.
	 * This will not do anything on the packaged build.
	 */
	UFUNCTION(BlueprintCallable, Category="Bulid Target")
	void SetBuildPreset(TSoftObjectPtr<class UJointBuildPreset> NewBuildPreset);

#if WITH_EDITORONLY_DATA
	
	/**
	 * Joint 2.9 : We moved all the properties that are related to the editor node on here. To update it properly, you must follow this:
	 * 1. Visit Joint Management, and press "Update BP fragments' Node Editor Setting With Last Value" button, and save your projects.
	 * 2. For the C++ nodes, you must update the properties manually. Just put "EdNodeSetting->" for your properties related to the editor node settings.
	 *
	 * Joint 2.10 : Also check out the Editor Node's DefaultEdNodeSetting when you want to specify editor node side default values for your cosmetic properties. (That's a fallback property of this one.)
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor", meta=(DisplayName="Editor Node", ShowOnlyInnerProperties))
	FJointEdNodeSetting EdNodeSetting;
	
#endif

public:

#if WITH_EDITORONLY_DATA

	//DON'T USE THESE. USE JointEdNodeSetting INSTEAD.
	
	UPROPERTY()
	bool bUseSpecifiedGraphNodeBodyColor = false;
	
	UPROPERTY()
	FLinearColor NodeBodyColor;
	
	UPROPERTY()
	FSlateBrush IconicNodeImageBrush;
	
	UPROPERTY()
	bool bAllowDisplayClassFriendlyNameText = true;
	
	UPROPERTY()
	bool bUseSimplifiedDisplayClassFriendlyNameText = false;
	
	UPROPERTY()
	FText SimplifiedClassFriendlyNameText;
	
	UPROPERTY()
	TEnumAsByte<EJointEdSlateDetailLevel::Type> DefaultEdSlateDetailLevel = EJointEdSlateDetailLevel::SlateDetailLevel_Maximum;
	
	UPROPERTY()
	TArray<FJointGraphNodePropertyData> PropertyDataForSimpleDisplayOnGraphNode;
	
	UPROPERTY()
	bool bAllowNodeInstancePinControl = false;

#endif

public:
	
	//Node Instance Pin Editing

	/**
	 * A function that will be triggered whenever a connection of the node has been changed on the graph.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Editor|Node Connection")
	void OnPinConnectionChanged(const TMap<FJointEdPinData,FJointNodes>& PinToConnections);

	virtual void OnPinConnectionChanged_Implementation(const TMap<FJointEdPinData,FJointNodes>& PinToConnections);
	
	/**
     * A function that will be triggered whenever the pin data need to be updated.
	 * Usually it will be whenever a property has been changed or on the graph connection events.
	 * @param PinData Pin Data array reference.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Editor|Pin Data Modification")
	void OnUpdatePinData(TArray<FJointEdPinData>& PinData);

	virtual void OnUpdatePinData_Implementation(TArray<FJointEdPinData>& PinData);

	/**
	 * Get the pin data from the graph node.
	 */
	UFUNCTION(BlueprintPure, Category="Editor|Pin Data Modification")
	TArray<FJointEdPinData> GetPinData() const;

public:

	//Node Instance Node Attachment
	
	/**
	 * A function that will be triggered whenever this node is being attached to this node.
	 * Return response to allow or disallow the attachment.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Editor|Node Attachment")
	const FJointEdPinConnectionResponse CanAttachThisAtParentNode(const UObject* InParentNode) const; 

	virtual const FJointEdPinConnectionResponse CanAttachThisAtParentNode_Implementation(const UObject* InParentNode) const;
	
	/**
	 * A function that will be triggered whenever this node is being attached to this node.
	 * Return response to allow or disallow the attachment.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Editor|Node Attachment")
	const FJointEdPinConnectionResponse CanAttachSubNodeOnThis(const UObject* InParentNode) const; 

	virtual const FJointEdPinConnectionResponse CanAttachSubNodeOnThis_Implementation(const UObject* InParentNode) const; 

	
public:

#if WITH_EDITOR
	bool GetAllowEditingOfPinDataOnDetailsPanel();
#endif
	
public:

#if WITH_EDITORONLY_DATA
	/** Delegate to call when a property has changed */
	FOnPropertyChanged PropertyChangedNotifiers;
#endif

	/**
	 * A function that will be triggered whenever a compiling of the node is conducted.
	 * Put any messages or errors regarded with the compilation.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Editor|Compilation")
	void OnCompileNode(TArray<FJointEdLogMessage>& LogMessages);

	virtual void OnCompileNode_Implementation(TArray<FJointEdLogMessage>& LogMessages);

public:
	static UJointFragment* IterateAndGetTheFirstFragmentForClassUnderNode(
		UJointNodeBase* NodeToCollect, TSubclassOf<UJointFragment> SpecificClassToFind);

	static void IterateAndCollectAllFragmentsUnderNode(UJointNodeBase* NodeToCollect,
	                                                   TArray<UJointFragment*>& Fragments,
	                                                   TSubclassOf<UJointFragment> SpecificClassToFind);

public:

#if WITH_EDITORONLY_DATA
	
	//The reference to the graph node. It will be feed to the node instance when only the editor is opened.
	UPROPERTY(transient)
	TWeakObjectPtr<UEdGraphNode> EdGraphNode;

#endif

public:

	/**
	 * Called during saving to determine the load flags to save with the object.
	 * If false, this object will be discarded on clients
	 *
	 * @return	true if this object should be loaded on clients
	 */
	virtual bool NeedsLoadForClient() const override;

	/**
	 * Called during saving to determine the load flags to save with the object.
	 * If false, this object will be discarded on servers
	 *
	 * @return	true if this object should be loaded on servers
	 */
	virtual bool NeedsLoadForServer() const override;

	/**
	 * Returns whether we need to load this node for the provided platform.
	 */
	virtual bool NeedsLoadForTargetPlatform(const ITargetPlatform* TargetPlatform) const override;
	
};
