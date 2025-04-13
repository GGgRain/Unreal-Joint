//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "Misc/TextFilterExpressionEvaluator.h"

enum class EJointTreeFilterResult;
class IJointTreeItem;
class FTextFilterExpressionEvaluator;

/** Output struct for builders to use */
struct JOINTEDITOR_API FJointTreeBuilderOutput
{
	FJointTreeBuilderOutput(TArray<TSharedPtr<class IJointTreeItem>>& InItems, TArray<TSharedPtr<class IJointTreeItem>>& InLinearItems)
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
	TSharedPtr<class IJointTreeItem> Find(const FName& InName, TArrayView<const FName> InTypes) const;

	/** 
	 * Find the item with the specified name
	 * @param	InName	The item's name
	 * @param	InTypes	The types of items to search. If this is empty all items will be searched.
	 * @return the item found, or an invalid ptr if it was not found.
	 */
	FORCEINLINE TSharedPtr<class IJointTreeItem> Find(const FName& InName, std::initializer_list<FName> InTypes) const
	{
		return Find(InName, MakeArrayView(InTypes));
	}

	/** 
	 * Find the item with the specified name
	 * @param	InName	The item's name
	 * @param	InType	The type of items to search. If this is NAME_None all items will be searched.
	 * @return the item found, or an invalid ptr if it was not found.
	 */
	TSharedPtr<class IJointTreeItem> Find(const FName& InName, const FName& InType) const;

private:
	/** The items that are built by this builder */
	TArray<TSharedPtr<class IJointTreeItem>>& Items;

	/** A linearized list of all items in OutItems (for easier searching) */
	TArray<TSharedPtr<class IJointTreeItem>>& LinearItems;
};

/** Basic filter used when re-filtering the tree */
struct FJointPropertyTreeFilterArgs
{
	FJointPropertyTreeFilterArgs(TSharedPtr<FTextFilterExpressionEvaluator> InTextFilter)
		: TextFilter(InTextFilter)
		, bFlattenHierarchyOnFilter(true)
	{}

	/** The text filter we are using, if any */
	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;

	/** Whether to flatten the hierarchy so filtered items appear in a linear list */
	bool bFlattenHierarchyOnFilter;
};

/** Delegate used to filter an item. */
DECLARE_DELEGATE_RetVal_TwoParams(EJointTreeFilterResult, FOnFilterJointPropertyTreeItem, const FJointPropertyTreeFilterArgs& /*InArgs*/, const TSharedPtr<class IJointTreeItem>& /*InItem*/);

/** Interface to implement to provide custom build logic to skeleton trees */
class JOINTEDITOR_API IJointTreeBuilder
{
public:

	virtual ~IJointTreeBuilder() {};

	/** Setup this builder with links to the tree and preview scene */
	virtual void Initialize(const TSharedRef<class SJointTree>& InSkeletonTree, FOnFilterJointPropertyTreeItem InOnFilterSkeletonTreeItem) = 0;

	/**
	 * Build an array of skeleton tree items to display in the tree.
	 * @param	Output			The items that are built by this builder
	 */
	virtual void Build(FJointTreeBuilderOutput& Output) = 0;

	/** Apply filtering to the tree items */
	virtual void Filter(const FJointPropertyTreeFilterArgs& InArgs, const TArray<TSharedPtr<class IJointTreeItem>>& InItems, TArray<TSharedPtr<class IJointTreeItem>>& OutFilteredItems) = 0;

	/** Allows the builder to contribute to filtering an item */
	virtual EJointTreeFilterResult FilterItem(const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<class IJointTreeItem>& InItem) = 0;


	virtual bool IsShowingJointManagers() const = 0;
	
	virtual bool IsShowingNodes() const = 0;
	
	virtual bool IsShowingProperties() const = 0;
	
};


