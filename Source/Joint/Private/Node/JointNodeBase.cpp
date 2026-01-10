//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/JointNodeBase.h"
#include "UObject/ConstructorHelpers.h"

#include "JointActor.h"
#include "JointLogChannels.h"
#include "Node/JointFragment.h"
#include "JointManager.h"

#include "Engine/NetDriver.h"
#include "Net/UnrealNetwork.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Interfaces/ITargetPlatform.h"
#include "Kismet/GameplayStatics.h"

#if WITH_EDITOR

#include "Logging/MessageLog.h"
#include "EdGraph/EdGraph.h"
#endif

#define LOCTEXT_NAMESPACE "JointNodeBase"


UJointNodeBase::UJointNodeBase()
{
	
#if WITH_EDITORONLY_DATA

	EdNodeSetting.IconicNodeImageBrush = FSlateBrush();
	EdNodeSetting.IconicNodeImageBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	EdNodeSetting.IconicNodeImageBrush.ImageSize = FVector2D(2,2);

#endif
	
}

bool UJointNodeBase::IsSubNode()
{
	return GetParentNode() != nullptr;
}

UJointNodeBase* UJointNodeBase::GetParentNode() const
{
	return ParentNode;
}

UJointNodeBase* GetParentmostNodeOf(UJointNodeBase* InNode)
{
	if (!InNode) return nullptr;

	if (InNode->GetParentNode() != nullptr)
	{
		return GetParentmostNodeOf(InNode->GetParentNode());
	}

	return InNode;
};

UJointNodeBase* UJointNodeBase::GetParentmostNode()
{
	return GetParentmostNodeOf(this);
}

TArray<UJointNodeBase*> UJointNodeBase::GetParentNodesOnHierarchy() const
{
	TArray<UJointNodeBase*> OutArray;

	UJointNodeBase* CurParentNode = GetParentNode();

	while (CurParentNode != nullptr)
	{
		if (OutArray.Contains(CurParentNode))
		{
			checkf(false,
				TEXT( "JOINT: Circular parent node reference detected at Joint node %s with tested parent node %s. This is not allowed in any case. Change your logic related to the parent node reference."),
				*this->GetName(),
				*CurParentNode->GetName());

			break;
			
		}
		OutArray.Add(CurParentNode);

		CurParentNode = CurParentNode->GetParentNode();
	}

	return OutArray;
}

UJointManager* UJointNodeBase::GetJointManager() const
{
	if (!this->IsValidLowLevel()) return nullptr;
	
	UObject* Outer = GetOuter(); 
	
	while (Outer && Outer->IsValidLowLevel())
	{
		// Move upwards until we find the Joint manager.

		if (UJointManager* JointManager = Cast<UJointManager>(Outer))
		{
			return JointManager;
		}

		Outer = Outer->GetOuter();
	}

	return nullptr;
}

const FGuid& UJointNodeBase::GetNodeGuid() const
{
	return NodeGuid;
}

void UJointNodeBase::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = NodeTags;
}


void UJointNodeBase::IterateAndCollectAllFragmentsUnderNode(UJointNodeBase* NodeToCollect,
                                                               TArray<UJointFragment*>& Fragments,
                                                               TSubclassOf<UJointFragment> SpecificClassToFind)
{
	if (NodeToCollect == nullptr) return;

	for (UJointNodeBase* SubNode : NodeToCollect->SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (UJointFragment* Fragment = Cast<UJointFragment>(SubNode))
		{
			if (SpecificClassToFind != nullptr)
			{
				if (Fragment->GetClass() == SpecificClassToFind) Fragments.Add(Fragment);
			}
			else
			{
				Fragments.Add(Fragment);
			}
		}

		IterateAndCollectAllFragmentsUnderNode(SubNode, Fragments, SpecificClassToFind);
	}
}

bool UJointNodeBase::NeedsLoadForClient() const
{
	
#if WITH_EDITOR

	// If the class itself is excluded, exclude it (UObject::NeedsLoadForClient())
	
	if(!UObject::NeedsLoadForClient()) return false;

	// If parent node will not be loaded for the target, it will be excluded as well.

	if(ParentNode && !ParentNode->NeedsLoadForClient()) return false;
	
	// If no preset is provided, it will be included always.
	
	if(BuildPreset.IsNull()) return true;

	if(UJointBuildPreset* Preset = BuildPreset.LoadSynchronous()) // If no preset is provided, it will follow the preset.
	{
		return Preset->AllowForClient();
	}

#endif

	return true;

}

bool UJointNodeBase::NeedsLoadForServer() const
{
	
#if WITH_EDITOR

	// If the class itself is excluded, exclude it (UObject::NeedsLoadForClient())

	if(!UObject::NeedsLoadForServer()) return false;

	// If parent node will not be loaded for the target, it will be excluded as well.

	if(ParentNode && !ParentNode->NeedsLoadForServer()) return false;

	// If no preset is provided, it will be included always.
		
	if(BuildPreset.IsNull()) return true;

	if(UJointBuildPreset* Preset = BuildPreset.LoadSynchronous())
	{
		return Preset->AllowForServer();
	}

#endif
	
	return true;
}

bool UJointNodeBase::NeedsLoadForTargetPlatform(const ITargetPlatform* TargetPlatform) const
{

#if WITH_EDITOR

	// If parent node will not be loaded for the target, it will be excluded as well.

	if(ParentNode && !ParentNode->NeedsLoadForTargetPlatform(TargetPlatform)) return false;

	// If no preset is provided, it will be included always.

	if(BuildPreset.IsNull()) return true;

	if(UJointBuildPreset* Preset = BuildPreset.LoadSynchronous())
	{
		return Preset->AllowForBuildTarget(TargetPlatform);
	}

#endif
	

	return true;

}

void UJointNodeBase::OnPinConnectionChanged_Implementation(const TMap<FJointEdPinData, FJointNodes>& PinToConnections)
{
	//Do whatever you want if you need. Especially when you don't want to mess up with the editor nodes.
	//We recommend you to wrap all stuffs in this function with if WITH_EDITOR macro to prevent all possible exploit on the runtime scenarios.
#if WITH_EDITOR

	

#endif
	
}

void UJointNodeBase::OnUpdatePinData_Implementation(TArray<FJointEdPinData>& PinDat)
{
	//Do whatever you want if you need. 
	//We recommend you to wrap all stuffs in this function with if WITH_EDITOR macro to prevent all possible exploit on the runtime scenarios.
#if WITH_EDITOR

	

#endif

}

TArray<FJointEdPinData> UJointNodeBase::GetPinData() const
{

	TArray<FJointEdPinData> PinData;
	
#if WITH_EDITOR

	if(EdGraphNode.Get())
	{
		if(IJointEdNodeInterface* Interface = Cast<IJointEdNodeInterface>(EdGraphNode.Get()))
		{
			PinData = Interface->JointEdNodeInterface_GetPinData();
		}
	}
		
#endif

	return PinData;
	
}




const FJointEdPinConnectionResponse UJointNodeBase::CanAttachThisAtParentNode_Implementation(const UObject* InParentNode) const
{
	//Do whatever you want if you need. 
	//We recommend you to wrap all stuffs in this function with if WITH_EDITOR macro to prevent all possible exploit on the runtime scenarios.
#if WITH_EDITOR

	//Check if the node instance is valid.
	if (InParentNode == nullptr) return FJointEdPinConnectionResponse(EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW, LOCTEXT("DisallowedAttachmentMessage", "Unknown reason"));

	//if the node instance is the same as this node, disallow the connection.
	if (InParentNode == this) return FJointEdPinConnectionResponse(EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW, LOCTEXT("DisallowedAttachmentMessage_SameNode", "Can not attach a node to the same node. How are you even doing this?"));

	//Allow the connection.
	if(GEngine->IsEditor()) return FJointEdPinConnectionResponse(EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_MAKE, LOCTEXT("AllowedAttachmentMessage", "Allow Attaching"));

#endif
	
	//Runtime Fallback
	return FJointEdPinConnectionResponse(EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW, INVTEXT("CANNOT CHANGE ATTACHMENT ON RUNTIME"));
	
}



const FJointEdPinConnectionResponse UJointNodeBase::CanAttachSubNodeOnThis_Implementation(const UObject* InSubNode) const

{
	//Do whatever you want if you need. 
	//We recommend you to wrap all stuffs in this function with if WITH_EDITOR macro to prevent all possible exploit on the runtime scenarios.
#if WITH_EDITOR

	//Check if the node instance is valid.
	if (InSubNode == nullptr) return FJointEdPinConnectionResponse(EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW, LOCTEXT("DisallowedAttachmentMessage", "Unknown reason"));

	//if the node instance is the same as this node, disallow the connection.
	if (InSubNode == this) return FJointEdPinConnectionResponse(EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW, LOCTEXT("DisallowedAttachmentMessage_SameNode", "Can not attach a node to the same node. How are you even doing this?"));

	//Allow the connection.
	if(GEngine->IsEditor()) return FJointEdPinConnectionResponse(EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_MAKE, LOCTEXT("AllowedAttachmentMessage", "Allow Attaching"));

	
#endif

	//Runtime Fallback
	return FJointEdPinConnectionResponse(EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW, INVTEXT("CANNOT CHANGE ATTACHMENT ON RUNTIME"));
	
}



bool UJointNodeBase::GetAllowNodeInstancePinControl()
{
	
#if WITH_EDITOR  
	return EdNodeSetting.bAllowNodeInstancePinControl;  
#else  
	return false;  
#endif
	
}

void UJointNodeBase::OnCompileNode_Implementation(TArray<FJointEdLogMessage>& LogMessages)
{
}

UJointFragment* UJointNodeBase::IterateAndGetTheFirstFragmentForClassUnderNode(
	UJointNodeBase* NodeToCollect,
	TSubclassOf<UJointFragment> SpecificClassToFind)
{
	if (NodeToCollect == nullptr) return nullptr;

	for (UJointNodeBase* SubNode : NodeToCollect->SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (UJointFragment* Fragment = Cast<UJointFragment>(SubNode))
		{
			if (SpecificClassToFind != nullptr)
			{
				if (Fragment->GetClass() == SpecificClassToFind) return Fragment;
			}
			else
			{
				return Fragment;
			}
		}

		if (UJointFragment* Result = IterateAndGetTheFirstFragmentForClassUnderNode(SubNode, SpecificClassToFind))
			return Result;
	}

	return nullptr;
}


UJointFragment* UJointNodeBase::FindFragmentWithTagOnLowerHierarchy(
	const FGameplayTag InNodeTag, const bool bExact)
{
	TArray<UJointFragment*> Fragments;

	IterateAndCollectAllFragmentsUnderNode(this, Fragments, nullptr);

	for (UJointFragment* Fragment : Fragments)
	{
		if (Fragment == nullptr) continue;

		if (bExact ? Fragment->NodeTags.HasTagExact(InNodeTag) : Fragment->NodeTags.HasTag(InNodeTag)) return Fragment;
	}

	return nullptr;
}

TArray<UJointFragment*> UJointNodeBase::FindFragmentsWithTagOnLowerHierarchy(
	const FGameplayTag InNodeTag, const bool bExact)
{
	TArray<UJointFragment*> OutFragments;

	TArray<UJointFragment*> Fragments;

	IterateAndCollectAllFragmentsUnderNode(this, Fragments, nullptr);

	for (UJointFragment* Fragment : Fragments)
	{
		if (Fragment == nullptr) continue;

		if (bExact ? Fragment->NodeTags.HasTagExact(InNodeTag) : Fragment->NodeTags.HasTag(InNodeTag))
			OutFragments.
				Add(Fragment);
	}

	return OutFragments;
}

UJointFragment* UJointNodeBase::FindFragmentWithAnyTagsOnLowerHierarchy(
	const FGameplayTagContainer InNodeTagContainer, const bool bExact)
{
	TArray<UJointFragment*> Fragments;

	IterateAndCollectAllFragmentsUnderNode(this, Fragments, nullptr);

	for (UJointFragment* Fragment : Fragments)
	{
		if (Fragment == nullptr) continue;

		if (bExact ? Fragment->NodeTags.HasAnyExact(InNodeTagContainer) : Fragment->NodeTags.HasAny(InNodeTagContainer))
			return Fragment;
	}

	return nullptr;
}

TArray<UJointFragment*> UJointNodeBase::FindFragmentsWithAnyTagsOnLowerHierarchy(
	const FGameplayTagContainer InNodeTagContainer, const bool bExact)
{
	TArray<UJointFragment*> OutFragments;

	TArray<UJointFragment*> Fragments;

	IterateAndCollectAllFragmentsUnderNode(this, Fragments, nullptr);

	for (UJointFragment* Fragment : Fragments)
	{
		if (Fragment == nullptr) continue;

		if (bExact ? Fragment->NodeTags.HasAnyExact(InNodeTagContainer) : Fragment->NodeTags.HasAny(InNodeTagContainer))
			OutFragments.Add(Fragment);
	}

	return OutFragments;
}

UJointFragment* UJointNodeBase::FindFragmentWithAllTagsOnLowerHierarchy(
	const FGameplayTagContainer InNodeTagContainer, const bool bExact)
{
	TArray<UJointFragment*> Fragments;

	IterateAndCollectAllFragmentsUnderNode(this, Fragments, nullptr);

	for (UJointFragment* Fragment : Fragments)
	{
		if (Fragment == nullptr) continue;

		if (bExact ? Fragment->NodeTags.HasAllExact(InNodeTagContainer) : Fragment->NodeTags.HasAll(InNodeTagContainer))
			return Fragment;
	}

	return nullptr;
}

TArray<UJointFragment*> UJointNodeBase::FindFragmentsWithAllTagsOnLowerHierarchy(
	const FGameplayTagContainer InNodeTagContainer, const bool bExact)
{
	TArray<UJointFragment*> OutFragments;

	TArray<UJointFragment*> Fragments;

	IterateAndCollectAllFragmentsUnderNode(this, Fragments, nullptr);

	for (UJointFragment* Fragment : Fragments)
	{
		if (Fragment == nullptr) continue;

		if (bExact ? Fragment->NodeTags.HasAllExact(InNodeTagContainer) : Fragment->NodeTags.HasAll(InNodeTagContainer))
			OutFragments.Add(Fragment);
	}

	return OutFragments;
}


UJointFragment* UJointNodeBase::FindFragmentWithGuidOnLowerHierarchy(FGuid InNodeGuid)
{
	TArray<UJointFragment*> Fragments;

	IterateAndCollectAllFragmentsUnderNode(this, Fragments, nullptr);

	for (UJointFragment* Fragment : Fragments)
	{
		if (Fragment == nullptr) continue;

		if (Fragment->NodeGuid == InNodeGuid) return Fragment;
	}

	return nullptr;
}


UJointFragment* UJointNodeBase::FindFragmentByClassOnLowerHierarchy(
	TSubclassOf<UJointFragment> FragmentClass)
{
	return IterateAndGetTheFirstFragmentForClassUnderNode(this, FragmentClass);
}

TArray<UJointFragment*> UJointNodeBase::FindFragmentsByClassOnLowerHierarchy(
	TSubclassOf<UJointFragment> FragmentClass)
{
	TArray<UJointFragment*> Fragments;

	IterateAndCollectAllFragmentsUnderNode(this, Fragments, FragmentClass);

	return Fragments;
}

TArray<UJointFragment*> UJointNodeBase::GetAllFragmentsOnLowerHierarchy()
{
	TArray<UJointFragment*> Fragments;

	IterateAndCollectAllFragmentsUnderNode(this, Fragments, nullptr);

	return Fragments;
}

UJointFragment* UJointNodeBase::FindFragmentWithTag(FGameplayTag InNodeTag, const bool bExact)
{
	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (bExact ? !SubNode->NodeTags.HasTagExact(InNodeTag) : !SubNode->NodeTags.HasTag(InNodeTag)) continue;

		if (UJointFragment* Fragment = Cast<UJointFragment>(SubNode)) return Fragment;
	}

	return nullptr;
}

TArray<UJointFragment*> UJointNodeBase::FindFragmentsWithTag(FGameplayTag InNodeTag, const bool bExact)
{
	TArray<UJointFragment*> FinalArray;

	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (bExact ? !SubNode->NodeTags.HasTagExact(InNodeTag) : !SubNode->NodeTags.HasTag(InNodeTag)) continue;

		if (UJointFragment* Fragment = Cast<UJointFragment>(SubNode)) FinalArray.Add(Fragment);
	}

	return FinalArray;
}

UJointFragment* UJointNodeBase::FindFragmentWithAnyTags(FGameplayTagContainer InNodeTagContainer,
                                                              const bool bExact)
{
	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (bExact ? !SubNode->NodeTags.HasAnyExact(InNodeTagContainer) : !SubNode->NodeTags.HasAny(InNodeTagContainer))
			continue;

		if (UJointFragment* Fragment = Cast<UJointFragment>(SubNode)) return Fragment;
	}

	return nullptr;
}

TArray<UJointFragment*> UJointNodeBase::FindFragmentsWithAnyTags(FGameplayTagContainer InNodeTagContainer,
                                                                       const bool bExact)
{
	TArray<UJointFragment*> FinalArray;

	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (bExact ? !SubNode->NodeTags.HasAnyExact(InNodeTagContainer) : !SubNode->NodeTags.HasAny(InNodeTagContainer))
			continue;

		if (UJointFragment* Fragment = Cast<UJointFragment>(SubNode)) FinalArray.Add(Fragment);
	}

	return FinalArray;
}

UJointFragment* UJointNodeBase::FindFragmentWithAllTags(FGameplayTagContainer InNodeTagContainer,
                                                              const bool bExact)
{
	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (bExact ? !SubNode->NodeTags.HasAllExact(InNodeTagContainer) : !SubNode->NodeTags.HasAll(InNodeTagContainer))
			continue;

		if (UJointFragment* Fragment = Cast<UJointFragment>(SubNode)) return Fragment;
	}

	return nullptr;
}

TArray<UJointFragment*> UJointNodeBase::FindFragmentsWithAllTags(FGameplayTagContainer InNodeTagContainer,
                                                                       const bool bExact)
{
	TArray<UJointFragment*> FinalArray;

	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (bExact ? !SubNode->NodeTags.HasAllExact(InNodeTagContainer) : !SubNode->NodeTags.HasAll(InNodeTagContainer))
			continue;

		if (UJointFragment* Fragment = Cast<UJointFragment>(SubNode)) FinalArray.Add(Fragment);
	}

	return FinalArray;
}

UJointFragment* UJointNodeBase::FindFragmentWithGuid(FGuid InNodeGuid) const
{
	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (SubNode->NodeGuid != NodeGuid) continue;

		if (UJointFragment* Fragment = Cast<UJointFragment>(SubNode)) return Fragment;
	}

	return nullptr;
}

UJointFragment* UJointNodeBase::FindFragmentByClass(TSubclassOf<UJointFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UJointNodeBase* Fragment : SubNodes)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Cast<UJointFragment>(Fragment);
			}
		}
	}

	return nullptr;
}


TArray<UJointFragment*> UJointNodeBase::FindFragmentsByClass(
	TSubclassOf<UJointFragment> FragmentClass) const
{
	TArray<UJointFragment*> FinalArray;

	if (FragmentClass != nullptr)
	{
		for (UJointNodeBase* Fragment : SubNodes)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				FinalArray.Add(Cast<UJointFragment>(Fragment));
			}
		}
	}

	return FinalArray;
}

TArray<UJointFragment*> UJointNodeBase::GetAllFragments() const
{
	TArray<UJointFragment*> FinalArray;

	for (UJointNodeBase* Fragment : SubNodes)
	{
		if (Fragment && Fragment->GetClass()->IsChildOf(UJointFragment::StaticClass()))
		{
			FinalArray.Add(Cast<UJointFragment>(Fragment));
		}
	}


	return FinalArray;
}

TArray<UJointNodeBase*> UJointNodeBase::SelectNextNodes_Implementation(
	AJointActor* InHostingJointInstance)
{
	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		if(!SubNode->IsNodeBegunPlay()) continue;

		//Test through the sub nodes and if it has something then use that.
		TArray<UJointNodeBase*> Nodes = SubNode->SelectNextNodes(InHostingJointInstance);
		if (!Nodes.IsEmpty()) return Nodes;
	}

	return TArray<UJointNodeBase*>();
}

bool UJointNodeBase::IsNodeBegunPlay() const
{
	return bIsNodeBegunPlay;
}

bool UJointNodeBase::IsNodeEndedPlay() const
{
	return bIsNodeEndedPlay;
}

bool UJointNodeBase::IsNodePending() const
{
	return bIsNodePending;
}

bool UJointNodeBase::IsNodeActive() const
{
	return IsNodeBegunPlay() && !IsNodeEndedPlay();
}

void UJointNodeBase::PostNodeMarkedAsPending_Implementation()
{
}

void UJointNodeBase::MarkNodePendingByForce()
{
	if (IsNodePending()) return;

	if (!GetHostingJointInstance()) return;

	GetHostingJointInstance()->RequestMarkNodeAsPending(this);
}

void UJointNodeBase::MarkNodePendingIfNeeded()
{
	if (CheckCanMarkNodeAsPending())
	{
		//Mark pending.
		MarkNodePendingByForce();
	}
}


bool UJointNodeBase::CheckCanMarkNodeAsPending_Implementation()
{
	for (const UJointNodeBase* SubNode : SubNodes)
	{
		if (!SubNode) continue;

		if (!SubNode->IsNodePending())
		{
			return false;
		}
	}

	return true;
}

UWorld* UJointNodeBase::GetWorld() const
{
	if (const AJointActor* Instance = const_cast<AJointActor*>(GetHostingJointInstance()))
	{
		return GetHostingJointInstance()->GetWorld();
	}

	return nullptr;
}

void UJointNodeBase::SetReplicates_Implementation(const bool bNewReplicates)
{
	if (bReplicates != bNewReplicates)
	{
		if(GetHostingJointInstance())
		{
			if(bNewReplicates)
			{
				GetHostingJointInstance()->AddNodeForNetworking(this);
			}else
			{
				GetHostingJointInstance()->RemoveNodeForNetworking(this);
			}
		}

		bReplicates = bNewReplicates;
	}
}

const bool UJointNodeBase::GetReplicates() const
{
	return bReplicates;
}

bool UJointNodeBase::IsSupportedForNetworking() const
{
	return true;
}

void UJointNodeBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

bool UJointNodeBase::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	
	if (!GetHostingJointInstance()) return false;
	
	/**
	 * If bUsePlayerControllerAsRPCFunctionCallspace is true and the client does not have authority over the Joint Instance, it will try to retrieve the PlayerController's NetDriver to process the remote function.
	 * + This is extremely experimental.
	 */
	
	if (!GetHostingJointInstance()->HasAuthority() && bUsePlayerControllerAsRPCFunctionCallspace)
	{
		//Use the PlayerController's function callspace.
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

		if (!PC) return false;

		UNetDriver* NetDriver = PC->GetNetDriver();
	
		if (!NetDriver) return false;

		NetDriver->ProcessRemoteFunction(PC, Function, Parms, OutParms, Stack, this);
	}
	
	
	//Use the Joint actor's function callspace instead.
	UNetDriver* NetDriver = HostingJointInstance->GetNetDriver();
	
	if (!NetDriver) return false;

	NetDriver->ProcessRemoteFunction(HostingJointInstance.Get(), Function, Parms, OutParms, Stack, this);
	
	return true;
}

int32 UJointNodeBase::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
#if WITH_EDITOR
	
	if (HasAnyFlags(RF_ClassDefaultObject) || !IsSupportedForNetworking() || GetHostingJointInstance() == nullptr)
	{
		// This handles absorbing authority/cosmetic
		return GEngine->GetGlobalFunctionCallspace(Function, this, Stack);
	}
	
	//for the clients that do not have authority (over the Joint Instance) - use the player controller's function callspace if the option is enabled.
	if (!GetHostingJointInstance()->HasAuthority() && bUsePlayerControllerAsRPCFunctionCallspace)
	{
		//Use the PlayerController's function callspace.
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		
		if (PC)
		{
			return PC->GetFunctionCallspace(Function, Stack);
		}
	}
	
	return GetHostingJointInstance()->GetFunctionCallspace(Function, Stack);

#else

	if (HasAnyFlags(RF_ClassDefaultObject) || !IsSupportedForNetworking() || GetHostingJointInstance() == nullptr)
	{
		// This handles absorbing authority/cosmetic
		return GEngine->GetGlobalFunctionCallspace(Function, this, Stack);
	}

	//for the clients that do not have authority (over the Joint Instance) - use the player controller's function callspace if the option is enabled.
	if (!GetHostingJointInstance()->HasAuthority() && bUsePlayerControllerAsRPCFunctionCallspace)
	{
		//Use the PlayerController's function callspace.
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		
		if (PC)
		{
			return PC->GetFunctionCallspace(Function, Stack);
		}
	}
	
	return GetHostingJointInstance()->GetFunctionCallspace(Function, Stack);
	
#endif
	
	
}

void UJointNodeBase::SetHostingJointInstance(const TWeakObjectPtr<AJointActor>& InHostingJointInstance)
{
	HostingJointInstance = InHostingJointInstance;
}

AJointActor* UJointNodeBase::GetHostingJointInstance() const
{
	if (HostingJointInstance.IsValid()) return HostingJointInstance.Get();
	
	UJointManager* Manager = GetJointManager();
	
	if (!Manager) return nullptr;
	
	// Cache the instance for future use.
	HostingJointInstance = Manager->GetHostingJointActor();
	
	return HostingJointInstance.Get();
}

void UJointNodeBase::ReloadNode()
{
	if (!CanReloadNode()){

#if WITH_EDITOR
		UE_LOG(LogJoint, Error, TEXT("Joint: Tried to reload Joint node %s but it is not allowed to reload. Aborting the action... (If you want to make it playable multiple times,"), *this->GetName());
#endif
		
		return;
	}
	
	bIsNodeBegunPlay = false;
	bIsNodeEndedPlay = false;
	bIsNodePending = false;
}

void UJointNodeBase::NodeBeginPlay()
{
	//Don't play again if once played before.
	if (IsNodeBegunPlay()) return;

	bIsNodeBegunPlay = true;

	PreNodeBeginPlay();
	
	if (OnJointNodeBeginDelegate.IsBound())
	{
		OnJointNodeBeginDelegate.Broadcast(this);
	}
	
	GetHostingJointInstance()->NotifyNodeBeginPlay(this);

	PostNodeBeginPlay();

}

void UJointNodeBase::NodeEndPlay()
{
	//Don't end again if once ended before.
	if (IsNodeEndedPlay()) return;
	
	bIsNodeEndedPlay = true;

	MarkNodePendingByForce();
	
	PreNodeEndPlay();
	
	//Broadcast OnJointNodeEndDelegate Action.
	if (OnJointNodeEndDelegate.IsBound())
	{
		OnJointNodeEndDelegate.Broadcast(this);
	}
	
	GetHostingJointInstance()->NotifyNodeEndPlay(this);
	
	PostNodeEndPlay();
}

void UJointNodeBase::MarkNodePending()
{
	//Don't end again if once ended before.
	if (IsNodePending()) return;

	bIsNodePending = true;

	PreNodeMarkedAsPending();

	//Broadcast OnJointNodeMarkedAsPendingDelegate Action.
	if (OnJointNodeMarkedAsPendingDelegate.IsBound())
	{
		OnJointNodeMarkedAsPendingDelegate.Broadcast(this);
	}
	
	GetHostingJointInstance()->NotifyNodePending(this);

	PostNodeMarkedAsPending();
	
	if (ParentNode) ParentNode->MarkNodePendingIfNeeded();
}

const bool& UJointNodeBase::CanReloadNode() const
{
	return bCanReloadNode;
}

void UJointNodeBase::SubNodesBeginPlay()
{
	//TODO: Change it to work without iteration since it is quite messy when the debugger comes in.
	//Or maybe add asynchronous action for this. IDK.
	//Maybe debugger holds and store all instances that has been started while on the pause action and restart it when it resumes? I guess that is the best way to go now.

	//Propagate BeginPlay action to the children nodes. if it is not necessary or need some other actions for the node class, replace it with something else.
	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		SubNode->RequestNodeBeginPlay(GetHostingJointInstance());
	}
}


void UJointNodeBase::SubNodesEndPlay()
{
	//Propagate EndPlay action to the children nodes. if it is not necessary or need some other actions for the node class, replace it with something else.
	for (UJointNodeBase* SubNode : SubNodes)
	{
		if (!SubNode) continue;

		SubNode->RequestNodeEndPlay();
	}
}

void UJointNodeBase::RequestNodeBeginPlay(AJointActor* InHostingJointInstance)
{
	if (IsNodeBegunPlay()) return;

	if (!InHostingJointInstance) return;

	InHostingJointInstance->RequestNodeBeginPlay(this);
}

void UJointNodeBase::RequestNodeEndPlay()
{
	if (IsNodeEndedPlay()) return;

	TSoftObjectPtr<AJointActor> Instance = nullptr;

	Instance = GetHostingJointInstance();

	if (!Instance.IsValid()) return;

	Instance->RequestNodeEndPlay(this);
}

void UJointNodeBase::RequestReloadNode(const bool bPropagateToSubNodes)
{
	AJointActor* Instance = GetHostingJointInstance();

	if (!Instance) return;

	Instance->RequestReloadNode(this, bPropagateToSubNodes);
}

void UJointNodeBase::PreNodeBeginPlay_Implementation()
{
}


void UJointNodeBase::PostNodeBeginPlay_Implementation()
{
	SubNodes.Remove(nullptr);
	
	if (!SubNodes.IsEmpty())
	{
		//Play sub nodes if it has.

		SubNodesBeginPlay();
	}
	else
	{
		//End play if there is no action specified. To prevent this, you must override this function and implement the features you want for this node type.

		RequestNodeEndPlay();
	}
}

void UJointNodeBase::PreNodeEndPlay_Implementation()
{
}

void UJointNodeBase::PostNodeEndPlay_Implementation()
{
	SubNodesEndPlay();
}


void UJointNodeBase::PreNodeMarkedAsPending_Implementation()
{
}

TSoftObjectPtr<UJointBuildPreset> UJointNodeBase::GetBuildPreset()
{

	TSoftObjectPtr<UJointBuildPreset> Preset = nullptr;
	
#if WITH_EDITOR

	Preset = BuildPreset;

#endif

	return Preset;
}

void UJointNodeBase::SetBuildPreset(TSoftObjectPtr<UJointBuildPreset> NewBuildPreset)
{
	
#if WITH_EDITOR

	BuildPreset = NewBuildPreset;

#endif

}



#if WITH_EDITOR

void UJointNodeBase::NotifyParentGraph()
{
	if (!GetOuter())
	{
		return;
	}

	if (UJointManager* Parent = Cast<UJointManager>(GetOuter()))
	{
		Parent->JointGraph->NotifyGraphChanged();
	}
}

void UJointNodeBase::PostEditImport()
{
	UObject::PostEditImport();
}


void UJointNodeBase::PreEditChange(FProperty* PropertyAboutToChange)
{
	UObject::PreEditChange(PropertyAboutToChange);

	ensure(this);

	SubNodes.Remove(nullptr);

	this->MarkPackageDirty();

	this->GetOuter()->MarkPackageDirty();
}

void UJointNodeBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	ensure(this);

	SubNodes.Remove(nullptr);

	this->MarkPackageDirty();

	this->GetOuter()->MarkPackageDirty();

#if WITH_EDITORONLY_DATA
	if (this->PropertyChangedNotifiers.IsBound())
	{
		this->PropertyChangedNotifiers.Broadcast(PropertyChangedEvent,
		                                         PropertyChangedEvent.GetPropertyName().ToString());
	}
#endif
}

#endif

#undef LOCTEXT_NAMESPACE
