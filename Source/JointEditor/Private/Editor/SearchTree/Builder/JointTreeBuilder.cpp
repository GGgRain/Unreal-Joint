//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "SearchTree/Builder/JointTreeBuilder.h"

#include "Item/JointTreeItem_Property.h"
#include "SearchTree/Item/JointTreeItem_Manager.h"
#include "SearchTree/Item/JointTreeItem_Node.h"
#include "SearchTree/Item/IJointTreeItem.h"
#include "SearchTree/Slate/SJointTree.h"

#include "Node/JointFragment.h"
#include "Node/JointNodeBase.h"

#include "Misc/EngineVersionComparison.h"

#define LOCTEXT_NAMESPACE "FJointTreeBuilder"

void FJointTreeBuilderOutput::Add(const TSharedPtr<class IJointTreeItem>& InItem,
                                             const FName& InParentName, const FName& InParentType, bool bAddToHead)
{
	Add(InItem, InParentName, TArray<FName, TInlineAllocator<1>>({InParentType}), bAddToHead);
}

void FJointTreeBuilderOutput::Add(const TSharedPtr<class IJointTreeItem>& InItem,
                                             const FName& InParentName, TArrayView<const FName> InParentTypes,
                                             bool bAddToHead)
{
	TSharedPtr<IJointTreeItem> ParentItem = Find(InParentName, InParentTypes);
	if (ParentItem.IsValid())
	{
		InItem->SetParent(ParentItem);

		if (bAddToHead) { ParentItem->GetChildren().Insert(InItem, 0); }
		else { ParentItem->GetChildren().Add(InItem); }
	}
	else
	{
		if (bAddToHead) { Items.Insert(InItem, 0); }
		else { Items.Add(InItem); }
	}

	LinearItems.Add(InItem);
}

TSharedPtr<class IJointTreeItem> FJointTreeBuilderOutput::Find(
	const FName& InName, const FName& InType) const
{
	return Find(InName, TArray<FName, TInlineAllocator<1>>({InType}));
}

TSharedPtr<class IJointTreeItem> FJointTreeBuilderOutput::Find(
	const FName& InName, TArrayView<const FName> InTypes) const
{
	for (const TSharedPtr<IJointTreeItem>& Item : LinearItems)
	{
		bool bPassesType = (InTypes.Num() == 0);
		for (const FName& TypeName : InTypes)
		{
			if (Item->IsOfTypeByName(TypeName))
			{
				bPassesType = true;
				break;
			}
		}

		if (bPassesType && Item->GetAttachName() == InName) { return Item; }
	}

	return nullptr;
}

FJointTreeBuilder::FJointTreeBuilder(const FJointPropertyTreeBuilderArgs& InBuilderArgs)
	: BuilderArgs(InBuilderArgs)
{
}

void FJointTreeBuilder::Initialize(const TSharedRef<class SJointTree>& InTree,
                                              FOnFilterJointPropertyTreeItem InOnFilterSkeletonTreeItem)
{
	TreePtr = InTree;
	OnFilterJointPropertyTreeItem = InOnFilterSkeletonTreeItem;
}

void FJointTreeBuilder::Build(FJointTreeBuilderOutput& Output)
{
	if (BuilderArgs.bShowJointManagers) { AddJointManagers(Output); }

	if (BuilderArgs.bShowNodes) { AddNodes(Output); }

	if (BuilderArgs.bShowProperties) { AddProperties(Output); }
}

void FJointTreeBuilder::Filter(const FJointPropertyTreeFilterArgs& InArgs,
                                          const TArray<TSharedPtr<IJointTreeItem>>& InItems
                                          , TArray<TSharedPtr<IJointTreeItem>>& OutFilteredItems)
{
	OutFilteredItems.Empty();

	for (const TSharedPtr<IJointTreeItem>& Item : InItems)
	{
		if (InArgs.TextFilter.IsValid() && InArgs.bFlattenHierarchyOnFilter)
		{
			FilterRecursive(InArgs, Item, OutFilteredItems);
		}
		else
		{
			EJointTreeFilterResult FilterResult = FilterRecursive(InArgs, Item, OutFilteredItems);
			if (FilterResult != EJointTreeFilterResult::Hidden) { OutFilteredItems.Add(Item); }

			Item->SetFilterResult(FilterResult);
		}
	}
}

EJointTreeFilterResult FJointTreeBuilder::FilterRecursive(
	const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<IJointTreeItem>& InItem
	, TArray<TSharedPtr<IJointTreeItem>>& OutFilteredItems)
{
	EJointTreeFilterResult FilterResult = EJointTreeFilterResult::Shown;

	InItem->GetFilteredChildren().Empty();

	if (InArgs.TextFilter.IsValid() && InArgs.bFlattenHierarchyOnFilter)
	{
		FilterResult = FilterItem(InArgs, InItem);
		InItem->SetFilterResult(FilterResult);

		if (FilterResult != EJointTreeFilterResult::Hidden) { OutFilteredItems.Add(InItem); }

		for (const TSharedPtr<IJointTreeItem>& Item : InItem->GetChildren())
		{
			FilterRecursive(InArgs, Item, OutFilteredItems);
		}
	}
	else
	{
		// check to see if we have any children that pass the filter
		EJointTreeFilterResult DescendantsFilterResult = EJointTreeFilterResult::Hidden;
		for (const TSharedPtr<IJointTreeItem>& Item : InItem->GetChildren())
		{
			EJointTreeFilterResult ChildResult = FilterRecursive(InArgs, Item, OutFilteredItems);
			if (ChildResult != EJointTreeFilterResult::Hidden) { InItem->GetFilteredChildren().Add(Item); }
			if (ChildResult > DescendantsFilterResult) { DescendantsFilterResult = ChildResult; }
		}

		FilterResult = FilterItem(InArgs, InItem);
		if (DescendantsFilterResult > FilterResult)
		{
			FilterResult = EJointTreeFilterResult::ShownDescendant;
		}

		InItem->SetFilterResult(FilterResult);
	}

	return FilterResult;
}

EJointTreeFilterResult FJointTreeBuilder::FilterItem(
	const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<class IJointTreeItem>& InItem)
{
	return OnFilterJointPropertyTreeItem.Execute(InArgs, InItem);
}

bool FJointTreeBuilder::IsShowingJointManagers() const { return BuilderArgs.bShowJointManagers; }

bool FJointTreeBuilder::IsShowingNodes() const { return BuilderArgs.bShowNodes; }

bool FJointTreeBuilder::IsShowingProperties() const { return BuilderArgs.bShowProperties; }


void FJointTreeBuilder::AddJointManagers(FJointTreeBuilderOutput& Output)
{
	struct FJointManagerInfo
	{
		FJointManagerInfo(UJointManager* InManager)
			: JointManager(InManager)
		{
			if (JointManager)
			{
				SortString = JointManager->GetName();
				SortNumber = 0;
				SortLength = SortString.Len();
			}

			// Split the bone name into string prefix and numeric suffix for sorting (different from FName to support leading zeros in the numeric suffix)
			int32 Index = SortLength - 1;
			for (int32 PlaceValue = 1; Index >= 0 && FChar::IsDigit(SortString[Index]); --Index, PlaceValue *= 10)
			{
				SortNumber += static_cast<int32>(SortString[Index] - '0') * PlaceValue;
			}
#if UE_VERSION_OLDER_THAN(5,5,0)
				SortString.LeftInline(Index + 1, false);
#else
				SortString.LeftInline(Index + 1, EAllowShrinking::No);
#endif
			}

		bool operator<(const FJointManagerInfo& RHS)
		{
			// Sort alphabetically by string prefix
			if (int32 SplitNameComparison = SortString.Compare(RHS.SortString)) { return SplitNameComparison < 0; }

			// Sort by number if the string prefixes match
			if (SortNumber != RHS.SortNumber) { return SortNumber < RHS.SortNumber; }

			// Sort by length to give us the equivalent to alphabetical sorting if the numbers match (which gives us the following sort order: bone_, bone_0, bone_00, bone_000, bone_001, bone_01, bone_1, etc)
			return (SortNumber == 0) ? SortLength < RHS.SortLength : SortLength > RHS.SortLength;
		}

		UJointManager* JointManager;

		FString SortString;
		int32 SortNumber = 0;
		int32 SortLength = 0;
	};

	TArray<FJointManagerInfo> Managers;
	Managers.Reserve(TreePtr.Pin()->JointManagerToShow.Num());

	for (int i = 0; i < TreePtr.Pin()->JointManagerToShow.Num(); ++i)
	{
		if (!TreePtr.Pin()->JointManagerToShow.IsValidIndex(i)) continue;

		if (!TreePtr.Pin()->JointManagerToShow[i]) continue;

		Managers.Emplace(FJointManagerInfo(TreePtr.Pin()->JointManagerToShow[i]));
	}

	Algo::Sort(Managers);

	// Add the sorted bones to the skeleton tree
	for (const FJointManagerInfo& Manager : Managers)
	{
		if (!Manager.JointManager) { continue; }
		TSharedRef<IJointTreeItem> DisplayNode = CreateManagerTreeItem(Manager.JointManager);
		Output.Add(DisplayNode, NAME_None, FJointTreeItem_Manager::GetTypeId());
	}
}

void FJointTreeBuilder::AddNodes(FJointTreeBuilderOutput& Output)
{
	for (UJointManager* Manager : TreePtr.Pin()->JointManagerToShow)
	{
		if (!Manager) { return; }

		struct FNodeInfo
		{
			FNodeInfo(UJointNodeBase* InNodeInstance)
				: NodeInstance(InNodeInstance)
			{
				if (InNodeInstance)
				{
					SortString = NodeInstance->GetName();
					SortNumber = 0;
					SortLength = SortString.Len();
				}

				// Split the bone name into string prefix and numeric suffix for sorting (different from FName to support leading zeros in the numeric suffix)
				int32 Index = SortLength - 1;
				for (int32 PlaceValue = 1; Index >= 0 && FChar::IsDigit(SortString[Index]); --Index, PlaceValue *= 10)
				{
					SortNumber += static_cast<int32>(SortString[Index] - '0') * PlaceValue;
				}
#if UE_VERSION_OLDER_THAN(5,5,0)
				SortString.LeftInline(Index + 1, false);
#else
				SortString.LeftInline(Index + 1, EAllowShrinking::No);
#endif
			}

			bool operator<(const FNodeInfo& RHS)
			{
				// Sort alphabetically by string prefix
				if (int32 SplitNameComparison = SortString.Compare(RHS.SortString)) { return SplitNameComparison < 0; }

				// Sort by number if the string prefixes match
				if (SortNumber != RHS.SortNumber) { return SortNumber < RHS.SortNumber; }

				// Sort by length to give us the equivalent to alphabetical sorting if the numbers match (which gives us the following sort order: bone_, bone_0, bone_00, bone_000, bone_001, bone_01, bone_1, etc)
				return (SortNumber == 0) ? SortLength < RHS.SortLength : SortLength > RHS.SortLength;
			}

			UJointNodeBase* NodeInstance;

			FString SortString;
			int32 SortNumber = 0;
			int32 SortLength = 0;
		};

		TArray<FNodeInfo> Nodes;

		// Gather the nodes.

		for (UJointNodeBase* NodeBase : Manager->ManagerFragments)
		{
			if (!NodeBase) continue;

			Nodes.Add(NodeBase);

			for (UJointFragment* Fragment : NodeBase->GetAllFragmentsOnLowerHierarchy())
			{
				if(!Fragment) continue;
				
				Nodes.Add(Fragment);
			}
		}
		
		for (UJointNodeBase* JointNodeBase : Manager->Nodes)
		{
			if (JointNodeBase == nullptr) continue;

			Nodes.Add(FNodeInfo(JointNodeBase));

			for (UJointFragment* Fragment : JointNodeBase->GetAllFragmentsOnLowerHierarchy())
			{
				Nodes.Add(Fragment);
			}

		}

		for (const FNodeInfo& Node : Nodes)
		{
			if (!Node.NodeInstance) { continue; }

			TSharedRef<IJointTreeItem> DisplayNode = CreateNodeTreeItem(Node.NodeInstance);

			if (Node.NodeInstance->GetParentNode() == nullptr && Node.NodeInstance->GetJointManager() && Node.NodeInstance->GetJointManager()->ManagerFragments.Contains(Node.NodeInstance))
			{
				Output.Add(DisplayNode,
					FName(Node.NodeInstance->GetJointManager()->GetPathName()),
					FJointTreeItem_Manager::GetTypeId());
			}
			else if (Node.NodeInstance->GetParentNode())
			{
				Output.Add(DisplayNode,
					FName(Node.NodeInstance->GetParentNode()->GetPathName()),
					FJointTreeItem_Node::GetTypeId());
			}else if(Node.NodeInstance->GetJointManager())
			{
				Output.Add(DisplayNode,
					FName(Node.NodeInstance->GetJointManager()->GetPathName()),
					FJointTreeItem_Manager::GetTypeId());
			}
		}
	}
}



struct FPropertyInfo
{
	FPropertyInfo(FProperty* InProperty, UObject* InObject)
		: Property(InProperty), Object(InObject)
	{
		if (InObject)
		{
			SortString = InObject->GetName();
			SortNumber = 0;
			SortLength = SortString.Len();
		}

		// Split the bone name into string prefix and numeric suffix for sorting (different from FName to support leading zeros in the numeric suffix)
		int32 Index = SortLength - 1;
		for (int32 PlaceValue = 1; Index >= 0 && FChar::IsDigit(SortString[Index]); --Index, PlaceValue *= 10)
		{
			SortNumber += static_cast<int32>(SortString[Index] - '0') * PlaceValue;
		}
#if UE_VERSION_OLDER_THAN(5,5,0)
		SortString.LeftInline(Index + 1, false);
#else
		SortString.LeftInline(Index + 1, EAllowShrinking::No);
#endif
	}

	bool operator<(const FPropertyInfo& RHS)
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

	FString SortString;
	int32 SortNumber = 0;
	int32 SortLength = 0;
};

bool CheckCanImplementProperty(FProperty* Property)
{
	if(Property){
		if (!Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance)
			&& !Property->HasAnyPropertyFlags(CPF_AdvancedDisplay)
			&& Property->HasAnyPropertyFlags(CPF_Edit))
		{
			return true;
		}
	}
	return false;
}

void AddPropertyInfo(TArray<FPropertyInfo>& Infos, UObject* SearchObj)
{
	for (TFieldIterator<FProperty> It(SearchObj->GetClass()); It; ++It)
	{
		FProperty* Property = *It;

		if (!CheckCanImplementProperty(Property)) continue;

		Infos.Add(FPropertyInfo(Property, SearchObj));
	}
}


void FJointTreeBuilder::AddProperties(FJointTreeBuilderOutput& Output)
{
	for (UJointManager* Manager : TreePtr.Pin()->JointManagerToShow)
	{
		if (!Manager) { return; }

		TArray<UObject*> ObjectsToIterate;

		for (UJointNodeBase* NodeBase : Manager->ManagerFragments)
		{
			if (!NodeBase) continue;

			ObjectsToIterate.Add(NodeBase);

			for (UJointFragment* Fragment : NodeBase->GetAllFragmentsOnLowerHierarchy())
			{
				if(!Fragment) continue;
				
				ObjectsToIterate.Add(Cast<UObject>(Fragment));
			}
		}

		for (UJointNodeBase* NodeBase : Manager->Nodes)
		{
			if (!NodeBase) continue;

			ObjectsToIterate.Add(NodeBase);

			for (UJointFragment* Fragment : NodeBase->GetAllFragmentsOnLowerHierarchy())
			{
				if(!Fragment) continue;
				
				ObjectsToIterate.Add(Cast<UObject>(Fragment));
			}
		}

		TArray<FPropertyInfo> Properties;

		// Gather the nodes.
		for (UObject* Object : ObjectsToIterate)
		{
			if (!Object) continue;

			AddPropertyInfo(Properties, Object);
		}


		for (FPropertyInfo& PropertyInfo : Properties)
		{
			if (!PropertyInfo.Object) { continue; }

			TSharedRef<IJointTreeItem> DisplayNode = CreatePropertyTreeItem(
				PropertyInfo.Property, PropertyInfo.Object);

			Output.Add(
				DisplayNode,
				FName(PropertyInfo.Object->GetPathName()),
				FJointTreeItem_Node::GetTypeId());
		}
	}
}

TSharedRef<IJointTreeItem> FJointTreeBuilder::CreateManagerTreeItem(UJointManager* ManagerPtr)
{
	return MakeShareable(new FJointTreeItem_Manager(ManagerPtr, TreePtr.Pin().ToSharedRef()));
}

TSharedRef<class IJointTreeItem> FJointTreeBuilder::CreateNodeTreeItem(UJointNodeBase* NodePtr)
{
	return MakeShareable(new FJointTreeItem_Node(NodePtr, TreePtr.Pin().ToSharedRef()));
}

TSharedRef<IJointTreeItem> FJointTreeBuilder::CreatePropertyTreeItem(FProperty* Property,
	UObject* InObject)
{
	return MakeShareable(new FJointTreeItem_Property(Property, InObject, TreePtr.Pin().ToSharedRef()));
}


#undef LOCTEXT_NAMESPACE
