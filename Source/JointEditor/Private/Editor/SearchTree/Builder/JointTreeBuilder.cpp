//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "SearchTree/Builder/JointTreeBuilder.h"

#include "JointEdGraph.h"
#include "JointEditorSettings.h"
#include "JointEdUtils.h"
#include "Async/Async.h"
#include "Item/JointTreeItem_Graph.h"
#include "Item/JointTreeItem_Property.h"
#include "SearchTree/Item/JointTreeItem_Manager.h"
#include "SearchTree/Item/JointTreeItem_Node.h"
#include "SearchTree/Item/IJointTreeItem.h"
#include "SearchTree/Slate/SJointTree.h"

#include "Node/JointFragment.h"
#include "Node/JointNodeBase.h"

#include "Misc/EngineVersionComparison.h"
#include "Misc/ScopeTryLock.h"

#define LOCTEXT_NAMESPACE "FJointTreeBuilder"

bool CheckCanImplementProperty(FProperty* Property)
{
	if (Property)
	{
		if (!Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance)
			&& !Property->HasAnyPropertyFlags(CPF_AdvancedDisplay)
			&& Property->HasAnyPropertyFlags(CPF_Edit))
		{
			return true;
		}
	}
	return false;
}

void AddPropertyInfo(TArray<FJointTreePropertyInfo>& Infos, UObject* SearchObj)
{
	for (TFieldIterator<FProperty> It(SearchObj->GetClass()); It; ++It)
	{
		FProperty* Property = *It;

		if (!CheckCanImplementProperty(Property)) continue;

		Infos.Add(FJointTreePropertyInfo(Property, SearchObj));
	}
}
void AddPropertyInfoForEditorNode(TArray<FJointTreePropertyInfo>& Infos, UEdGraphNode* SearchObj)
{
	if (!SearchObj) return;

	if (UJointEdGraphNode* CastedEdNode = Cast<UJointEdGraphNode>(SearchObj))
	{
		if (UJointNodeBase* NodeInstance = CastedEdNode->GetCastedNodeInstance())
		{
			for (TFieldIterator<FProperty> It(NodeInstance->GetClass()); It; ++It)
			{
				FProperty* Property = *It;

				if (!CheckCanImplementProperty(Property)) continue;

				Infos.Add(FJointTreePropertyInfo(Property, NodeInstance, CastedEdNode));
			}
		}
	}
	
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
	ItemMap.Add(InItem->GetAttachName(), InItem);
}

void FJointTreeBuilderOutput::Add(const TSharedPtr<class IJointTreeItem>& InItem,
                                  const FName& InParentName, const FName& InParentType, bool bAddToHead)
{
	Add(InItem, InParentName, TArray<FName, TInlineAllocator<1>>({InParentType}), bAddToHead);
}

TSharedPtr<class IJointTreeItem> FJointTreeBuilderOutput::Find(
	const FName& InName, TArrayView<const FName> InTypes)
{
	if (ItemMap.Contains(InName))
	{
		return ItemMap[InName];
	}

	for (const TSharedPtr<IJointTreeItem>& Item : LinearItems)
	{
		if (!Item.IsValid()) continue;

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

TSharedPtr<class IJointTreeItem> FJointTreeBuilderOutput::Find(
	const FName& InName, const FName& InType)
{
	return Find(InName, TArray<FName, TInlineAllocator<1>>({InType}));
}

IJointTreeBuilder::~IJointTreeBuilder()
{
}

FJointTreeBuilder::FJointTreeBuilder()
{
}

FJointTreeBuilder::~FJointTreeBuilder()
{
	if (bIsBuilding)
	{
		SetShouldAbandonBuild(true);
			
		// block until the current build is finished.
		WaitForBuildToFinish();
	}
}

void FJointTreeBuilder::Initialize(const TSharedRef<class SJointTree>& InTree,
                                   FOnFilterJointPropertyTreeItem InOnFilterJointPropertyTreeItem)
{
	TreePtr = InTree;
	OnFilterJointPropertyTreeItem = InOnFilterJointPropertyTreeItem;

	bUseMultithreading = UJointEditorSettings::Get()->bUseLODRenderingForSimplePropertyDisplay;

#if UE_VERSION_OLDER_THAN(5,3,0)
	// This delegate is deprecated in 5.3 - Direct access to this delegate is not thread safe while it can be used concurrently
	FCoreDelegates::ApplicationWillTerminateDelegate.AddSP(this, &FJointTreeBuilder::OnCleanUpBlocking);
#else
	FCoreDelegates::GetApplicationWillTerminateDelegate().AddSP(this, &FJointTreeBuilder::OnCleanUpBlocking);
#endif
}

void FJointTreeBuilder::ReserveJointManagerInfo(TArray<FJointTreeJointManagerInfo>& ManagerInfos)
{
	ManagerInfos.Reserve(BuildTargetJointManagers.Num());

	for (int i = 0; i < BuildTargetJointManagers.Num(); ++i)
	{
		if (GetShouldAbandonBuild()) break;

		if (!BuildTargetJointManagers.IsValidIndex(i) || !BuildTargetJointManagers[i]) continue;

		ManagerInfos.Emplace(FJointTreeJointManagerInfo(BuildTargetJointManagers[i]));
	}
}

void FJointTreeBuilder::ReserveGraphInfo(TArray<FJointTreeGraphInfo>& Graphs)
{
	Graphs.Reserve(BuildTargetGraphs.Num());

	for (int i = 0; i < BuildTargetGraphs.Num(); ++i)
	{
		if (GetShouldAbandonBuild()) break;

		if (!BuildTargetGraphs.IsValidIndex(i) || !BuildTargetGraphs[i]) continue;

		Graphs.Emplace(FJointTreeGraphInfo(BuildTargetGraphs[i]));
	}
}

void FJointTreeBuilder::ReserveNodeInfo(TArray<FJointTreeNodeInfo>& NodeInfos)
{
	// Gather the nodes.
	
	for (UEdGraphNode* NodeBase : BuildTargetEditorNodes)
	{
		if (GetShouldAbandonBuild()) break;

		if (!NodeBase) continue;

		NodeInfos.Add(NodeBase);
	}
}

void FJointTreeBuilder::ReservePropertyInfo(TArray<FJointTreePropertyInfo>& Properties)
{
	TArray<UEdGraphNode*> EditorNodes = BuildTargetEditorNodes;

	for (UEdGraphNode* EditorNode : EditorNodes)
	{
		if (!EditorNode) continue;

		AddPropertyInfoForEditorNode(Properties, EditorNode);
	}
}

void FJointTreeBuilder::Build(FJointTreeBuilderOutput& Output)
{
	if (TreePtr.Pin())
	{
		FJointPropertyTreeBuilderArgs Args = TreePtr.Pin()->BuilderArgsAttr.Get();
		
		if (Args.bShowJointManagers)
		{
			TArray<FJointTreeJointManagerInfo> Managers;
			
			ReserveJointManagerInfo(Managers);

			for (FJointTreeJointManagerInfo& Manager : Managers)
			{
				if (GetShouldAbandonBuild()) break;

				BuildJointManagerInfo(Manager,Output);
			}
		}
		
		if (Args.bShowGraphs)
		{
			TArray<FJointTreeGraphInfo> Graphs;

			ReserveGraphInfo(Graphs);

			for (FJointTreeGraphInfo& Graph : Graphs)
			{
				if (GetShouldAbandonBuild()) break;

				BuildGraphInfo(Graph,Output);
			}
		}

		if (Args.bShowNodes)
		{
			TArray<FJointTreeNodeInfo> NodeInfos;

			ReserveNodeInfo(NodeInfos);

			for (FJointTreeNodeInfo& NodeInfo : NodeInfos)
			{
				if (GetShouldAbandonBuild()) break;
				
				BuildNodeInfo(NodeInfo,Output);
			}
		}
		
		if (Args.bShowProperties)
		{
			TArray<FJointTreePropertyInfo> Properties;

			ReservePropertyInfo(Properties);

			for (FJointTreePropertyInfo& Property : Properties)
			{
				if (GetShouldAbandonBuild()) break;
				
				BuildPropertyInfo(Property,Output);
			}
		}
	}

	CleanUpCollectedReferences();
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

EJointTreeFilterResult FJointTreeBuilder::FilterItem(
	const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<class IJointTreeItem>& InItem)
{
	return OnFilterJointPropertyTreeItem.Execute(InArgs, InItem);
}

void FJointTreeBuilder::RequestBuild(TArray<TWeakObjectPtr<UJointManager>> InJointManagersToShow)
{
	// put it in the queued jointmanagers to build with - this is threadsafe.
	QueuedBuildTargetJointManagers = InJointManagersToShow;

	// If we are not using multithreading, we do nothing.
	if (bUseMultithreading)
	{
		RunBuildAsync();
	}
	else
	{
		RunBuildSync();
	}
}

void FJointTreeBuilder::RunBuildSync()
{
	bIsBuilding = true;

	OnJointTreeBuildStarted();

	//Discard previous items
	CleanUpCollectedReferences();

	//Collect references to build
	CollectReferencesToBuild(QueuedBuildTargetJointManagers);

	FJointTreeBuilderOutput Output = FJointTreeBuilderOutput();

	Build(Output);

	OnJointTreeBuildFinished(Output);

	bIsBuilding = false;
}

void FJointTreeBuilder::RunBuildAsync()
{
	// do not start a new build if a new build is already requested. 
	if (GetIsNewBuildRequested()) return;

	SetIsNewBuildRequested(true);
	
	// Start to build the tree in the background thread.
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
	{

		// if we are already building, we set the flag to abandon the current build.
		if (bIsBuilding)
		{
			SetShouldAbandonBuild(true);
			
			// block until the current build is finished.
			WaitForBuildToFinish();
		}

		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			// Start to build the tree in the background thread.
			FScopeTryLock Lock(&BuildMutex);
			if (!Lock.IsLocked()) return;

			SetShouldAbandonBuild(false);
			bIsBuilding = true;

			OnJointTreeBuildStarted();

			//Discard previous items
			CleanUpCollectedReferences();

			//Collect references to build
			CollectReferencesToBuild(QueuedBuildTargetJointManagers);

			// Start to build the tree in the background thread.
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
			{
				FScopeTryLock NewLock(&BuildMutex);

				if (!NewLock.IsLocked()) return;

				// we are no longer requesting a new build - we are building now.
				SetIsNewBuildRequested(false);
					
				FJointTreeBuilderOutput Output = FJointTreeBuilderOutput();

				Build(Output);

				// we finished building the tree - if we should abandon the build, we do so now.
				AsyncTask(ENamedThreads::GameThread, [this, Output]()
				{
					if (!GetShouldAbandonBuild())
					{
						OnJointTreeBuildFinished(Output);
					}
					else
					{
						OnJointTreeBuildCancelled();
					}

					SetShouldAbandonBuild(false);
					bIsBuilding = false;
				});
			});
		});
	});
}

void FJointTreeBuilder::AbandonBuild()
{
	// If we are not using multithreading, we do nothing.
	if (!bUseMultithreading) return;

	// Simply set the flag to abandon the build - the build thread will check this flag and abandon the build if necessary.
	SetShouldAbandonBuild(true);
}

void FJointTreeBuilder::WaitForBuildToFinish()
{
	while (bIsBuilding)
	{
		//Add a small delay to avoid harshing the thread on busy waiting.

		FPlatformProcess::Sleep(0.02f);
	}
}

void FJointTreeBuilder::OnJointTreeBuildStarted()
{
	if (OnJointTreeBuildStartedDele.IsBound())
	{
		OnJointTreeBuildStartedDele.Execute();
	}
}

void FJointTreeBuilder::OnJointTreeBuildFinished(const FJointTreeBuilderOutput Output)
{
	if (OnJointTreeBuildFinishedDele.IsBound())
	{
		OnJointTreeBuildFinishedDele.Execute(Output);
	}
}

void FJointTreeBuilder::OnJointTreeBuildCancelled()
{
	if (OnJointTreeBuildCancelledDele.IsBound())
	{
		OnJointTreeBuildCancelledDele.Execute();
	}
}

void FJointTreeBuilder::OnCleanUpBlocking()
{
	// If we are not using multithreading, we do nothing.
	if (!bUseMultithreading) return;

	SetShouldAbandonBuild(true);

	FScopeTryLock Lock(&BuildMutex);

	WaitForBuildToFinish();

	CleanUpCollectedReferences();
}


void FJointTreeBuilder::CleanUpCollectedReferences()
{
	BuildTargetJointManagers.Empty();
	BuildTargetEditorNodes.Empty();
	BuildTargetGraphs.Empty();
}

void FJointTreeBuilder::CollectReferencesToBuild(const TArray<TWeakObjectPtr<UJointManager>>& InJointManagers)
{
	for (TWeakObjectPtr<UJointManager> ManagerPtr : InJointManagers)
	{
		if (!ManagerPtr.IsValid()) continue;

		UJointManager* Manager = ManagerPtr.Get();

		if (!Manager) continue;

		BuildTargetJointManagers.Add(Manager);

		TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(Manager);

		for (UJointEdGraph* Graph : Graphs)
		{
			if (!Graph) continue;

			BuildTargetGraphs.Add(Graph);
		}

		for (UJointEdGraph* JointEdGraph : Graphs)
		{
			if (!JointEdGraph) continue;

			TSet<TWeakObjectPtr<UJointEdGraphNode>> Nodes = JointEdGraph->GetCachedJointGraphNodes(false);

			for (TWeakObjectPtr<UJointEdGraphNode> JointEdGraphNode : Nodes)
			{
				BuildTargetEditorNodes.Add(JointEdGraphNode.Get());
			}

			// for comment nodes - they are not included in the cached nodes.
			for (UEdGraphNode* GraphNodes : JointEdGraph->Nodes)
			{
				if (!GraphNodes) continue;

				if (!Cast<UEdGraphNode_Comment>(GraphNodes)) continue;
				
				if (!BuildTargetEditorNodes.Contains(GraphNodes)) BuildTargetEditorNodes.Add(GraphNodes);
			}
		}
	}
}

void FJointTreeBuilder::SetShouldAbandonBuild(const bool bNewInShouldAbandonBuild)
{
	bShouldAbandonBuild = bNewInShouldAbandonBuild;
}

const bool FJointTreeBuilder::GetShouldAbandonBuild() const
{
	return bShouldAbandonBuild || IsEngineExitRequested() || IsRunningCommandlet() || IsGarbageCollecting();
}

void FJointTreeBuilder::SetIsNewBuildRequested(const bool bNewInIsNewBuildRequested)
{
	bIsNewBuildRequested = bNewInIsNewBuildRequested;
}

bool FJointTreeBuilder::GetIsNewBuildRequested() const
{
	return bIsNewBuildRequested;
}

void FJointTreeBuilder::BuildJointManagerInfo(const FJointTreeJointManagerInfo& JointManagerInfo, FJointTreeBuilderOutput& Output)
{
	if (!JointManagerInfo.JointManager.Get()) return;

	TSharedPtr<IJointTreeItem> DisplayNode = CreateManagerTreeItem(JointManagerInfo.JointManager);

	if (!DisplayNode.IsValid()) return;

	Output.Add(DisplayNode, NAME_None, FJointTreeItem_Manager::GetTypeId());
}

void FJointTreeBuilder::BuildGraphInfo(const FJointTreeGraphInfo& GraphInfo, FJointTreeBuilderOutput& Output)
{
	// Add the sorted bones to the skeleton tree

	if (!GraphInfo.Graph) return;

	TSharedPtr<IJointTreeItem> DisplayNode = CreateGraphTreeItem(GraphInfo.Graph);

	if (!DisplayNode.IsValid()) return;

	if (GraphInfo.Graph->GetParentGraph())
	{
		UJointEdGraph* Graph = GraphInfo.Graph;
		Output.Add(DisplayNode, FName(Graph->GetParentGraph()->GetPathName()), FJointTreeItem_Graph::GetTypeId());
	}
	else
	{
		UJointEdGraph* Graph = GraphInfo.Graph;
		Output.Add(DisplayNode, FName(Graph->GetJointManager()->GetPathName()), FJointTreeItem_Manager::GetTypeId());
	}
}

void FJointTreeBuilder::BuildNodeInfo(const FJointTreeNodeInfo& NodeInfo, FJointTreeBuilderOutput& Output)
{
	if (!NodeInfo.EditorNode) return;

	TSharedPtr<IJointTreeItem> DisplayNode = CreateNodeTreeItem(NodeInfo.EditorNode);

	if (!DisplayNode.IsValid()) return;

	if (UJointEdGraphNode* JointNode = Cast<UJointEdGraphNode>(NodeInfo.EditorNode))
	{
		if (JointNode->GetCastedNodeInstance<UJointManager>() == JointNode->GetJointManager())
		{
			// root node of the manager
			Output.Add(DisplayNode, FName(JointNode->GetJointManager()->GetPathName()), FJointTreeItem_Manager::GetTypeId());
		}
		else if (JointNode->GetJointManager() && JointNode->GetJointManager()->ManagerFragments.Contains(JointNode->GetCastedNodeInstance()))
		{
			//manager fragments - attach it on the root node above ( attach ".Root" at the end )
			Output.Add(DisplayNode,FName(JointNode->GetJointManager()->GetPathName()+".Root"),FJointTreeItem_Manager::GetTypeId());
		}
		else if (JointNode->ParentNode == nullptr)
		{
			//top level nodes without managers - attach it on the graph.
			Output.Add(DisplayNode, FName(JointNode->GetGraph()->GetPathName()), FJointTreeItem_Graph::GetTypeId());
		}
		else if (JointNode->ParentNode)
		{
			Output.Add(DisplayNode,
					   FName(JointNode->ParentNode->GetPathName()),
					   FJointTreeItem_Node::GetTypeId());
		}
		else if (JointNode->GetJointManager())
		{
			Output.Add(DisplayNode,
					   FName(JointNode->GetJointManager()->GetPathName()),
					   FJointTreeItem_Manager::GetTypeId());
		}
	}else if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(NodeInfo.EditorNode))
	{
		// Comment nodes - attach it on the graph.
		Output.Add(DisplayNode, FName(CommentNode->GetGraph()->GetPathName()), FJointTreeItem_Graph::GetTypeId());
	}
}

EJointTreeFilterResult FJointTreeBuilder::FilterRecursive(
	const FJointPropertyTreeFilterArgs& InArgs, const TSharedPtr<IJointTreeItem>& InItem
	, TArray<TSharedPtr<IJointTreeItem>>& OutFilteredItems)
{
	EJointTreeFilterResult FilterResult = EJointTreeFilterResult::Shown;

	InItem->GetFilteredChildren().Empty();

	if (InArgs.TextFilter.IsValid())
	{
		// check to see if we have any children that pass the filter
		EJointTreeFilterResult DescendantsFilterResult = EJointTreeFilterResult::Hidden;

		// check to see if we pass the filter by itself - 
		FilterResult = FilterItem(InArgs, InItem);

		// Set the filter result for this item - the descendants will check this value and decide whether to show themselves or not if they want.
		InItem->SetFilterResult(FilterResult);

		// check out the children
		for (const TSharedPtr<IJointTreeItem>& Item : InItem->GetChildren())
		{
			EJointTreeFilterResult ChildResult = FilterRecursive(InArgs, Item, OutFilteredItems);

			// If the child is visible, add it to the filtered children list.
			if (ChildResult != EJointTreeFilterResult::Hidden) { InItem->GetFilteredChildren().Add(Item); }

			// Accumulate the best result from all children.
			if (ChildResult > DescendantsFilterResult) { DescendantsFilterResult = ChildResult; }
		}

		// If the item itself passes the filter, or any of its descendants do, it should be shown.
		if (DescendantsFilterResult > FilterResult)
		{
			FilterResult = EJointTreeFilterResult::ShownDescendant;
		}

		// finalize the filter result for this item
		InItem->SetFilterResult(FilterResult);
	}

	return FilterResult;
}

void FJointTreeBuilder::BuildPropertyInfo(FJointTreePropertyInfo& PropertyInfo, FJointTreeBuilderOutput& Output)
{
	if (!PropertyInfo.Object) return;

	TSharedPtr<IJointTreeItem> DisplayNode = CreatePropertyTreeItem(PropertyInfo.Property, PropertyInfo.Object);

	if (!DisplayNode.IsValid()) return;

	Output.Add(
		DisplayNode,
		FName(PropertyInfo.TreeItemOwnerObject != nullptr ? PropertyInfo.TreeItemOwnerObject->GetPathName() : PropertyInfo.Object->GetPathName()),
		FJointTreeItem_Node::GetTypeId());
}

TSharedPtr<IJointTreeItem> FJointTreeBuilder::CreateManagerTreeItem(TWeakObjectPtr<UJointManager> ManagerPtr)
{
	if (GetShouldAbandonBuild()) return nullptr;

	return MakeShareable(new FJointTreeItem_Manager(ManagerPtr, TreePtr.Pin().ToSharedRef()));
}

TSharedPtr<IJointTreeItem> FJointTreeBuilder::CreateGraphTreeItem(UJointEdGraph* GraphPtr)
{
	if (GetShouldAbandonBuild()) return nullptr;

	return MakeShareable(new FJointTreeItem_Graph(GraphPtr, TreePtr.Pin().ToSharedRef()));
}

TSharedPtr<IJointTreeItem> FJointTreeBuilder::CreateNodeTreeItem(UEdGraphNode* NodePtr)
{
	if (GetShouldAbandonBuild()) return nullptr;

	return MakeShareable(new FJointTreeItem_Node(NodePtr, TreePtr.Pin().ToSharedRef()));
}

TSharedPtr<IJointTreeItem> FJointTreeBuilder::CreatePropertyTreeItem(FProperty* Property,
                                                                     UObject* InObject)
{
	if (GetShouldAbandonBuild()) return nullptr;

	return MakeShareable(new FJointTreeItem_Property(Property, InObject, TreePtr.Pin().ToSharedRef()));
}


#undef LOCTEXT_NAMESPACE
