//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointManager.h"

#include "JointActor.h"
#include "Node/JointFragment.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "Node/JointNodeBase.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectIterator.h"

UJointManager::UJointManager()
{
	
}

AJointActor* UJointManager::GetHostingJointActor() const
{
	// cast outer to Joint actor
	return GetOuter() ? Cast<AJointActor>(GetOuter()) : nullptr;
}

UJointNodeBase* UJointManager::FindBaseNodeWithGuid(FGuid NodeGuid) const
{
	for (UJointNodeBase* Node : Nodes)
	{
		if (Node == nullptr) continue;

		if (Node->NodeGuid == NodeGuid) return Node;
	}

	return nullptr;
}


UJointFragment* UJointManager::FindFragmentWithGuid(FGuid NodeGuid) const
{
	TArray<UJointFragment*> AllManagerFragments = GetAllManagerFragmentsOnLowerHierarchy();

	for (UJointFragment* ManagerFragment : AllManagerFragments)
	{
		if (ManagerFragment == nullptr) continue;

		if (ManagerFragment->NodeGuid == NodeGuid) return ManagerFragment;
	}

	for (UJointNodeBase* Node : Nodes)
	{
		if (Node == nullptr) continue;

		TArray<UJointFragment*> SubNodes = Node->GetAllFragmentsOnLowerHierarchy();

		for (UJointFragment* SubNode : SubNodes)
		{
			if (SubNode == nullptr) continue;

			if (SubNode->NodeGuid == NodeGuid) return SubNode;
		}
	}

	return nullptr;
}

UJointFragment* UJointManager::FindManagerFragmentByClass(TSubclassOf<UJointFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UJointNodeBase* Fragment : ManagerFragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass)) { return Cast<UJointFragment>(Fragment); }
		}
	}

	return nullptr;
}


const TArray<UJointFragment*> UJointManager::FindManagerFragmentsByClass(
	TSubclassOf<UJointFragment> FragmentClass) const
{
	TArray<UJointFragment*> FinalArray;

	if (FragmentClass != nullptr)
	{
		for (UJointNodeBase* Fragment : ManagerFragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass)) { FinalArray.Add(Cast<UJointFragment>(Fragment)); }
		}
	}

	return FinalArray;
}

const TArray<UJointFragment*> UJointManager::GetAllManagerFragments() const
{
	TArray<UJointFragment*> FinalArray;

	for (UJointNodeBase* Fragment : ManagerFragments)
	{
		if (Fragment && Fragment->GetClass()->IsChildOf(UJointFragment::StaticClass()))
		{
			FinalArray.Add(Cast<UJointFragment>(Fragment));
		}
	}


	return FinalArray;
}

UJointFragment* UJointManager::FindManagerFragmentByClassOnLowerHierarchy(
	TSubclassOf<UJointFragment> FragmentClass) const
{
	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if(!ManagerFragment) continue;

		if(UJointFragment* CastedManagerFragment = Cast<UJointFragment>(ManagerFragment))
		{
			if (FragmentClass != nullptr && CastedManagerFragment->GetClass() == FragmentClass) return CastedManagerFragment; //It intentionally returns nothing.
		}
		
		if (UJointFragment* FoundNode = UJointNodeBase::IterateAndGetTheFirstFragmentForClassUnderNode(ManagerFragment, FragmentClass)) return FoundNode;
	}

	return nullptr;
}

const TArray<UJointFragment*> UJointManager::FindManagerFragmentsByClassOnLowerHierarchy(
	TSubclassOf<UJointFragment> FragmentClass) const
{
	TArray<UJointFragment*> Fragments;

	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if(!ManagerFragment) continue;

		if(UJointFragment* CastedManagerFragment = Cast<UJointFragment>(ManagerFragment))
		{
			if (FragmentClass != nullptr)
			{
				if (CastedManagerFragment->GetClass() == FragmentClass) Fragments.Add(CastedManagerFragment);
			}
			else
			{
				Fragments.Add(CastedManagerFragment);
			}
		}
		
		UJointNodeBase::IterateAndCollectAllFragmentsUnderNode(ManagerFragment, Fragments, FragmentClass);
	}

	return Fragments;
}

const TArray<UJointFragment*> UJointManager::GetAllManagerFragmentsOnLowerHierarchy() const
{
	TArray<UJointFragment*> Fragments;

	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if(!ManagerFragment) continue;

		if(UJointFragment* CastedManagerFragment = Cast<UJointFragment>(ManagerFragment))
		{
			Fragments.Add(CastedManagerFragment);
		}
		
		UJointNodeBase::IterateAndCollectAllFragmentsUnderNode(ManagerFragment, Fragments, nullptr);
	}

	return Fragments;
}

UJointFragment* UJointManager::FindManagerFragmentWithTagOnLowerHierarchy(FGameplayTag InNodeTag,
	const bool bExact)
{
	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if(!ManagerFragment) continue;

		TArray<UJointFragment*> IterFragments;

		if(UJointFragment* CastedManagerFragment = Cast<UJointFragment>(ManagerFragment))
		{
			IterFragments.Add(CastedManagerFragment);
		}
		
		UJointNodeBase::IterateAndCollectAllFragmentsUnderNode(ManagerFragment, IterFragments, nullptr);

		for (UJointFragment* IterFragment : IterFragments)
		{
			if (IterFragment == nullptr) continue;

			if (bExact ? IterFragment->NodeTags.HasTagExact(InNodeTag) : IterFragment->NodeTags.HasTag(InNodeTag))
				return IterFragment;
		}
	}

	return nullptr;
}

TArray<UJointFragment*> UJointManager::FindManagerFragmentsWithTagOnLowerHierarchy(FGameplayTag InNodeTag,
	const bool bExact)
{

	TArray<UJointFragment*> OutFragments;
	
	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if(!ManagerFragment) continue;

		TArray<UJointFragment*> IterFragments;

		if(UJointFragment* CastedManagerFragment = Cast<UJointFragment>(ManagerFragment))
		{
			IterFragments.Add(CastedManagerFragment);
		}
		
		UJointNodeBase::IterateAndCollectAllFragmentsUnderNode(ManagerFragment, IterFragments, nullptr);

		for (UJointFragment* IterFragment : IterFragments)
		{
			if (IterFragment == nullptr) continue;

			if (bExact ? IterFragment->NodeTags.HasTagExact(InNodeTag) : IterFragment->NodeTags.HasTag(InNodeTag)) OutFragments.Add(IterFragment);
		}
	}

	return OutFragments;
}

UJointFragment* UJointManager::FindManagerFragmentWithAnyTagsOnLowerHierarchy(
	FGameplayTagContainer InNodeTagContainer, const bool bExact)
{
	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if(!ManagerFragment) continue;

		TArray<UJointFragment*> IterFragments;

		if(UJointFragment* CastedManagerFragment = Cast<UJointFragment>(ManagerFragment))
		{
			IterFragments.Add(CastedManagerFragment);
		}

		UJointNodeBase::IterateAndCollectAllFragmentsUnderNode(ManagerFragment, IterFragments, nullptr);

		for (UJointFragment* IterFragment : IterFragments)
		{
			if (IterFragment == nullptr) continue;

			if (bExact
				    ? IterFragment->NodeTags.HasAnyExact(InNodeTagContainer)
				    : IterFragment->NodeTags.HasAny(InNodeTagContainer)) return IterFragment;
		}
	}

	return nullptr;
}

TArray<UJointFragment*> UJointManager::FindManagerFragmentsWithAnyTagsOnLowerHierarchy(
	FGameplayTagContainer InNodeTagContainer, const bool bExact)
{
	TArray<UJointFragment*> OutFragments;

	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if(!ManagerFragment) continue;

		TArray<UJointFragment*> IterFragments;

		if(UJointFragment* CastedManagerFragment = Cast<UJointFragment>(ManagerFragment))
		{
			IterFragments.Add(CastedManagerFragment);
		}

		UJointNodeBase::IterateAndCollectAllFragmentsUnderNode(ManagerFragment, IterFragments, nullptr);

		for (UJointFragment* IterFragment : IterFragments)
		{
			if (IterFragment == nullptr) continue;

			if (bExact
					? IterFragment->NodeTags.HasAnyExact(InNodeTagContainer)
					: IterFragment->NodeTags.HasAny(InNodeTagContainer)) OutFragments.Add(IterFragment);
		}
	}

	return OutFragments;
}

UJointFragment* UJointManager::FindManagerFragmentWithAllTagsOnLowerHierarchy(
	FGameplayTagContainer InNodeTagContainer, const bool bExact)
{
	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if(!ManagerFragment) continue;

		TArray<UJointFragment*> IterFragments;

		if(UJointFragment* CastedManagerFragment = Cast<UJointFragment>(ManagerFragment))
		{
			IterFragments.Add(CastedManagerFragment);
		}

		UJointNodeBase::IterateAndCollectAllFragmentsUnderNode(ManagerFragment, IterFragments, nullptr);

		for (UJointFragment* IterFragment : IterFragments)
		{
			if (IterFragment == nullptr) continue;

			if (bExact
				    ? IterFragment->NodeTags.HasAll(InNodeTagContainer)
				    : IterFragment->NodeTags.HasAllExact(InNodeTagContainer)) return IterFragment;
		}
	}

	return nullptr;
}

TArray<UJointFragment*> UJointManager::FindManagerFragmentsWithAllTagsOnLowerHierarchy(
	FGameplayTagContainer InNodeTagContainer, const bool bExact)
{

	TArray<UJointFragment*> OutFragments;
	
	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if(!ManagerFragment) continue;

		TArray<UJointFragment*> IterFragments;

		if(UJointFragment* CastedManagerFragment = Cast<UJointFragment>(ManagerFragment))
		{
			IterFragments.Add(CastedManagerFragment);
		}

		UJointNodeBase::IterateAndCollectAllFragmentsUnderNode(ManagerFragment, IterFragments, nullptr);

		for (UJointFragment* IterFragment : IterFragments)
		{
			if (IterFragment == nullptr) continue;

			if (bExact
					? IterFragment->NodeTags.HasAll(InNodeTagContainer)
					: IterFragment->NodeTags.HasAllExact(InNodeTagContainer)) OutFragments.Add(IterFragment);
		}
	}

	return OutFragments;
}

UJointFragment* UJointManager::FindManagerFragmentWithTag(FGameplayTag InNodeTag, const bool bExact)
{
	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if (ManagerFragment == nullptr) continue;

		if (bExact ? !ManagerFragment->NodeTags.HasTagExact(InNodeTag) : !ManagerFragment->NodeTags.HasTag(InNodeTag))
			continue;

		if (UJointFragment* CastedFragment = Cast<UJointFragment>(ManagerFragment)) return CastedFragment;
	}

	return nullptr;
}

TArray<UJointFragment*> UJointManager::FindManagerFragmentsWithTag(FGameplayTag InNodeTag, const bool bExact)
{
	TArray<UJointFragment*> OutFragments;

	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if (ManagerFragment == nullptr) continue;

		if (bExact ? !ManagerFragment->NodeTags.HasTagExact(InNodeTag) : !ManagerFragment->NodeTags.HasTag(InNodeTag))
			continue;

		if (UJointFragment* CastedFragment = Cast<UJointFragment>(ManagerFragment)) OutFragments.Add(CastedFragment);
	}

	return OutFragments;
}

UJointFragment* UJointManager::FindManagerFragmentWithAnyTags(FGameplayTagContainer InNodeTagContainer,
                                                                    const bool bExact)
{
	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if (ManagerFragment == nullptr) continue;

		if (bExact ? !ManagerFragment->NodeTags.HasAnyExact(InNodeTagContainer) : !ManagerFragment->NodeTags.HasAny(InNodeTagContainer))
			continue;

		if (UJointFragment* CastedFragment = Cast<UJointFragment>(ManagerFragment)) return CastedFragment;
	}

	return nullptr;
}

TArray<UJointFragment*> UJointManager::FindManagerFragmentsWithAnyTags(FGameplayTagContainer InNodeTagContainer,
                                                                             const bool bExact)
{
	TArray<UJointFragment*> OutFragments;

	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if (ManagerFragment == nullptr) continue;

		if (bExact ? !ManagerFragment->NodeTags.HasAnyExact(InNodeTagContainer) : !ManagerFragment->NodeTags.HasAny(InNodeTagContainer))
			continue;

		if (UJointFragment* CastedFragment = Cast<UJointFragment>(ManagerFragment)) OutFragments.Add(CastedFragment);
	}

	return OutFragments;
}

UJointFragment* UJointManager::FindManagerFragmentWithAllTags(FGameplayTagContainer InNodeTagContainer,
                                                                    const bool bExact)
{
	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if (ManagerFragment == nullptr) continue;

		if (bExact ? !ManagerFragment->NodeTags.HasAllExact(InNodeTagContainer) : !ManagerFragment->NodeTags.HasAll(InNodeTagContainer))
			continue;

		if (UJointFragment* CastedFragment = Cast<UJointFragment>(ManagerFragment)) return CastedFragment;
	}

	return nullptr;
}

TArray<UJointFragment*> UJointManager::FindManagerFragmentsWithAllTags(FGameplayTagContainer InNodeTagContainer,
                                                                             const bool bExact)
{
	TArray<UJointFragment*> OutFragments;

	for (UJointNodeBase* ManagerFragment : ManagerFragments)
	{
		if (ManagerFragment == nullptr) continue;

		if (bExact ? !ManagerFragment->NodeTags.HasAllExact(InNodeTagContainer) : !ManagerFragment->NodeTags.HasAll(InNodeTagContainer))
			continue;

		if (UJointFragment* CastedFragment = Cast<UJointFragment>(ManagerFragment)) OutFragments.Add(CastedFragment);
	}

	return OutFragments;
}


#if WITH_EDITOR

void UJointManager::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	ensure(this);

	this->MarkPackageDirty();

	this->GetOuter()->MarkPackageDirty();

	// Broadcast an object property changed event to update the JointTreeViewer
	FPropertyChangedEvent EmptyPropertyChangedEvent(nullptr);
	FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(this, EmptyPropertyChangedEvent);
}

void UJointManager::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ensure(this);

	this->MarkPackageDirty();

	this->GetOuter()->MarkPackageDirty();

	// Broadcast an object property changed event to update the JointTreeViewer
	FPropertyChangedEvent EmptyPropertyChangedEvent(nullptr);
	FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(this, EmptyPropertyChangedEvent);
}

#endif

void UJointManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(UJointManager, Nodes)
}

bool UJointManager::IsSupportedForNetworking() const
{
	return true;
}

bool UJointManager::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = false;

	for (UJointNodeBase* Node : Nodes)
	{
		if (IsValid(Node))
		{
			WroteSomething |= Channel->ReplicateSubobject(Node, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

UWorld* UJointManager::GetWorld() const
{
	if (GetOuter())
	{
		return GetOuter()->GetWorld();
	}
	
	return nullptr;
}

void UJointManager::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);
}
