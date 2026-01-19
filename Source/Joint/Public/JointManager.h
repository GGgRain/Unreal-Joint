//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Net/DataBunch.h"
#include "Templates/SubclassOf.h"
#include "Engine/EngineTypes.h"
#include "Engine/Blueprint.h"
#include "JointManager.generated.h"

//An asset class for storaging data and some functions.


class AJointActor;
class UDataTable;
class UJointNodeBase;
class UJointFragment;

class UActorChannel;
class UEdGraph;

UCLASS(Blueprintable)
class JOINT_API UJointManager : public UObject
{
	GENERATED_BODY()

public:
	UJointManager();

public:
	/**
	 * The node that has been connected with the start pin of the Joint manager.
	 * Change this property to make it start from a specific node.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Data")
	TArray<TObjectPtr<UJointNodeBase>> StartNodes;

	/**
	 * The nodes this Joint manager contains.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Data")
	TArray<TObjectPtr<UJointNodeBase>> Nodes;

public:
	
	UPROPERTY(VisibleAnywhere, Category = "Data")
	TArray<TObjectPtr<UJointNodeBase>> ManagerFragments;

public:
	
	/**
	 * Return the Joint actor that is hosting this Joint manager.
	 */
	UFUNCTION(BlueprintPure, Category = "Joint")
	AJointActor* GetHostingJointActor() const;
	
public:
	/**
	 * Find a node with a given node Guid.
	 * This will look for the nodes that are contained at the Nodes array.
	 * @param NodeGuid The node Guid to search.
	 * @return Found node for the class.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	UJointNodeBase* FindBaseNodeWithGuid(FGuid NodeGuid) const;


	/**
	 * Find a fragment with a given node Guid.
	 * This will look for the whole node's hierarchy,
	 * It includes the manager fragments.
	 * @param NodeGuid The node Guid to search.
	 * @return Found node for the class.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	class UJointFragment* FindFragmentWithGuid(FGuid NodeGuid) const;

public:
	/**
	 * Find a fragment with a given class.
	 * This function will look for the fragments that are directly attached to the manager only.
	 * @param FragmentClass Provided class for the search action.
	 * @return Found fragment for the class.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment", meta=(DeterminesOutputType="FragmentClass"))
	class UJointFragment* FindManagerFragmentByClass(TSubclassOf<class UJointFragment> FragmentClass) const;

	/**
	 * Find all fragment with a given class.
	 * This function will look for the fragments that are directly attached to the manager only.
	 * @param FragmentClass Provided class for the search action.
	 * @return Found fragments for the class.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment", meta=(DeterminesOutputType="FragmentClass"))
	const TArray<class UJointFragment*> FindManagerFragmentsByClass(TSubclassOf<class UJointFragment> FragmentClass) const;

	/**
	 * Get all fragments attached on this node.
	 * This function will look for the fragments that are directly attached to the manager only.
	 * @return Found fragments.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	const TArray<class UJointFragment*> GetAllManagerFragments() const;

	/**
	 * Find a fragment with a given class.
	 * This function searches for the whole hierarchy of the manager node (root node), which means, it includes a sub node's sub nodes' sub nodes.
	 * @param FragmentClass Provided class for the search action
	 * @return Found fragment for the class.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment", meta=(DeterminesOutputType="FragmentClass"))
	class UJointFragment* FindManagerFragmentByClassOnLowerHierarchy(TSubclassOf<class UJointFragment> FragmentClass) const;

	/**
	 * Find all fragment with a given class.
	 * This function searches for the whole hierarchy of the manager node (root node), which means, it includes a sub node's sub nodes' sub nodes.
	 * @param FragmentClass Provided class for the search action
	 * @return Found fragments for the class
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment", meta=(DeterminesOutputType="FragmentClass"))
	const TArray<class UJointFragment*> FindManagerFragmentsByClassOnLowerHierarchy(
		TSubclassOf<class UJointFragment> FragmentClass) const;

	/**
	 * Get all fragments attached on this node.
	 * This function searches for the whole hierarchy of the manager node (root node), which means, it includes a sub node's sub nodes' sub nodes.
	 * @return Found fragments
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	const TArray<class UJointFragment*> GetAllManagerFragmentsOnLowerHierarchy() const;

public:
	/**
	 * Find a fragment by the provided tag while iterating through the lower hierarchy.
	 * @param InNodeTag The tag to search.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	class UJointFragment* FindManagerFragmentWithTagOnLowerHierarchy(FGameplayTag InNodeTag, const bool bExact = false);

	/**
	 * Find fragments by the provided tag while iterating through the lower hierarchy.
	 * @param InNodeTag The tag to search.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<class UJointFragment*> FindManagerFragmentsWithTagOnLowerHierarchy(FGameplayTag InNodeTag, const bool bExact = false);

	/**
	 * Find a fragment by the provided tags while iterating through the lower hierarchy.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	class UJointFragment* FindManagerFragmentWithAnyTagsOnLowerHierarchy(FGameplayTagContainer InNodeTagContainer,
	                                                           const bool bExact = false);

	/**
	 * Find fragments by the provided tags while iterating through the lower hierarchy.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<class UJointFragment*> FindManagerFragmentsWithAnyTagsOnLowerHierarchy(FGameplayTagContainer InNodeTagContainer,
	                                                                    const bool bExact = false);

	/**
	 * Find a fragment by the provided tags while iterating through the lower hierarchy.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	class UJointFragment* FindManagerFragmentWithAllTagsOnLowerHierarchy(FGameplayTagContainer InNodeTagContainer,
	                                                           const bool bExact = false);

	/**
	 * Find fragments by the provided tags while iterating through the lower hierarchy.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has all the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<class UJointFragment*> FindManagerFragmentsWithAllTagsOnLowerHierarchy(FGameplayTagContainer InNodeTagContainer,
	                                                                    const bool bExact = false);
public:
	
	
	/**
	 * Find a fragment by the provided tag.
	 * @param InNodeTag The tag to search.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	class UJointFragment* FindManagerFragmentWithTag(FGameplayTag InNodeTag, const bool bExact = false);

	/**
	 * Find fragments by the provided tag.
	 * @param InNodeTag The tag to search.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<class UJointFragment*> FindManagerFragmentsWithTag(FGameplayTag InNodeTag, const bool bExact = false);

	/**
	 * Find a fragment by the provided tags.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	class UJointFragment* FindManagerFragmentWithAnyTags(FGameplayTagContainer InNodeTagContainer,
	                                                           const bool bExact = false);

	/**
	 * Find fragments by the provided tags.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<class UJointFragment*> FindManagerFragmentsWithAnyTags(FGameplayTagContainer InNodeTagContainer,
	                                                                    const bool bExact = false);

	/**
	 * Find a fragment by the provided tags.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has any of the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragment for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	class UJointFragment* FindManagerFragmentWithAllTags(FGameplayTagContainer InNodeTagContainer,
	                                                           const bool bExact = false);

	/**
	 * Find fragments by the provided tags.
	 * @param InNodeTagContainer The tags to search. It will return all the nodes that has all the matching tags with the provided tags.
	 * @param bExact whether to force exact.
	 * @return Found fragments for the tag.
	 */
	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<class UJointFragment*> FindManagerFragmentsWithAllTags(FGameplayTagContainer InNodeTagContainer,
	                                                                    const bool bExact = false);
	
public:
#if WITH_EDITORONLY_DATA

	/** Editor Graph Data for Joint Manager. */
	UPROPERTY(VisibleAnywhere, Category="Editor")
	TObjectPtr<class UEdGraph> JointGraph;

	UPROPERTY()
	TArray<FEditedDocumentInfo> LastEditedDocuments;
	
#endif

#if WITH_EDITOR

	template<typename T=UEdGraph>
	T* GetJointGraphAs() const
	{
		if (!JointGraph) return nullptr;
		return Cast<T>(JointGraph);
	}
	
#endif
	
#if WITH_EDITORONLY_DATA

	//Editor Compile related.
	
	/** The mode that will be used when compiling this class. */
	//UPROPERTY(EditAnywhere, Category=ClassOptions, AdvancedDisplay)
	//EBlueprintCompileMode CompileMode;

	/** The current status of this Joint */
	UPROPERTY(transient)
	TEnumAsByte<enum EBlueprintStatus> Status;

#endif

#if WITH_EDITOR

	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

public:
	
	//Networking related.

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsSupportedForNetworking() const override;

	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags);
	
public:
	
	virtual UWorld* GetWorld() const override;

public:

	virtual void Serialize(FArchive& Ar) override;
	
};
