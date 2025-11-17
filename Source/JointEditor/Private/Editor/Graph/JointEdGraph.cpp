//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointEdGraph.h"

#include "JointManager.h"
#include "GraphEditAction.h"
#include "IMessageLogListing.h"
#include "JointEdGraphSchema.h"
#include "JointEditorNameValidator.h"
#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "JointEdUtils.h"
#include "MessageLogModule.h"
#include "EdGraph/EdGraphSchema.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GraphNode/SJointGraphNodeBase.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Node/JointEdGraphNode.h"
#include "Node/JointNodeBase.h"

#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"


#define LOCTEXT_NAMESPACE "UJointEdGraph"



UJointEdGraph::UJointEdGraph()
{
}

void UJointEdGraph::OnGraphObjectCreated()
{
	//Do nothing.
}

void UJointEdGraph::OnLoaded()
{
	RecacheNodes();

	bAllowDeletion = !IsRootGraph();
	
	if (!IsLocked())
	{
		//Lock the graph to avoid multiple updates during the process (some instances might trigger graph updates during the process)
		LockUpdates();
		
		TryReinstancingUnknownNodeClasses();

		BindEdNodeEvents();
		FeedToolkitToGraphNodes();
		ResetGraphNodeSlates();
		RecalculateNodeDepth();

		UnlockUpdates();
	}
}

void UJointEdGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RecacheNodes();

	UpdateGraph();
}

void UJointEdGraph::NotifyGraphChanged()
{
	Super::NotifyGraphChanged();

	RecacheNodes();

	UpdateGraph();
	
}

void UJointEdGraph::NotifyGraphChanged(const FEdGraphEditAction& InAction)
{
	Super::NotifyGraphChanged(InAction);

	RecacheNodes();

	UpdateGraph();
}

void UJointEdGraph::NotifyGraphRequestUpdate()
{
	RecacheNodes();

	UpdateGraph();
}


void UJointEdGraph::UpdateGraph()
{
	if (!IsLocked())
	{
		//Lock the graph to avoid multiple updates during the process (some instances might trigger graph updates during the process)
		LockUpdates();

		AllocateBaseNodesToJointManager();
		
		TryReinstancingUnknownNodeClasses();
		
		UpdateClassData();
		
		UpdateSubNodeChains();
		
		FeedToolkitToGraphNodes();
		
		BindEdNodeEvents();
		
		UpdateDebugData();
		
		CompileAllJointGraphFromRoot();
		
		NotifyNodeConnectionChanged();

		if (GetToolkit().Pin())
		{
			GetToolkit().Pin()->RequestManagerViewerRefresh();
			GetToolkit().Pin()->RefreshJointEditorOutliner();
		}
		
		//Unlock the graph after all the updates are done.
		UnlockUpdates();
	}
}

void UJointEdGraph::ResetGraphNodeSlates()
{
	for (const TWeakObjectPtr<UJointEdGraphNode> GraphNode : GetCachedJointGraphNodes())
	{
		if (GraphNode.IsValid()) GraphNode->SetGraphNodeSlate(nullptr);
	}
}

void UJointEdGraph::RecalculateNodeDepth()
{
	for (const TWeakObjectPtr<UJointEdGraphNode> GraphNode : GetCachedJointGraphNodes())
	{
		if (GraphNode.IsValid()) GraphNode->RecalculateNodeDepth();
	}
}

void UJointEdGraph::FeedToolkitToGraphNodes()
{
	for (const TWeakObjectPtr<UJointEdGraphNode> GraphNode : GetCachedJointGraphNodes())
	{
		if (GraphNode.IsValid()) GraphNode->OptionalToolkit = Toolkit;
	}
}

void UJointEdGraph::NotifyNodeConnectionChanged()
{
	for (const TWeakObjectPtr<UJointEdGraphNode> GraphNode : GetCachedJointGraphNodes())
	{
		if (GraphNode.IsValid()) GraphNode->NodeConnectionListChanged();
	}
}

void UJointEdGraph::ReallocateGraphPanelToGraphNodeSlates(TSharedPtr<SGraphPanel> GraphPanel)
{
	if (!GraphPanel.IsValid()) return;

	for (const TWeakObjectPtr<UJointEdGraphNode> GraphNode : GetCachedJointGraphNodes(true))
	{
		if (!GraphNode.IsValid()) continue;

		TWeakPtr<SJointGraphNodeBase> Slate = GraphNode->GetGraphNodeSlate();

		if (Slate.IsValid())
		{
			Slate.Pin()->SetOwner(GraphPanel.ToSharedRef());
		}
	}
}

void UJointEdGraph::RecacheNodes()
{
	CacheJointNodeInstances();
	CacheJointGraphNodes();
}

void GetSubGraphsRecursively(UJointEdGraph* InGraph, TArray<UJointEdGraph*>& OutGraphs)
{
	if (InGraph == nullptr) return;

	for (UEdGraph* SubGraph : InGraph->SubGraphs)
	{
		if (SubGraph == nullptr) continue;

		UJointEdGraph* SubGraphCasted = Cast<UJointEdGraph>(SubGraph);

		OutGraphs.Add(SubGraphCasted);

		GetSubGraphsRecursively(SubGraphCasted, OutGraphs);
	}
}


UJointEdGraph* UJointEdGraph::GetParentGraph() const
{
	UJointEdGraph* CurrentGraph = const_cast<UJointEdGraph*>(this);

	if (UEdGraph* ParentGraph = GetOuterGraph(CurrentGraph))
	{
		return Cast<UJointEdGraph>(ParentGraph);
	}

	return nullptr;
}

UJointEdGraph* UJointEdGraph::GetRootGraph() const
{
	UJointEdGraph* CurrentGraph = const_cast<UJointEdGraph*>(this);

	while (CurrentGraph->GetParentGraph())
	{
		CurrentGraph = CurrentGraph->GetParentGraph();
	}

	return CurrentGraph;
}

bool UJointEdGraph::IsRootGraph() const
{
	return GetRootGraph() == this;
}

TArray<UJointEdGraph*> UJointEdGraph::GetAllSubGraphsRecursively() const
{
	TArray<UJointEdGraph*> OutGraphs;

	GetSubGraphsRecursively(const_cast<UJointEdGraph*>(this), OutGraphs);

	return OutGraphs;
}

TArray<UJointEdGraph*> UJointEdGraph::GetDirectSubGraphs() const
{
	TArray<UJointEdGraph*> OutGraphs;

	for (UEdGraph* SubGraph : SubGraphs)
	{
		if (SubGraph == nullptr) continue;

		if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(SubGraph))
		{
			OutGraphs.Add(CastedGraph);
		}
	}

	return OutGraphs;
}

TArray<UJointEdGraph*> UJointEdGraph::GetAllGraphsFrom(UEdGraph* InGraph)
{
	TArray<UJointEdGraph*> OutGraphs;

	if (InGraph == nullptr) return OutGraphs;

	if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(InGraph))
	{
		OutGraphs.Add(CastedGraph);

		OutGraphs.Append(CastedGraph->GetAllSubGraphsRecursively());
	}

	return OutGraphs;
}

TArray<UJointEdGraph*> UJointEdGraph::GetAllGraphsFrom(const UJointManager* InJointManager)
{
	TArray<UJointEdGraph*> OutGraphs;

	if (InJointManager)
	{
		OutGraphs.Append(GetAllGraphsFrom(InJointManager->JointGraph));
	}

	return OutGraphs;
}

void UJointEdGraph::SetToolkit(const TSharedPtr<FJointEditorToolkit>& InToolkit)
{
	if (!InToolkit.IsValid()) return;

	Toolkit = InToolkit;
}

void UJointEdGraph::UpdateDebugData()
{
	//Remove all unnecessary data in the array.

	DebugData.RemoveAll([](const FJointNodeDebugData& Value)
	{
		if (!Value.CheckWhetherNecessary()) return true;

		return false;
	});
}

void UJointEdGraph::OnSave()
{
	if (!IsLocked())
	{
		LockUpdates();
		
		TryReinstancingUnknownNodeClasses();
		UpdateClassData();
		ReconstructAllNodes();
		UpdateSubNodeChains();

		UnlockUpdates();
	}
}

void UJointEdGraph::OnClosed()
{
	CleanUpNodes();
}

void UJointEdGraph::BindEdNodeEvents()
{
	for (const TWeakObjectPtr<UJointEdGraphNode> GraphNode : CachedJointGraphNodes)
	{
		if (GraphNode.IsValid()) GraphNode->BindNodeInstance();
	}
}

void UJointEdGraph::AllocateBaseNodesToJointManager()
{
	//Ensure the Root graph starts off the allocation of the nodes to the Joint manager - not the sub graphs.
	if (GetRootGraph() != this)
	{
		GetRootGraph()->AllocateBaseNodesToJointManager();
		return;
	}

	if (!JointManager) return;

	JointManager->Nodes.Empty();

	AllocateThisGraphBaseNodesToJointManager(JointManager, this);

	TArray<UJointEdGraph*> InSubGraphs = GetAllSubGraphsRecursively();

	for (UJointEdGraph* SubGraph : InSubGraphs)
	{
		AllocateThisGraphBaseNodesToJointManager(JointManager, SubGraph);
	}
}


void UJointEdGraph::AllocateThisGraphBaseNodesToJointManager(UJointManager* JointManager, UJointEdGraph* Graph)
{
	if (!JointManager || !Graph) return;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node == nullptr) continue;

		const UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node);

		if (CastedNode == nullptr) continue;

		UJointNodeBase* NodeInstance = CastedNode->GetCastedNodeInstance();

		if (NodeInstance == nullptr) continue;

		JointManager->Nodes.Add(NodeInstance);
	}
}

void UJointEdGraph::UpdateClassDataForNode(UJointEdGraphNode* Node, const bool bPropagateToSubNodes)
{
	if (!Node) return;

	Node->UpdateNodeClassData();

	if (bPropagateToSubNodes)
	{
		for (UJointEdGraphNode* SubNode : Node->SubNodes) UpdateClassDataForNode(SubNode);
	}
}


void UJointEdGraph::GrabUnknownClassDataFromNode(UJointEdGraphNode* Node, const bool bPropagateToSubNodes)
{
	if (!Node) return;

	if (!Node->NodeClassData.GetClass())
	{
		FJointGraphNodeClassHelper::AddUnknownClass(Node->NodeClassData);
	}

	if (bPropagateToSubNodes)
	{
		for (UJointEdGraphNode* SubNode : Node->SubNodes)
		{
			GrabUnknownClassDataFromNode(SubNode, bPropagateToSubNodes);
		}
	}
}

void UJointEdGraph::TryReinstancingUnknownNodeClasses()
{
	for (const TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		PatchNodeInstanceFromStoredNodeClass(EdGraphNode);
	}
}

void UJointEdGraph::PatchNodeInstanceFromStoredNodeClass(TObjectPtr<UEdGraphNode> TargetNode,
                                                         const bool bPropagateToSubNodes)
{
	if (!TargetNode) return;

	UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(TargetNode);

	if (!CastedNode) return;

	CastedNode->PatchNodeInstanceFromClassDataIfNeeded();

	if (bPropagateToSubNodes)
	{
		for (UJointEdGraphNode* SubNode : CastedNode->SubNodes) PatchNodeInstanceFromStoredNodeClass(SubNode);
	}
}

void UJointEdGraph::PreDuplicate(FObjectDuplicationParameters& DupParams)
{
	Super::PreDuplicate(DupParams);

	//PrepareForCopy();
}

void UJointEdGraph::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	//PostCopy();
}

void UJointEdGraph::PrepareForCopy()
{
	TSet<TWeakObjectPtr<UJointEdGraphNode>> AllNodes = GetCachedJointGraphNodes(true);
	
	for (const TWeakObjectPtr<UJointEdGraphNode>& JointEdGraphNode : AllNodes)
	{
		if (!JointEdGraphNode.IsValid()) continue;
		
		JointEdGraphNode->PrepareForCopying();
	}

	//Set every node's outer to this graph to make sure they get duplicated along with the graph.

	for (const TWeakObjectPtr<UJointEdGraphNode>& JointEdGraphNode : AllNodes)
	{
		if (!JointEdGraphNode.IsValid()) continue;
		
		JointEdGraphNode->SetOuterAs(this);
	}

	//propagate to sub graphs
	TArray<UJointEdGraph*> InSubGraphs = GetDirectSubGraphs();

	for (UJointEdGraph* const& SubGraph : InSubGraphs)
	{
		if (!SubGraph) continue;
		
		SubGraph->PrepareForCopy();
	}
}

void UJointEdGraph::PostCopy()
{
	TSet<TWeakObjectPtr<UJointEdGraphNode>> AllNodes = GetCachedJointGraphNodes(true);
	
	for (const TWeakObjectPtr<UJointEdGraphNode>& JointEdGraphNode : AllNodes)
	{
		if (!JointEdGraphNode.IsValid()) continue;
		
		JointEdGraphNode->PostCopyNode();
	}

	//Recache
	AllNodes = GetCachedJointGraphNodes(true);

	//Set every node's outer to this graph.

	for (const TWeakObjectPtr<UJointEdGraphNode>& JointEdGraphNode : AllNodes)
	{
		if (!JointEdGraphNode.IsValid()) continue;
		
		JointEdGraphNode->SetOuterAs(this);
	}

	//propagate to sub graphs
	TArray<UJointEdGraph*> InSubGraphs = GetDirectSubGraphs();

	for (UJointEdGraph* const& SubGraph : InSubGraphs)
	{
		if (!SubGraph) continue;
		
		SubGraph->PostCopy();
	}
}

void UJointEdGraph::UpdateClassData()
{
	for (TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(EdGraphNode);

		UpdateClassDataForNode(Node);
	}
}

void UJointEdGraph::GrabUnknownClassDataFromGraph()
{
	for (TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(EdGraphNode);

		GrabUnknownClassDataFromNode(Node, true);
	}
}


void UJointEdGraph::CleanUpNodes()
{
	TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = GetCachedJointGraphNodes();

	for (TWeakObjectPtr<UJointEdGraphNode> JointEdGraphNode : GraphNodes)
	{
		if (JointEdGraphNode.IsValid())
		{
			JointEdGraphNode->ClearGraphNodeSlate();
		}
	}
}

void UJointEdGraph::ReconstructAllNodes(bool bPropagateUnder)
{
	for (UEdGraphNode* Node : Nodes)
	{
		UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node);

		if (!CastedNode) continue;

		if (bPropagateUnder)
		{
			CastedNode->ReconstructNodeInHierarchy();
		}
		else
		{
			CastedNode->ReconstructNode();
		}
	}

	NotifyGraphRequestUpdate();
}

void UJointEdGraph::PatchNodePickers()
{
	for (UEdGraphNode* Node : Nodes)
	{
		if (!Node) continue;
	}
}

void UJointEdGraph::ExecuteForAllNodesInHierarchy(const TFunction<void(UEdGraphNode*)>& Func)
{
	const TSet<TWeakObjectPtr<UJointEdGraphNode>>& CachedNodes = GetCachedJointGraphNodes();

	for (TWeakObjectPtr<UJointEdGraphNode> JointEdGraphNode : CachedNodes)
	{
		Func(JointEdGraphNode.Get());
	}
}


void UJointEdGraph::InitializeCompileResultIfNeeded()
{
	if (CompileResultPtr.IsValid()) return;

	// subgraphs will use the same message log as the parent graph - only the Root graph will create a new message log.
	UJointEdGraph* RootGraph = GetRootGraph();

	if (RootGraph && RootGraph != this)
	{
		RootGraph->InitializeCompileResultIfNeeded();

		CompileResultPtr = RootGraph->CompileResultPtr;

		return;
	}

	if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
	{
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");

		FMessageLogInitializationOptions LogOptions;
		LogOptions.bShowFilters = true;

		CompileResultPtr = MessageLogModule.CreateLogListing("LogJoint", LogOptions);

		if (CompileResultPtr.IsValid())
		{
			TSharedRef<FTokenizedMessage> Token = FTokenizedMessage::Create(
				EMessageSeverity::Info, FText::FromString("Press 'Compile' to check out possible issues."));

			CompileResultPtr->AddMessage(Token);
		}
	}
}

void UJointEdGraph::CompileAllJointGraphFromRoot()
{
	if (GetRootGraph() != this)
	{
		GetRootGraph()->CompileAllJointGraphFromRoot();
		return;
	}

	if (!CompileResultPtr) return;

	const double CompileStartTime = FPlatformTime::Seconds();

	CompileResultPtr->ClearMessages();

	CompileJointGraph();

	for (UJointEdGraph* SubGraph : GetAllSubGraphsRecursively())
	{
		SubGraph->CompileJointGraph();
	}

	const double CompileEndTime = FPlatformTime::Seconds();

	if (OnCompileFinished.IsBound())
		OnCompileFinished.Execute(
			UJointEdGraph::FJointGraphCompileInfo(GetCachedJointGraphNodes().Num(), (CompileEndTime - CompileStartTime)));
}

void UJointEdGraph::CompileJointGraphForNode(UJointEdGraphNode* Node, const bool bPropagateToSubNodes)
{
	if (Node == nullptr) return;

	//Abort if the CompileResultPtr was not valid.
	if (!CompileResultPtr.IsValid()) return;

	Node->CompileNode(CompileResultPtr.ToSharedRef());

	if (bPropagateToSubNodes)
	{
		for (UJointEdGraphNode* SubNode : Node->SubNodes) CompileJointGraphForNode(SubNode, bPropagateToSubNodes);
	}
}

void UJointEdGraph::CompileJointGraph()
{
	InitializeCompileResultIfNeeded();

	// This function sets error messages and logs errors about nodes.
	if (!CompileResultPtr.IsValid()) return;

	for (TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		if (EdGraphNode == nullptr) continue;

		UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(EdGraphNode);

		CompileJointGraphForNode(CastedNode, true);
	}
}

void UJointEdGraph::UpdateSubNodeChains()
{
	for (TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		if (EdGraphNode == nullptr) continue;

		UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(EdGraphNode);

		if (CastedNode == nullptr) continue;

		CastedNode->UpdateSubNodeChain();
	}
}

UEdGraphNode* UJointEdGraph::FindGraphNodeForNodeInstance(const UObject* NodeInstance)
{
	if (NodeInstance == nullptr) return nullptr;

	CacheJointGraphNodes();

	for (TWeakObjectPtr<UJointEdGraphNode> CachedJointGraphNode : CachedJointGraphNodes)
	{
		if (CachedJointGraphNode == nullptr) continue;

		if (CachedJointGraphNode.Get()->NodeInstance == NodeInstance) return CachedJointGraphNode.Get();
	}

	return nullptr;
}

TSet<TWeakObjectPtr<UObject>> UJointEdGraph::GetCachedJointNodeInstances(const bool bForceRecache)
{
	if (bForceRecache || CachedJointNodeInstances.IsEmpty()) CacheJointNodeInstances();

	return CachedJointNodeInstances;
}

TSet<TWeakObjectPtr<UJointEdGraphNode>> UJointEdGraph::GetCachedJointGraphNodes(const bool bForceRecache)
{
	if (bForceRecache || CachedJointGraphNodes.IsEmpty()) CacheJointGraphNodes();

	return CachedJointGraphNodes;
}


void UJointEdGraph::BeginCacheForCookedPlatformData(const ITargetPlatform* TargetPlatform)
{
	if (GetRootGraph() == this)
	{
		CompileAllJointGraphFromRoot();
	}

	Super::BeginCacheForCookedPlatformData(TargetPlatform);
}

UJointEdGraph* UJointEdGraph::CreateNewJointGraph(UObject* InOuter, UJointManager* InJointManager, const FName& GraphName)
{
	UJointEdGraph* NewJointGraph = nullptr;

	if (InJointManager && InOuter)
	{
		FString NewName = GraphName.ToString();

		FJointEdUtils::GetSafeNameForObject(NewName, InOuter);
		
		NewJointGraph = Cast<UJointEdGraph>(FBlueprintEditorUtils::CreateNewGraph(
			InOuter,
			FName(NewName),
			UJointEdGraph::StaticClass(),
			UJointEdGraphSchema::StaticClass()));

		NewJointGraph->JointManager = InJointManager;
		NewJointGraph->GetSchema()->CreateDefaultNodesForGraph(*NewJointGraph);
		NewJointGraph->OnGraphObjectCreated();
	}

	return NewJointGraph;
}


void CollectInstances(TSet<TWeakObjectPtr<UObject>>& NodeInstances, TObjectPtr<UEdGraphNode> Node)
{
	if (Node == nullptr) return;

	if (UJointEdGraphNode* MyNode = Cast<UJointEdGraphNode>(Node))
	{
		NodeInstances.Add(MyNode->NodeInstance);

		for (UJointEdGraphNode* SubNode : MyNode->SubNodes)
		{
			CollectInstances(NodeInstances, SubNode);
		}
	}
}

void UJointEdGraph::CacheJointNodeInstances()
{
	FScopeLock Lock(&CachedJointNodeInstancesMutex);

	CachedJointNodeInstances.Empty();

	for (const TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		CollectInstances(CachedJointNodeInstances, EdGraphNode);
	}
}

void CollectAllGraphNodesInternal(TSet<TWeakObjectPtr<UJointEdGraphNode>>& GraphNodes, TObjectPtr<UEdGraphNode> Node)
{
	if (Node == nullptr) return;

	if (UJointEdGraphNode* MyNode = Cast<UJointEdGraphNode>(Node))
	{
		GraphNodes.Add(MyNode);

		for (UJointEdGraphNode* SubNode : MyNode->SubNodes)
		{
			CollectAllGraphNodesInternal(GraphNodes, SubNode);
		}
	}
}


void UJointEdGraph::CacheJointGraphNodes()
{
	FScopeLock Lock(&CachedJointGraphNodesMutex);

	CachedJointGraphNodes.Empty();

	for (const TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		CollectAllGraphNodesInternal(CachedJointGraphNodes, EdGraphNode);
	}
}


void UJointEdGraph::OnNodesPasted(const FString& ImportStr)
{
	// empty in base class
}

bool UJointEdGraph::CanRemoveNestedObject(UObject* TestObject) const
{
	return !TestObject->IsA(UEdGraphNode::StaticClass()) &&
		!TestObject->IsA(UEdGraph::StaticClass()) &&
		!TestObject->IsA(UEdGraphSchema::StaticClass());
}

void UJointEdGraph::RemoveOrphanedNodes()
{
	UpdateSubNodeChains();

	// Obtain a list of all nodes actually in the asset and discard unused nodes
	TArray<UObject*> AllInners;

	constexpr bool bIncludeNestedObjects = false;

	GetObjectsWithOuter(GetOuter(), AllInners, bIncludeNestedObjects);

	uint16 count = 0;

	GetCachedJointNodeInstances();

	for (auto InnerIt = AllInners.CreateConstIterator(); InnerIt; ++InnerIt)
	{
		UObject* TestObject = *InnerIt;

		if (!CachedJointNodeInstances.Contains(TestObject) && CanRemoveNestedObject(TestObject))
		{
			OnNodeInstanceRemoved(TestObject);

			TestObject->SetFlags(RF_Transient);
			TestObject->Rename(NULL, GetTransientPackage(),
			                   REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);

			++count;
		}
	}

	//Notify and mark the asset dirty.
	if (count > 0)
	{
		FNotificationInfo NotificationInfo(
			FText::Format(
				LOCTEXT("DiscardOrphanedObjects", "{0}: Detected and discarded {1} orphaned object(s) from the graph."),
				GetJointManager() ? FText::FromString(GetJointManager()->GetName()) : FText::FromString(FString("NULL")),
				count)
		);
		NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.FadeInDuration = 0.3f;
		NotificationInfo.FadeOutDuration = 1.3f;
		NotificationInfo.ExpireDuration = 4.5f;

		FSlateNotificationManager::Get().AddNotification(NotificationInfo);

		if (GetJointManager() != nullptr) GetJointManager()->MarkPackageDirty();
	}
	else
	{
		FNotificationInfo NotificationInfo(
			FText::Format(
				LOCTEXT("NoOrphanedObjects", "{0}: Has zero orphened nodes"),
				GetJointManager() ? FText::FromString(GetJointManager()->GetName()) : FText::FromString(FString("NULL")))
		);
		NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.FadeInDuration = 0.3f;
		NotificationInfo.FadeOutDuration = 1.3f;
		NotificationInfo.ExpireDuration = 4.5f;

		FSlateNotificationManager::Get().AddNotification(NotificationInfo);
	}
}

void UJointEdGraph::OnNodeInstanceRemoved(UObject* NodeInstance)
{
	// empty in base class
}

const bool& UJointEdGraph::IsLocked() const
{
	return bIsLocked;
}

void UJointEdGraph::LockUpdates()
{
	bIsLocked = true;
}

void UJointEdGraph::UnlockUpdates()
{
	bIsLocked = false;
}

#undef LOCTEXT_NAMESPACE
