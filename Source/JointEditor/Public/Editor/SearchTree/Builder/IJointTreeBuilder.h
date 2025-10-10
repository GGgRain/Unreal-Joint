//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraph.h"
#include "JointEdGraphNode.h"
#include "JointManager.h"
#include "Containers/ArrayView.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "Misc/EngineVersionComparison.h"


enum class EJointTreeFilterResult;
class IJointTreeItem;
class FTextFilterExpressionEvaluator;
class UJointEdGraphNode;

struct FJointTreeJointManagerInfo

{
	FJointTreeJointManagerInfo(TWeakObjectPtr<UJointManager> InManager)
		: JointManager(InManager)
	{
		if (JointManager.Get())
		{
			SortString = JointManager.Get()->GetName();
			SortNumber = 0;
			SortLength = SortString.Len();
		}

		// Split the bone name into string prefix and numeric suffix for sorting (different from FName to support leading zeros in the numeric suffix)
		int32 Index = SortLength - 1;
		for (int32 PlaceValue = 1; Index >= 0 && FChar::IsDigit(SortString[Index]); --Index, PlaceValue *= 10)
		{
			SortNumber += static_cast<int32>(SortString[Index] - '0') * PlaceValue;
		}
#if UE_VERSION_OLDER_THAN(5, 5, 0)
		SortString.LeftInline(Index + 1, false);
#else
		SortString.LeftInline(Index + 1, EAllowShrinking::No);
#endif
	}

	bool operator<(const FJointTreeJointManagerInfo& RHS)
	{
		// Sort alphabetically by string prefix
		if (int32 SplitNameComparison = SortString.Compare(RHS.SortString)) { return SplitNameComparison < 0; }

		// Sort by number if the string prefixes match
		if (SortNumber != RHS.SortNumber) { return SortNumber < RHS.SortNumber; }

		// Sort by length to give us the equivalent to alphabetical sorting if the numbers match (which gives us the following sort order: bone_, bone_0, bone_00, bone_000, bone_001, bone_01, bone_1, etc)
		return (SortNumber == 0) ? SortLength < RHS.SortLength : SortLength > RHS.SortLength;
	}

	TWeakObjectPtr<UJointManager> JointManager;

	FString SortString;
	int32 SortNumber = 0;
	int32 SortLength = 0;
};

struct FJointTreeGraphInfo
{
	FJointTreeGraphInfo(UJointEdGraph* InGraph)
		: Graph(InGraph)
	{
		if (Graph)
		{
			SortString = Graph->GetName();
			SortNumber = 0;
			SortLength = SortString.Len();
		}

		// Split the bone name into string prefix and numeric suffix for sorting (different from FName to support leading zeros in the numeric suffix)
		int32 Index = SortLength - 1;
		for (int32 PlaceValue = 1; Index >= 0 && FChar::IsDigit(SortString[Index]); --Index, PlaceValue *= 10)
		{
			SortNumber += static_cast<int32>(SortString[Index] - '0') * PlaceValue;
		}
#if UE_VERSION_OLDER_THAN(5, 5, 0)
		SortString.LeftInline(Index + 1, false);
#else
		SortString.LeftInline(Index + 1, EAllowShrinking::No);
#endif
	}

	bool operator<(const FJointTreeGraphInfo& RHS)
	{
		// Sort alphabetically by string prefix
		if (int32 SplitNameComparison = SortString.Compare(RHS.SortString)) { return SplitNameComparison < 0; }

		// Sort by number if the string prefixes match
		if (SortNumber != RHS.SortNumber) { return SortNumber < RHS.SortNumber; }

		// Sort by length to give us the equivalent to alphabetical sorting if the numbers match (which gives us the following sort order: bone_, bone_0, bone_00, bone_000, bone_001, bone_01, bone_1, etc)
		return (SortNumber == 0) ? SortLength < RHS.SortLength : SortLength > RHS.SortLength;
	}

	UJointEdGraph* Graph;

	FString SortString;
	int32 SortNumber = 0;
	int32 SortLength = 0;
};

struct FJointTreeNodeInfo
{
	FJointTreeNodeInfo(UEdGraphNode* InEditorNode)
		: EditorNode(InEditorNode)
	{
		if (InEditorNode)
		{
			if (UJointEdGraphNode* JointNode = Cast<UJointEdGraphNode>(InEditorNode))
			{
				SortString = JointNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
			}
			else
			{
				SortString = EditorNode->GetName();
			}
			SortNumber = 0;
			SortLength = SortString.Len();
		}

		// Split the bone name into string prefix and numeric suffix for sorting (different from FName to support leading zeros in the numeric suffix)
		int32 Index = SortLength - 1;
		for (int32 PlaceValue = 1; Index >= 0 && FChar::IsDigit(SortString[Index]); --Index, PlaceValue *= 10)
		{
			SortNumber += static_cast<int32>(SortString[Index] - '0') * PlaceValue;
		}
#if UE_VERSION_OLDER_THAN(5, 5, 0)
		SortString.LeftInline(Index + 1, false);
#else
		SortString.LeftInline(Index + 1, EAllowShrinking::No);
#endif
	}

	bool operator<(const FJointTreeNodeInfo& RHS)
	{
		// Sort alphabetically by string prefix
		if (int32 SplitNameComparison = SortString.Compare(RHS.SortString)) { return SplitNameComparison < 0; }

		// Sort by number if the string prefixes match
		if (SortNumber != RHS.SortNumber) { return SortNumber < RHS.SortNumber; }

		// Sort by length to give us the equivalent to alphabetical sorting if the numbers match (which gives us the following sort order: bone_, bone_0, bone_00, bone_000, bone_001, bone_01, bone_1, etc)
		return (SortNumber == 0) ? SortLength < RHS.SortLength : SortLength > RHS.SortLength;
	}

	UEdGraphNode* EditorNode;

	FString SortString;
	int32 SortNumber = 0;
	int32 SortLength = 0;
};


struct FJointTreePropertyInfo
{
	FJointTreePropertyInfo(FProperty* InProperty, UObject* InPropertyOwnerObject, UObject* TreeItemOwnerObject = nullptr)
		: Property(InProperty), Object(InPropertyOwnerObject), TreeItemOwnerObject(TreeItemOwnerObject)
	{
		if (InPropertyOwnerObject)
		{
			SortString = InPropertyOwnerObject->GetName();
			SortNumber = 0;
			SortLength = SortString.Len();
		}

		// Split the bone name into string prefix and numeric suffix for sorting (different from FName to support leading zeros in the numeric suffix)
		int32 Index = SortLength - 1;
		for (int32 PlaceValue = 1; Index >= 0 && FChar::IsDigit(SortString[Index]); --Index, PlaceValue *= 10)
		{
			SortNumber += static_cast<int32>(SortString[Index] - '0') * PlaceValue;
		}
#if UE_VERSION_OLDER_THAN(5, 5, 0)
		SortString.LeftInline(Index + 1, false);
#else
		SortString.LeftInline(Index + 1, EAllowShrinking::No);
#endif
	}

	bool operator<(const FJointTreePropertyInfo& RHS)
	{
		// Sort alphabetically by string prefix
		if (int32 SplitNameComparison = SortString.Compare(RHS.SortString)) { return SplitNameComparison < 0; }

		// Sort by number if the string prefixes match
		if (SortNumber != RHS.SortNumber) { return SortNumber < RHS.SortNumber; }

		// Sort by length to give us the equivalent to alphabetical sorting if the numbers match (which gives us the following sort order: bone_, bone_0, bone_00, bone_000, bone_001, bone_01, bone_1, etc)
		return (SortNumber == 0) ? SortLength < RHS.SortLength : SortLength > RHS.SortLength;
	}

	FProperty* Property;
	UObject* Object;
	UObject* TreeItemOwnerObject;

	FString SortString;
	int32 SortNumber = 0;
	int32 SortLength = 0;
};




/** Output struct for builders to use */
struct JOINTEDITOR_API FJointTreeBuilderOutput
{
	FJointTreeBuilderOutput() {}
	
	FJointTreeBuilderOutput(TArray<TSharedPtr<class IJointTreeItem>> InItems, TArray<TSharedPtr<class IJointTreeItem>> InLinearItems)
		: Items(InItems)
		, LinearItems(InLinearItems)
	{}

	/** 
	 * Add an item to the output
	 * @param	InItem			The item to add
	 * @param	InParentName	The name of the item's parent
	 * @param	InParentTypes	The types of items to search. If this is empty all items will be searched.
	 * @param	bAddToHead		Whether to add the item to the start or end of the parent's children array
	 */
	void Add(const TSharedPtr<class IJointTreeItem>& InItem, const FName& InParentName, TArrayView<const FName> InParentTypes, bool bAddToHead = false);

	/** 
	 * Add an item to the output
	 * @param	InItem			The item to add
	 * @param	InParentName	The name of the item's parent
	 * @param	InParentTypes	The types of items to search. If this is empty all items will be searched.
	 * @param	bAddToHead		Whether to add the item to the start or end of the parent's children array
	 */
	FORCEINLINE void Add(const TSharedPtr<class IJointTreeItem>& InItem, const FName& InParentName, std::initializer_list<FName> InParentTypes, bool bAddToHead = false)
	{
		Add(InItem, InParentName, MakeArrayView(InParentTypes), bAddToHead);
	}

	/** 
	 * Add an item to the output
	 * @param	InItem			The item to add
	 * @param	InParentName	The name of the item's parent
	 * @param	InParentType	The type of items to search. If this is NAME_None all items will be searched.
	 * @param	bAddToHead		Whether to add the item to the start or end of the parent's children array
	 */
	void Add(const TSharedPtr<class IJointTreeItem>& InItem, const FName& InParentName, const FName& InParentType, bool bAddToHead = false);

	/** 
	 * Find the item with the specified name
	 * @param	InName	The item's name
	 * @param	InTypes	The types of items to search. If this is empty all items will be searched.
	 * @return the item found, or an invalid ptr if it was not found.
	 */
	TSharedPtr<class IJointTreeItem> Find(const FName& InName, TArrayView<const FName> InTypes);

	/** 
	 * Find the item with the specified name
	 * @param	InName	The item's name
	 * @param	InTypes	The types of items to search. If this is empty all items will be searched.
	 * @return the item found, or an invalid ptr if it was not found.
	 */
	FORCEINLINE TSharedPtr<class IJointTreeItem> Find(const FName& InName, std::initializer_list<FName> InTypes)
	{
		return Find(InName, MakeArrayView(InTypes));
	}

	/** 
	 * Find the item with the specified name
	 * @param	InName	The item's name
	 * @param	InType	The type of items to search. If this is NAME_None all items will be searched.
	 * @return the item found, or an invalid ptr if it was not found.
	 */
	TSharedPtr<class IJointTreeItem> Find(const FName& InName, const FName& InType);

public:

	// Map of item names to items for fast searching - it will be used to recycling items when rebuilding the tree as well.
	TMap<FName, TSharedPtr<class IJointTreeItem>> ItemMap;

public:
	
	/** The items that are built by this builder */
	TArray<TSharedPtr<class IJointTreeItem>> Items;

	/** A linearized list of all items in OutItems (for easier searching) */
	TArray<TSharedPtr<class IJointTreeItem>> LinearItems;
};

/** Basic filter used when re-filtering the tree */
struct FJointPropertyTreeFilterArgs
{
	FJointPropertyTreeFilterArgs() : bFlattenHierarchyOnFilter(false)
	{
	}

	/** The text filter we are using, if any */
	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;

	/** Whether to flatten the hierarchy so filtered items appear in a linear list */
	bool bFlattenHierarchyOnFilter;

	//The graphs to be shown. If empty all graphs will be shown
	TArray<UEdGraph*> GraphsToShow;
	
};

/** Delegate used to filter an item. */
DECLARE_DELEGATE_RetVal_TwoParams(EJointTreeFilterResult, FOnFilterJointPropertyTreeItem, const FJointPropertyTreeFilterArgs& /*InArgs*/, const TSharedPtr<class IJointTreeItem>& /*InItem*/);

DECLARE_DELEGATE(FOnJointTreeBuildStarted);
DECLARE_DELEGATE_OneParam(FOnJointTreeBuildFinished, const FJointTreeBuilderOutput);
DECLARE_DELEGATE(FOnJointTreeBuildCancelled);


/** Interface to implement to provide custom build logic to skeleton trees */
class JOINTEDITOR_API IJointTreeBuilder
{
	
public:

	virtual ~IJointTreeBuilder();

	/** Setup this builder with links to the tree and preview scene */
	virtual void Initialize(const TSharedRef<class SJointTree>& InTree, FOnFilterJointPropertyTreeItem InOnFilterSkeletonTreeItem) = 0;

	/**
	 * Build an array of skeleton tree items to display in the tree.
	 * @param	Output			The items that are built by this builder
	 */
	virtual void Build(FJointTreeBuilderOutput& Output) = 0;

	/** Apply filtering to the tree items */
	virtual void Filter(const FJointPropertyTreeFilterArgs& InArgs, const TArray<TSharedPtr<class IJointTreeItem>>& InItems, TArray<TSharedPtr<class IJointTreeItem>>& OutFilteredItems) = 0;

	/** Allows the builder to contribute to filtering an item */
	virtual EJointTreeFilterResult FilterItem(const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<class IJointTreeItem>& InItem) = 0;
	
public:


	virtual void SetShouldAbandonBuild(bool bInShouldAbandonBuild) = 0;

	virtual const bool GetShouldAbandonBuild() const = 0;
	
};

