//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointEdGraph.h"

#include "JointManager.h"
#include "GraphEditAction.h"
#include "IMessageLogListing.h"
#include "JointEditorStyle.h"
#include "MessageLogModule.h"
#include "EdGraph/EdGraphSchema.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Node/JointEdGraphNode.h"
#include "Node/JointNodeBase.h"

#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"


#define LOCTEXT_NAMESPACE "UJointEdGraph"


void UJointEdGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RecacheNodes();

	if (!IsLocked())
	{
		ReallocateGraphNodesToJointManager();
		TryReinstancingUnknownNodeClasses();
		UpdateClassData();
		UpdateSubNodeChains();
		ResetGraphNodeToolkits();
		BindEdNodeEvents();
		UpdateDebugData();
		CompileJointGraph();
	}
}

void UJointEdGraph::NotifyGraphChanged()
{
	Super::NotifyGraphChanged();

	RecacheNodes();

	if (!IsLocked())
	{
		ReallocateGraphNodesToJointManager();
		TryReinstancingUnknownNodeClasses();
		UpdateClassData();
		UpdateSubNodeChains();
		ResetGraphNodeToolkits();
		BindEdNodeEvents();
		UpdateDebugData();
		CompileJointGraph();
	}
}

void UJointEdGraph::NotifyGraphChanged(const FEdGraphEditAction& InAction)
{
	Super::NotifyGraphChanged(InAction);

	RecacheNodes();
	
	if (!IsLocked())
	{
		ReallocateGraphNodesToJointManager();
		TryReinstancingUnknownNodeClasses();
		UpdateClassData();
		UpdateSubNodeChains();
		ResetGraphNodeToolkits();
		BindEdNodeEvents();
		UpdateDebugData();
		CompileJointGraph();
	}
	
}

void UJointEdGraph::RecacheNodes()
{
	CacheJointNodeInstances();
	CacheJointGraphNodes();
}

void UJointEdGraph::NotifyGraphRequestUpdate()
{

	RecacheNodes();
	
	if (!IsLocked())
	{
		if (OnGraphRequestUpdate.IsBound()) OnGraphRequestUpdate.Broadcast();
		
		ReallocateGraphNodesToJointManager();
		TryReinstancingUnknownNodeClasses();
		UpdateClassData();
		UpdateSubNodeChains();
		ResetGraphNodeToolkits();
		BindEdNodeEvents();
		UpdateDebugData();
		CompileJointGraph();
	}
}

FDelegateHandle UJointEdGraph::AddOnGraphRequestUpdateHandler(const FOnGraphRequestUpdate::FDelegate& InHandler)
{
	return OnGraphRequestUpdate.Add(InHandler);
}

void UJointEdGraph::RemoveOnGraphRequestUpdateHandler(FDelegateHandle Handle)
{
	OnGraphRequestUpdate.Remove(Handle);
}

void UJointEdGraph::OnCreated()
{
	//Do nothing.
}

void UJointEdGraph::ResetGraphNodeSlates()
{
	for (const TSoftObjectPtr<UJointEdGraphNode> GraphNode : CachedJointGraphNodes)
	{
		if(GraphNode) GraphNode->SetGraphNodeSlate(nullptr);
	}
}

void UJointEdGraph::ResetNodeDepth()
{
	for (const TSoftObjectPtr<UJointEdGraphNode> GraphNode : CachedJointGraphNodes)
	{
		if(GraphNode) GraphNode->RecalculateNodeDepth();
	}
}

void UJointEdGraph::ResetGraphNodeToolkits()
{
	for (const TSoftObjectPtr<UJointEdGraphNode> GraphNode : CachedJointGraphNodes)
	{
		if(GraphNode) GraphNode->OptionalToolkit = Toolkit;
	}
}

void UJointEdGraph::OnLoaded()
{
	CacheJointNodeInstances();
	CacheJointGraphNodes();
	
	TryReinstancingUnknownNodeClasses();
	
	BindEdNodeEvents();
	ResetGraphNodeToolkits();
	ResetGraphNodeSlates();
	ResetNodeDepth();
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
		if(!Value.CheckWhetherNecessary()) return true;
		
		return false;
	});
}

void UJointEdGraph::Initialize()
{
	ResetGraphNodeToolkits();
}

void UJointEdGraph::OnSave()
{
	TryReinstancingUnknownNodeClasses();
	UpdateClassData();
	ReconstructAllNodes();
	UpdateSubNodeChains();
}

void UJointEdGraph::OnClosed()
{
	CleanUpNodes();	
}

void UJointEdGraph::ReallocateGraphNodesToJointManager()
{
	if (!GetJointManager()) return;

	GetJointManager()->Nodes.Empty();

	for (UEdGraphNode* Node : Nodes)
	{
		if (Node == nullptr) continue;

		const UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node);

		if (CastedNode == nullptr) continue;

		UJointNodeBase* NodeInstance = CastedNode->GetCastedNodeInstance();

		if (NodeInstance == nullptr) continue;

		GetJointManager()->Nodes.Add(NodeInstance);
	}
}


void UJointEdGraph::ReconstructAllNodes(bool bPropagateUnder)
{
	for (UEdGraphNode* Node : Nodes)
	{
		UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node);

		if (!CastedNode) continue;

		if(bPropagateUnder)
		{
			CastedNode->ReconstructNodeInHierarchy();
		}else
		{
			CastedNode->ReconstructNode();
		}

	}

	NotifyGraphRequestUpdate();
}

void UJointEdGraph::CleanUpNodes()
{
	TSet<TSoftObjectPtr<UJointEdGraphNode>> GraphNodes = GetCachedJointGraphNodes();

	for (TSoftObjectPtr<UJointEdGraphNode> JointEdGraphNode : GraphNodes)
	{
		if(JointEdGraphNode.IsValid())
		{
			JointEdGraphNode->SetGraphNodeSlate(nullptr);
		}
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

void UJointEdGraph::PatchNodePickers()
{
	
	for (UEdGraphNode* Node : Nodes)
	{
		if(!Node)  continue;
	}
}

void UJointEdGraph::ExecuteForAllNodesInHierarchy(const TFunction<void(UEdGraphNode*)>& Func)
{
	const TSet<TSoftObjectPtr<UJointEdGraphNode>>& CachedNodes = GetCachedJointGraphNodes();

	for (TSoftObjectPtr<UJointEdGraphNode> JointEdGraphNode : CachedNodes)
	{
		Func(JointEdGraphNode.Get());
	}
}

void UJointEdGraph::InitializeCompileResultIfNeeded()
{
	if (CompileResultPtr.IsValid()) return;

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


void UJointEdGraph::TryReinstancingUnknownNodeClasses()
{
	for (const TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		PatchNodeInstanceFromStoredNodeClass(EdGraphNode);
	}
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

	const double CompileStartTime = FPlatformTime::Seconds();

	CompileResultPtr->ClearMessages();

	for (TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		if (EdGraphNode == nullptr) continue;

		UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(EdGraphNode);

		CompileJointGraphForNode(CastedNode, true);
	}

	const double CompileEndTime = FPlatformTime::Seconds();

	if (OnCompileFinished.IsBound()) OnCompileFinished.Execute(
		UJointEdGraph::FJointGraphCompileInfo(GetCachedJointGraphNodes().Num(),(CompileEndTime - CompileStartTime)));
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


void UJointEdGraph::BindEdNodeEvents()
{
	for (const TSoftObjectPtr<UJointEdGraphNode> GraphNode : CachedJointGraphNodes)
	{
		if(GraphNode) GraphNode->BindNodeInstance();
	}
}

void UJointEdGraph::BeginCacheForCookedPlatformData(const ITargetPlatform* TargetPlatform)
{
	
	CompileJointGraph();
	
	Super::BeginCacheForCookedPlatformData(TargetPlatform);
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

void UJointEdGraph::GrabUnknownClassDataFromGraph()
{
	for (TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		UJointEdGraphNode* Node = Cast<UJointEdGraphNode>(EdGraphNode);

		GrabUnknownClassDataFromNode(Node, true);
	}
}

void UJointEdGraph::GrabUnknownClassDataFromNode(UJointEdGraphNode* Node, const bool bPropagateToSubNodes)
{
	if(!Node) return;
	
	if(!Node->NodeClassData.GetClass())
	{
		FJointGraphNodeClassHelper::AddUnknownClass(Node->NodeClassData);
	}

	if(bPropagateToSubNodes)
	{
		for (UJointEdGraphNode* SubNode : Node->SubNodes)
		{
			GrabUnknownClassDataFromNode(SubNode, bPropagateToSubNodes);
		}
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

UEdGraphNode* UJointEdGraph::FindGraphNodeForNodeInstance(const UObject* NodeInstance)
{
	if (NodeInstance == nullptr) return nullptr;

	CacheJointGraphNodes();

	for (TSoftObjectPtr<UJointEdGraphNode> CachedJointGraphNode : CachedJointGraphNodes)
	{
		if (CachedJointGraphNode == nullptr) continue;

		if (CachedJointGraphNode.Get()->NodeInstance == NodeInstance) return CachedJointGraphNode.Get();
	}

	return nullptr;
}


TSet<TSoftObjectPtr<UObject>> UJointEdGraph::GetCachedJointNodeInstances(const bool bForce)
{
	if (bForce || CachedJointNodeInstances.IsEmpty()) CacheJointNodeInstances();

	return CachedJointNodeInstances;
}

TSet<TSoftObjectPtr<UJointEdGraphNode>> UJointEdGraph::GetCachedJointGraphNodes(const bool bForce)
{
	if (bForce || CachedJointGraphNodes.IsEmpty()) CacheJointGraphNodes();

	return CachedJointGraphNodes;
}


void CollectInstances(TSet<TSoftObjectPtr<UObject>>& NodeInstances, TObjectPtr<UEdGraphNode> Node)
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
	CachedJointNodeInstances.Empty();

	for (const TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		CollectInstances(CachedJointNodeInstances, EdGraphNode);
	}
}

void CollectAllGraphNodesInternal(TSet<TSoftObjectPtr<UJointEdGraphNode>>& GraphNodes, TObjectPtr<UEdGraphNode> Node)
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
	CachedJointGraphNodes.Empty();

	for (const TObjectPtr<UEdGraphNode> EdGraphNode : Nodes)
	{
		CollectAllGraphNodesInternal(CachedJointGraphNodes, EdGraphNode);
	}
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
	if(count > 0)
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

		if(GetJointManager() != nullptr) GetJointManager()->MarkPackageDirty();
	}else
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

void UJointEdGraph::OnNodesPasted(const FString& ImportStr)
{
	// empty in base class
}

#undef LOCTEXT_NAMESPACE
