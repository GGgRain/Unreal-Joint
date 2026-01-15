//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointActor.h"

#include "Node/JointFragment.h"
#include "JointManager.h"
#include "Node/JointNodeBase.h"
#include "Joint.h"
#include "JointLogChannels.h"
#include "Subsystem/JointSubsystem.h"

#include "Engine/ActorChannel.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include "Misc/EngineVersionComparison.h"

//Debug related macro.
#define DEBUG_ShowNodeEvent 0 && WITH_EDITOR
#define DEBUG_ShowNodeEvent_BeginPlay 1 && DEBUG_ShowNodeEvent
#define DEBUG_ShowNodeEvent_EndPlay 1 && DEBUG_ShowNodeEvent
#define DEBUG_ShowNodeEvent_Pending 1 && DEBUG_ShowNodeEvent

#define DEBUG_ShowJointEvent 0 && WITH_EDITOR
#define DEBUG_ShowJointEvent_SetJoint 1 && DEBUG_ShowJointEvent
#define DEBUG_ShowJointEvent_StartJoint 1 && DEBUG_ShowJointEvent
#define DEBUG_ShowJointEvent_EndJoint 1 && DEBUG_ShowJointEvent
#define DEBUG_ShowJointEvent_DiscardJoint 0 && DEBUG_ShowJointEvent
#define DEBUG_ShowJointEvent_DestroyJoint 0 && DEBUG_ShowJointEvent
#define DEBUG_ShowJointEvent_PlayNextNode 1 && DEBUG_ShowJointEvent
#define DEBUG_ShowJointEvent_SetPlayingNode 1 && DEBUG_ShowJointEvent
#define DEBUG_ShowJointEvent_PopExecutionQueue 1 && DEBUG_ShowJointEvent


#define DEBUG_ShowReplication 0 && WITH_EDITOR

// Define to use new replication system introduced in UE 5.1.0.
#define USE_NEW_REPLICATION !UE_VERSION_OLDER_THAN(5, 1, 0) && true

#define JOINT_NET_CONTEXT(Object) \
	(UKismetSystemLibrary::IsStandalone(Object) ? TEXT("Standalone") : \
	UKismetSystemLibrary::IsDedicatedServer(Object) ? TEXT("Dedicated Server") : \
	UKismetSystemLibrary::IsServer(Object) ? TEXT("Listen Server (Client_Host)") : \
	TEXT("Client"))

#define JOINT_DEBUG_LOG(Object, Color, Format, ...) \
	do { \
		if (UWorld* World = (Object)->GetWorld()) { \
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0)) { \
				const FString Msg = FString::Printf( \
					Format, \
					JOINT_NET_CONTEXT(Object), \
					*PC->GetName(), \
					*(Object)->GetName(), \
					##__VA_ARGS__ \
				); \
				if (GEngine) { \
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, Color, Msg); \
				} \
				UE_LOG(LogJoint, Log, TEXT("%s"), *Msg); \
			} \
		} \
	} while (0)


// Sets default values
AJointActor::AJointActor()
{
	//Joint Actor can not tick.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

#if USE_NEW_REPLICATION

	bReplicateUsingRegisteredSubObjectList = true;

#endif

	bAlwaysRelevant = true;

	ImplementAbilitySystemComponent();

	JointGuid = FGuid::NewGuid();
}

UAbilitySystemComponent* AJointActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AJointActor::ImplementAbilitySystemComponent()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
}

void AJointActor::OnRep_JointManager(const UJointManager* PreviousJointManager)
{
#if DEBUG_ShowReplication
	
	JOINT_DEBUG_LOG(this, FColor::Orange, TEXT("%s, %s, %s: OnRep_JointManager, Previous Joint Manager : %s, New Joint Manager : %s"),
	               PreviousJointManager ? *PreviousJointManager->GetName() : *FString("None"),
	               JointManager ? *JointManager->GetName() : *FString("None"));

#endif
	RequestSetJointManager(JointManager);
}

void AJointActor::ProcessExecutionQueue(bool bForceTakeHandle)
{
	// if bForceTakeHandle is true, we allow re-entrance into this function. This is useful when we want to process the queue right now even if it is already being processed.
	// set bIsProcessingExecutionQueue to false if we have to force take the handle.
	bIsProcessingExecutionQueue = !bForceTakeHandle && bIsProcessingExecutionQueue;
	 
	if ( bIsProcessingExecutionQueue ) return;
	
	bIsProcessingExecutionQueue = true;
	
	// check bIsProcessingExecutionQueue here to let it stop 
	while (ExecutionQueue.Num() > 0 && bIsProcessingExecutionQueue )
	{
		PopExecutionQueue();
	}
	
	bIsProcessingExecutionQueue = false;
}

void AJointActor::EnqueueExecutionElement(const FJointActorExecutionElement& NewElement)
{
	ExecutionQueue.Add(NewElement);
}

void AJointActor::PopExecutionQueue()
{
	const FJointActorExecutionElement Item = ExecutionQueue[0];
	
#if WITH_EDITOR
	
	if (CheckDebuggerWantToHaltExecution(Item))
	{
		//If the debugger has breakpoint for this node, stop the execution here.
		bIsProcessingExecutionQueue = false;
			
		return;
	}
#endif
	
#if DEBUG_ShowJointEvent_PopExecutionQueue
	
	FString LeftInQueue;
	for (int32 i = 1; i < ExecutionQueue.Num(); i++)
	{
		LeftInQueue += FString::Printf(TEXT("%s ( %s ), "), 
			ExecutionQueue[i].TargetNode.IsValid() ? *ExecutionQueue[i].TargetNode->GetName() : *FString("None"),
			*UEnum::GetValueAsString(ExecutionQueue[i].ExecutionType));
	}
	
	JOINT_DEBUG_LOG(this, FColor::Silver, TEXT("%s, %s, %s: PopExecutionQueue - Begin, Node : %s, Execution Type : %s, Left in Queue : ( %s )"),
	               Item.TargetNode.IsValid() ? *Item.TargetNode->GetName() : *FString("None"),
	               *UEnum::GetValueAsString(Item.ExecutionType),
	               *LeftInQueue);

#endif
	
	switch (Item.ExecutionType)
	{
	case EJointActorExecutionType::PreBeginPlay:
		ExecutionQueue.RemoveAt(0);
		ProcessPreNodeBeginPlay(Item.TargetNode.Get());
		break;
	case EJointActorExecutionType::PostBeginPlay:
		ExecutionQueue.RemoveAt(0);
		ProcessPostNodeBeginPlay(Item.TargetNode.Get());
		break;
	case EJointActorExecutionType::PrePending:
		ExecutionQueue.RemoveAt(0);
		ProcessPreMarkNodeAsPending(Item.TargetNode.Get());
		break;
	case EJointActorExecutionType::PostPending:
		ExecutionQueue.RemoveAt(0);
		ProcessPostMarkNodeAsPending(Item.TargetNode.Get());
		break;
	case EJointActorExecutionType::PreEndPlay:
		ExecutionQueue.RemoveAt(0);
		ProcessPreNodeEndPlay(Item.TargetNode.Get());
		break;
	case EJointActorExecutionType::PostEndPlay:
		ExecutionQueue.RemoveAt(0);
		ProcessPostNodeEndPlay(Item.TargetNode.Get());
		break;
	case EJointActorExecutionType::None:
		break;
	}
	
#if DEBUG_ShowJointEvent_PopExecutionQueue
	
	FString LeftInQueue_After;
	for (int32 i = 0; i < ExecutionQueue.Num(); i++)
	{
		LeftInQueue_After += FString::Printf(TEXT("%s ( %s ), "), 
			ExecutionQueue[i].TargetNode.IsValid() ? *ExecutionQueue[i].TargetNode->GetName() : *FString("None"),
			*UEnum::GetValueAsString(ExecutionQueue[i].ExecutionType));
	}
	
	JOINT_DEBUG_LOG(this, FColor::Silver, TEXT("%s, %s, %s: PopExecutionQueue - End, Node : %s, Execution Type : %s, Left in Queue : ( %s )"),
	               Item.TargetNode.IsValid() ? *Item.TargetNode->GetName() : *FString("None"),
	               *UEnum::GetValueAsString(Item.ExecutionType),
	               *LeftInQueue_After);
	
#endif
	
	
}

void AJointActor::ClearExecutionQueue()
{
	ExecutionQueue.Empty();
}

void AJointActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndJoint();
	
	Super::EndPlay(EndPlayReason);
}

UJointNodeBase* AJointActor::GetPlayingJointNode()
{
	return PlayingJointNode;
}

void AJointActor::OnRep_CachedNodesForNetworking(
	const TArray<UJointNodeBase*>& PreviousCachedNodesForNetworking)
{
	// ATM, CachedNodesForNetworking is already updated with the new value when this function is called - so we can compare it with the previous value.
	
#if USE_NEW_REPLICATION
	
	if (IsUsingRegisteredSubObjectList())
	{
		// Find the attributes that got removed
		for (UJointNodeBase* PreviousNode : PreviousCachedNodesForNetworking)
		{
			if (PreviousNode)
			{
				const bool bWasRemoved = CachedNodesForNetworking.Find(PreviousNode) == INDEX_NONE;
				
				if (bWasRemoved)
				{
					RemoveReplicatedSubObject(PreviousNode);
				}
			}
		}

		// Find the attributes that got added
		for (UJointNodeBase* CurNodes : CachedNodesForNetworking)
		{
			if (IsValid(CurNodes))
			{
				//Check whether it is added newly.
				const bool bIsAdded = PreviousCachedNodesForNetworking.Find(CurNodes) == INDEX_NONE;
				
				//If it is added newly, register it.
				if (bIsAdded)
				{
					AddReplicatedSubObject(CurNodes);
				}
			}
		}
	}
	
#endif

	/* Clean Up code for removed nodes.
	for (UJointNodeBase* PreviousNode : PreviousCachedNodesForNetworking)
	{
		if (PreviousNode && CachedNodesForNetworking.Find(PreviousNode) == INDEX_NONE)
		{

		}
	}
	*/
}

void AJointActor::RequestSetJointManager_Implementation(UJointManager* NewJointManager)
{
	if (NewJointManager != nullptr)
	{
		UJointManager* DuplicatedJointManager = DuplicateObject<UJointManager>(NewJointManager, this);

#if WITH_EDITOR
		//For debugging purpose.

		OriginalJointManager = NewJointManager;
#endif

		JointManager = DuplicatedJointManager;
		
		//SetJointManager(DuplicatedJointManager);
		
		// Cache nodes for networking here because the Joint Manager has changed.
		// This is also necessary because sometimes we need to start replicating nodes before the Joint starts playing (especially for the participants)
		CacheNodesForNetworking();

#if DEBUG_ShowJointEvent_SetJoint
		
		JOINT_DEBUG_LOG(this, FColor::Emerald, TEXT("%s, %s, %s: Joint Setted, Joint Manager : %s"),*JointManager->GetName());

#endif
	}
}

UJointManager* AJointActor::GetJointManager()
{
	return JointManager;
}

void AJointActor::OnNotifiedCurrentNodePending(UJointNodeBase* Node)
{
#if DEBUG_ShowNodeEvent_Pending
	
	JOINT_DEBUG_LOG(this, FColor::Yellow, TEXT("%s, %s, %s: OnNotifiedCurrentNodePending, Node : %s "),
	               Node ? *Node->GetName() : *FString("None"));

#endif

	//On Client side, stop the current playing Joint.
	RequestNodeEndPlay(PlayingJointNode);

	//Let it move only when this client has the authority over the actor.
	//This little shit does work like freaking flip flop every time when try to do it from scratch.
	//This is because setting off the Joint manager and actor properly takes a deeper understanding of replication and networking.
	//Maybe I have to mention about it on the tutorial or other stuffs.
}

void AJointActor::OnNotifiedCurrentNodeEnded(UJointNodeBase* Node)
{
	if (HasAuthority())
	{
		PlayNextNode();
	}
}

void AJointActor::ReleaseEventsFromPlayingJointNode_Implementation()
{
	//Remove old delegate
	if (PlayingJointNode == nullptr) return;

	PlayingJointNode->OnJointNodeMarkedAsPendingDelegate.RemoveAll(this);

	PlayingJointNode->OnJointNodeEndDelegate.RemoveAll(this);
}

void AJointActor::BindEventsOnPlayingJointNode_Implementation()
{
	if (PlayingJointNode == nullptr) return;

	//Bind delegate
	PlayingJointNode->OnJointNodeMarkedAsPendingDelegate.RemoveAll(this);

	PlayingJointNode->OnJointNodeEndDelegate.RemoveAll(this);


	PlayingJointNode->OnJointNodeMarkedAsPendingDelegate.AddDynamic(
		this, &AJointActor::OnNotifiedCurrentNodePending);

	PlayingJointNode->OnJointNodeEndDelegate.AddDynamic(
		this, &AJointActor::OnNotifiedCurrentNodeEnded);
}

void AJointActor::SetPlayingJointNode_Implementation(UJointNodeBase* NewPlayingJointNode)
{
	PlayingJointNode = NewPlayingJointNode;

	//Reload the node's activity related flags.
	if (PlayingJointNode) RequestReloadNode(PlayingJointNode, true);

#if DEBUG_ShowJointEvent_SetPlayingNode
	
	JOINT_DEBUG_LOG(this, FColor::Emerald, TEXT("%s, %s, %s: Joint Set PlayingNode, New Node : %s"),
	               PlayingJointNode ? *PlayingJointNode->GetName() : *FString("Null"));

#endif
}

void AJointActor::BeginPlayPlayingJointNode_Implementation()
{
	if (PlayingJointNode)
	{
		if (OnJointBaseNodePlayedDelegate.IsBound())
		{
			OnJointBaseNodePlayedDelegate.Broadcast(this, PlayingJointNode);
		}
		RequestNodeBeginPlay(PlayingJointNode);
	}
}

void AJointActor::EndPlayPlayingJointNode_Implementation()
{
	if (PlayingJointNode)
	{
		RequestNodeEndPlay(PlayingJointNode);
	}
}

UJointNodeBase* AJointActor::PickUpNewNodeFrom(const TArray<UJointNodeBase*>& TestTargetNodes)
{
	for (UJointNodeBase* TargetNode : TestTargetNodes)
	{
		if (!TargetNode) continue;

		return TargetNode;
	}

	return nullptr;
}

bool AJointActor::IsJointStarted()
{
	return bIsJointStarted;
}

bool AJointActor::IsJointEnded()
{
	return bIsJointEnded;
}

void AJointActor::StartJoint_Implementation()
{
	if (IsJointStarted()) return;

	if (JointManager == nullptr) return;

#if DEBUG_ShowJointEvent_StartJoint

	JOINT_DEBUG_LOG(this, FColor::Emerald, TEXT("%s, %s, %s: JointStarted, Joint Manager : %s"), *JointManager->GetName());

#endif

#if WITH_EDITOR

	if (FJointModule* Module = &FModuleManager::GetModuleChecked<FJointModule>("Joint"); Module != nullptr && Module->JointDebuggerJointBeginPlayNotification.IsBound())
	{
		Module->JointDebuggerJointBeginPlayNotification.ExecuteIfBound(this, this->JointGuid);
	}

#endif
	
	CacheNodesForNetworking();
	
	//Multicast the actual action on the Joint start event.
	ProcessStartJoint();

	//Pick up the new node to play.
	SetPlayingJointNode(PickUpNewNodeFrom(JointManager->StartNodes));
	
	//Check we have actually playing Joint node. If not, just end it here.
	if (PlayingJointNode == nullptr)
	{
		EndJoint();
	}else
	{
		BindEventsOnPlayingJointNode();

		BeginPlayPlayingJointNode();	
	}
	
}

void AJointActor::EndJoint_Implementation()
{
	if (IsJointEnded()) return;

	if (JointManager == nullptr) return;

#if DEBUG_ShowJointEvent_EndJoint

	JOINT_DEBUG_LOG(this, FColor::Emerald, TEXT("%s, %s, %s: JointEnded"));

#endif

	ReleaseEventsFromPlayingJointNode();

	EndPlayPlayingJointNode();
	
	DiscardJoint();
	
	//Multicast the actual action on the Joint end event.
	ProcessEndJoint();

#if WITH_EDITOR

	if (FJointModule* Module = &FModuleManager::GetModuleChecked<FJointModule>("Joint"); Module != nullptr && Module->JointDebuggerJointEndPlayNotification.IsBound())
	{
		Module->JointDebuggerJointEndPlayNotification.ExecuteIfBound(this, this->JointGuid);
	}

#endif
	
	//Clear it if it's not being destroyed.
	if(!IsActorBeingDestroyed()) DestroyJoint();
}


void AJointActor::ForceEndAllKnownActiveNodes()
{
	TArray<UJointNodeBase*> CopiedKnownActiveNodes = KnownActiveNodes;

	for (int i = CopiedKnownActiveNodes.Num() - 1; i > INDEX_NONE; --i)
	{
		if (UJointNodeBase* NodeBase = CopiedKnownActiveNodes[i]; NodeBase && NodeBase->IsValidLowLevel())
			RequestNodeEndPlay(NodeBase);
	}

	KnownActiveNodes.Empty();
}

void AJointActor::DiscardJoint_Implementation()
{
#if DEBUG_ShowJointEvent_DiscardJoint
	
	JOINT_DEBUG_LOG(this, FColor::Emerald, TEXT("%s, %s, %s: DiscardJoint"));

#endif

	ReleaseEventsFromPlayingJointNode();

	ForceEndAllKnownActiveNodes();
}


void AJointActor::DestroyJoint_Implementation()
{
#if DEBUG_ShowJointEvent_DestroyJoint
	
	JOINT_DEBUG_LOG(this, FColor::Emerald, TEXT("%s, %s, %s: DestroyJoint"));
	
#endif
	
	if (IsValidLowLevel() && GetWorld())
	{
		//Destroy on the next tick for additional clean up.
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AJointActor::DestroyInstance);
	}
}

void AJointActor::DestroyInstance()
{
	Destroy();
}

void AJointActor::ProcessStartJoint_Implementation()
{
	MarkAsStarted();

	NotifyStartJoint();

	if (OnJointStartedDelegate.IsBound())
	{
		OnJointStartedDelegate.Broadcast(this);
	}

	BeginManagerFragments();
}

void AJointActor::ProcessEndJoint_Implementation()
{
	EndManagerFragments();

	ForceEndAllKnownActiveNodes();

	MarkAsEnded();

	NotifyEndJoint();

	if (OnJointEndedDelegate.IsBound())
	{
		OnJointEndedDelegate.Broadcast(this);
	}
}

void AJointActor::MarkAsStarted()
{
	bIsJointStarted = true;
}

void AJointActor::MarkAsEnded()
{
	bIsJointEnded = true;
}


void AJointActor::NotifyStartJoint()
{
	if (UJointSubsystem* SubSystem = UJointSubsystem::Get(this)) SubSystem->OnJointStarted(this);
}

void AJointActor::NotifyEndJoint()
{
	if (UJointSubsystem* SubSystem = UJointSubsystem::Get(this)) SubSystem->OnJointEnded(this);
}

void AJointActor::PlayNextNode_Implementation()
{
	if (JointManager == nullptr) return;

#if DEBUG_ShowJointEvent_PlayNextNode

	JOINT_DEBUG_LOG(this, FColor::Emerald, TEXT("%s, %s, %s: PlayNextNode - Begin"));
	
#endif

	ReleaseEventsFromPlayingJointNode();
	
	EndPlayPlayingJointNode();
	
	//Select new node from the last node.
	if (PlayingJointNode) SetPlayingJointNode(PickUpNewNodeFrom(PlayingJointNode->SelectNextNodes(this)));
	
	// Clear the execution queue to avoid any pending actions on the previous node.
	//ClearExecutionQueue();

	if (PlayingJointNode == nullptr)
	{
		EndJoint();
	}else
	{
		BindEventsOnPlayingJointNode();

		BeginPlayPlayingJointNode();	
	}
	
#if DEBUG_ShowJointEvent_PlayNextNode
	
	JOINT_DEBUG_LOG(this, FColor::Emerald, TEXT("%s, %s, %s: PlayNextNode - End"));	
	
#endif
}

void AJointActor::ProcessPlayNextNode_Implementation()
{
}


void AJointActor::RequestNodeBeginPlay(UJointNodeBase* InNode)
{
	//check if the node can be begun play.
	if (!InNode) return;
	
	if (InNode->IsNodeBegunPlay()) return;
	
	EnqueueExecutionElement(
		FJointActorExecutionElement(
			EJointActorExecutionType::PreBeginPlay,
			InNode)
	);
	
	ProcessExecutionQueue(false);
}

void AJointActor::RequestNodeEndPlay(UJointNodeBase* InNode)
{
	//check if the node can be ended play.
	if (!InNode) return;
	
	if (!InNode->IsNodeBegunPlay() || InNode->IsNodeEndedPlay()) return;
	
	EnqueueExecutionElement(
		FJointActorExecutionElement(
			EJointActorExecutionType::PreEndPlay,
			InNode)
	);
	
	ProcessExecutionQueue(false);
}

void AJointActor::RequestMarkNodeAsPending(UJointNodeBase* InNode)
{
	//check if the node can be ended play.
	if (!InNode) return;
	
	if (!InNode->IsNodeBegunPlay() || InNode->IsNodePending()) return;
	
	EnqueueExecutionElement(
		FJointActorExecutionElement(
			EJointActorExecutionType::PrePending,
			InNode)
	);
	
	ProcessExecutionQueue(false);
}

void AJointActor::RequestReloadNode(UJointNodeBase* InNode, const bool bPropagateToSubNodes, const bool bAllowPropagationEvenParentFails)
{
	if (!InNode) return;

	const bool& bCanReloadNode = InNode->CanReloadNode();
	
	if (bCanReloadNode) InNode->ReloadNode();
	
	if (bPropagateToSubNodes && (bCanReloadNode || bAllowPropagationEvenParentFails)) {
		
		for (UJointNodeBase* SubNode : InNode->SubNodes)
		{
			RequestReloadNode(SubNode, bPropagateToSubNodes);
		}
	}
}

void AJointActor::RequestPostNodeBeginPlay(UJointNodeBase* InNode)
{
	EnqueueExecutionElement(
		FJointActorExecutionElement(
			EJointActorExecutionType::PostBeginPlay,
			InNode)
	);
	
	ProcessExecutionQueue(false);
}

void AJointActor::RequestPostNodeEndPlay(UJointNodeBase* InNode)
{
	EnqueueExecutionElement(
		FJointActorExecutionElement(
			EJointActorExecutionType::PostEndPlay,
			InNode)
	);
	
	ProcessExecutionQueue(false);
}

void AJointActor::RequestPostMarkNodeAsPending(UJointNodeBase* InNode)
{
	EnqueueExecutionElement(
		FJointActorExecutionElement(
			EJointActorExecutionType::PostPending,
			InNode)
	);
	
	ProcessExecutionQueue(false);
}

void AJointActor::ProcessPreNodeBeginPlay(UJointNodeBase* InNode)
{
	if (!InNode) return;

	if (InNode->IsNodeBegunPlay()) return;


#if DEBUG_ShowNodeEvent_BeginPlay
	
	JOINT_DEBUG_LOG(this, FColor::Yellow, TEXT("%s, %s, %s: RequestNodeBeginPlay, Node : %s"), *InNode->GetName());

#endif
	
	if (InNode->GetParentNode())
	{
		//Check if the node has parent node and parent node is active. otherwise, don't start it.

		if (!InNode->GetParentNode()->IsNodeBegunPlay() || InNode->GetParentNode()->IsNodeEndedPlay()) return;
	}
	else
	{
		//If it is a base node, then check whether we are actually playing it, while this node is not a manager fragment.
		if (this->PlayingJointNode != InNode && !GetJointManager()->ManagerFragments.Contains(InNode)) return;
	}

	InNode->SetHostingJointInstance(this);

#if WITH_EDITOR

	if (FJointModule* Module = &FModuleManager::GetModuleChecked<FJointModule>("Joint"); Module != nullptr && Module->
	                                                                                                          JointDebuggerNodeBeginPlayNotification.IsBound())
	{
		Module->JointDebuggerNodeBeginPlayNotification.ExecuteIfBound(this, InNode);
	}

#endif

	KnownActiveNodes.Add(InNode);

	InNode->ProcessPreNodeBeginPlay();
}

void AJointActor::ProcessPostNodeBeginPlay(UJointNodeBase* InNode)
{
	if (!InNode) return;

	if (!InNode->IsNodeBegunPlay()) return;

	InNode->ProcessPostNodeBeginPlay();
}

void AJointActor::ProcessPreNodeEndPlay(UJointNodeBase* InNode)
{
	if (!InNode) return;

	if (InNode->IsNodeEndedPlay()) return;

#if DEBUG_ShowNodeEvent_EndPlay
	
	JOINT_DEBUG_LOG(this, FColor::Yellow, TEXT("%s, %s, %s: RequestNodeEndPlay, Node : %s"), *InNode->GetName());
		
#endif

	KnownActiveNodes.Remove(InNode);

	InNode->ProcessPreNodeEndPlay();

#if WITH_EDITOR

	if (FJointModule* Module = &FModuleManager::GetModuleChecked<FJointModule>("Joint"); Module != nullptr && Module->
	                                                                                                          JointDebuggerNodeEndPlayNotification.IsBound())
	{
		Module->JointDebuggerNodeEndPlayNotification.ExecuteIfBound(this, InNode);
	}

#endif
}

void AJointActor::ProcessPostNodeEndPlay(UJointNodeBase* InNode)
{
	if (!InNode) return;

	if (!InNode->IsNodeEndedPlay()) return;

	InNode->ProcessPostNodeEndPlay();
}

void AJointActor::ProcessPreMarkNodeAsPending(UJointNodeBase* InNode)
{
	if (!InNode) return;

	if (InNode->IsNodePending()) return;

#if WITH_EDITOR
	
	if (FJointModule* Module = &FModuleManager::GetModuleChecked<FJointModule>("Joint"); Module != nullptr && Module->JointDebuggerNodePendingNotification.IsBound())
	{
		Module->JointDebuggerNodePendingNotification.ExecuteIfBound(this, InNode);
	}

#endif

#if DEBUG_ShowNodeEvent_Pending

	JOINT_DEBUG_LOG(this, FColor::Yellow, TEXT("%s, %s, %s: RequestMarkNodeAsPending, Node : %s"), *InNode->GetName());

#endif

	InNode->ProcessPreMarkNodePending();
}

void AJointActor::ProcessPostMarkNodeAsPending(UJointNodeBase* InNode)
{
	if (!InNode) return;

	if (!InNode->IsNodePending()) return;

	InNode->ProcessPostMarkNodePending();
}

void AJointActor::NotifyNodeBeginPlay(UJointNodeBase* InNode)
{
	if (IsValidLowLevel() && OnJointNodeBeginPlayDelegate.IsBound()) OnJointNodeBeginPlayDelegate.Broadcast(this, InNode);
}

void AJointActor::NotifyNodeEndPlay(UJointNodeBase* InNode)
{
	if (IsValidLowLevel() && OnJointNodeEndPlayDelegate.IsBound()) OnJointNodeEndPlayDelegate.Broadcast(this, InNode);
}

void AJointActor::NotifyNodeMarkedAsPending(UJointNodeBase* InNode)
{
	if (IsValidLowLevel() && OnJointNodeMarkedAsPendingDelegate.IsBound()) OnJointNodeMarkedAsPendingDelegate.Broadcast(this, InNode);
}


void AJointActor::BeginManagerFragments()
{
	if (!JointManager) return;

	for (UJointNodeBase* ManagerFragment : JointManager->ManagerFragments)
	{
		if (ManagerFragment == nullptr) continue;
		
		ManagerFragment->RequestNodeBeginPlay(this);
	}
}

void AJointActor::EndManagerFragments()
{
	if (!JointManager) return;

	for (UJointNodeBase* ManagerFragment : JointManager->ManagerFragments)
	{
		if (ManagerFragment == nullptr) continue;

		ManagerFragment->RequestNodeEndPlay();
	}
}

#if WITH_EDITOR

bool AJointActor::CheckDebuggerWantToHaltExecution(const FJointActorExecutionElement& NodeExecutionElement)
{
	if (FJointModule* Module = &FModuleManager::GetModuleChecked<FJointModule>("Joint"); Module != nullptr && Module->OnJointExecutionExceptionDelegate.IsBound())
	{
		return Module->OnJointExecutionExceptionDelegate.Execute(this, NodeExecutionElement);
	}

	return false;
}

#endif

void AJointActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	Params.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(AJointActor, CachedNodesForNetworking, Params);
	//DOREPLIFETIME(AJointActor, JointManager);
}

bool AJointActor::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
#if USE_NEW_REPLICATION
	
	return Super::ReplicateSubobjects(Channel,Bunch,RepFlags);

#else


	check(Channel);
	check(Bunch);
	check(RepFlags);

	bool WroteSomething = false;


	for (UJointNodeBase* NodeForNetworking : CachedNodesForNetworking)
	{
		if (IsValid(NodeForNetworking))
		{
			WroteSomething |= Channel->ReplicateSubobject(NodeForNetworking, *Bunch, *RepFlags);
		}
	}
	
	for (UActorComponent* ActorComp : ReplicatedComponents)
	{
		if (ActorComp && ActorComp->GetIsReplicated())
		{
			WroteSomething |= ActorComp->ReplicateSubobjects(Channel, Bunch, RepFlags);
			// Lets the component add subobjects before replicating its own properties.
			WroteSomething |= Channel->ReplicateSubobject(ActorComp, *Bunch, *RepFlags);
			// (this makes those subobjects 'supported', and from here on those objects may have reference replicated)		
		}
	}

#if DEBUG_ShowReplication

	if (WroteSomething) UE_LOG(LogJoint, Log, TEXT("%s: Replicated subobjects"), *this->GetName());

#endif

	return WroteSomething;

#endif
}


void AJointActor::CacheNodesForNetworking()
{
	if (HasAuthority())
	{
		if (JointManager == nullptr) return;

#if USE_NEW_REPLICATION
		
		for (UJointNodeBase* NodesForNetworking : CachedNodesForNetworking)
		{
			if(IsValid(NodesForNetworking))
			{
				RemoveNodeForNetworking(NodesForNetworking);
			}
		}
		
#endif


		CachedNodesForNetworking.Empty();

		TArray<UJointNodeBase*> Nodes;

		for (UJointFragment* Fragment : JointManager->GetAllManagerFragmentsOnLowerHierarchy())
		{
			if (IsValid(Fragment))
			{
				if (Fragment->bReplicates)
				{
					Nodes.Add(Fragment);
				}
			}
		}

		for (UJointNodeBase* Node : JointManager->Nodes)
		{
			if (IsValid(Node))
			{
				if (Node->bReplicates)
				{
					Nodes.Add(Node);
				}

				for (UJointNodeBase* SubNodes : Node->GetAllFragmentsOnLowerHierarchy())
				{
					if (IsValid(SubNodes))
					{
						if (SubNodes->bReplicates)
						{
							Nodes.Add(SubNodes);
						}
					}
				}
			}
		}

		CachedNodesForNetworking = Nodes;
		
#if USE_NEW_REPLICATION
		
		// This must be called after the caching
		
		for (UJointNodeBase* AllNodesForNetworking : CachedNodesForNetworking)
		{
			if(IsValid(AllNodesForNetworking))
			{
				AddReplicatedSubObject(AllNodesForNetworking);
			}
		}

#endif
		
	}
}

void AJointActor::AddNodeForNetworking(UJointNodeBase* InNode)
{
	if (HasAuthority())
	{
		if (JointManager == nullptr) return;

		if (InNode->GetHostingJointInstance() != this || InNode->GetJointManager() != this->GetJointManager())
			return;

		if (CachedNodesForNetworking.Contains(InNode)) return;

#if USE_NEW_REPLICATION
	
		AddReplicatedSubObject(InNode);

#endif

		CachedNodesForNetworking.Add(InNode);
	}
}

void AJointActor::RemoveNodeForNetworking(UJointNodeBase* InNode)
{
	if (HasAuthority())
	{
		if (JointManager == nullptr) return;

		if (InNode->GetHostingJointInstance() != this || InNode->GetJointManager() != this->GetJointManager())
			return;

		if (!CachedNodesForNetworking.Contains(InNode)) return;

#if USE_NEW_REPLICATION
	
		RemoveReplicatedSubObject(InNode);

#endif

		CachedNodesForNetworking.Remove(InNode);
	}
}

#undef DEBUG_ShowNodeEvent
#undef DEBUG_ShowNodeEvent_BeginPlay
#undef DEBUG_ShowNodeEvent_EndPlay
#undef DEBUG_ShowNodeEvent_Pending

#undef DEBUG_ShowJointEvent
#undef DEBUG_ShowJointEvent_SetJoint
#undef DEBUG_ShowJointEvent_StartJoint
#undef DEBUG_ShowJointEvent_EndJoint
#undef DEBUG_ShowJointEvent_PlayNextNode
#undef DEBUG_ShowJointEvent_SetPlayingNode

#undef DEBUG_ShowReplication

#undef USE_NEW_REPLICATION
