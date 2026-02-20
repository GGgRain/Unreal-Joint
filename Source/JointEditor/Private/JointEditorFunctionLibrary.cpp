//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointEditorFunctionLibrary.h"

#include "JointEdGraphNode_Foundation.h"
#include "JointEdGraphNode_Fragment.h"
#include "JointEdGraphSchema.h"
#include "JointEditorLogChannels.h"
#include "JointEdUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GraphNode/SJointGraphNodeBase.h"
#include "Markdown/SJointMDSlate_Admonitions.h"
#include "Misc/EngineVersionComparison.h"
#include "Node/JointFragment.h"
#include "Node/JointNodeBase.h"
#include "Script/JointScriptSettings.h"

#define LOCTEXT_NAMESPACE "JointEditorFunctionLibrary"

UJointEditorFunctionLibrary::UJointEditorFunctionLibrary()
{
	
}

UJointEdGraphNode* UJointEditorFunctionLibrary::GetBaseNodeGraphNodeForNodeGuid(const UJointManager* TargetJointManager, const FGuid& NodeGuid)
{
	if (!TargetJointManager) return nullptr;

	UJointNodeBase* Node = TargetJointManager->FindBaseNodeWithGuid(NodeGuid);
	
	if (!Node) return nullptr;
	
	UEdGraphNode* GraphNode = FJointEdUtils::FindGraphNodeForNodeInstance(Node);
	
	return GraphNode ? Cast<UJointEdGraphNode>(GraphNode) : nullptr;
}

UJointEdGraphNode* UJointEditorFunctionLibrary::GetFragmentGraphNodeForNodeGuid(const UJointManager* TargetJointManager, const FGuid& NodeGuid)
{
	if (!TargetJointManager) return nullptr;

	UJointFragment* Fragment = TargetJointManager->FindFragmentWithGuid(NodeGuid);
	
	if (!Fragment) return nullptr;
	
	UEdGraphNode* GraphNode = FJointEdUtils::FindGraphNodeForNodeInstance(Fragment);
	
	return GraphNode ? Cast<UJointEdGraphNode>(GraphNode) : nullptr;
}

UJointEdGraphNode* UJointEditorFunctionLibrary::GetManagerFragmentGraphNodeForNodeGuid(const UJointManager* TargetJointManager, const FGuid& NodeGuid)
{
	if (!TargetJointManager) return nullptr;

	UJointFragment* Fragment = TargetJointManager->FindManagerFragmentWithGuid(NodeGuid);
	
	if (!Fragment) return nullptr;
	
	UEdGraphNode* GraphNode = FJointEdUtils::FindGraphNodeForNodeInstance(Fragment);
	
	return GraphNode ? Cast<UJointEdGraphNode>(GraphNode) : nullptr;
}

UJointEdGraphNode* UJointEditorFunctionLibrary::AddBaseNode(
	UJointManager* TargetJointManager, 
	UEdGraph* OptionalTargetGraph, 
	const TSubclassOf<UJointNodeBase> NodeClass, 
	const FVector2D NodePosition,
	const FVector2D NodeSize)
{
	if (!TargetJointManager || !NodeClass) return nullptr;

	//check if NodeClass is not abstract & derived from UJointFragment
	
	if (NodeClass->HasAnyClassFlags(CLASS_Abstract) || NodeClass->IsChildOf(UJointFragment::StaticClass()))
	{
		 FJointEdUtils::FireNotification(
		 	LOCTEXT("JointEditorFunctionLibrary_AddBaseNode_InvalidNodeClass_Title", "Invalid Joint Node Class"),
		 	LOCTEXT("JointEditorFunctionLibrary_AddBaseNode_InvalidNodeClass_Message", "The provided Joint Node class is either abstract or derived from UJointFragment, which cannot be added as a base node."),
		 	EJointMDAdmonitionType::Error,
		 	10
		 );
	}
	
	UJointEdGraph* FinalTargetGraph = nullptr;
	UJointEdGraph* RootGraph = TargetJointManager->GetJointGraphAs<UJointEdGraph>();

	if (OptionalTargetGraph)
	{
		// check if TargetJointManager has this graph
		if ( RootGraph->GetAllSubGraphsRecursively().Contains(Cast<UJointEdGraph>(OptionalTargetGraph)) || RootGraph == OptionalTargetGraph)
		{
			FinalTargetGraph = Cast<UJointEdGraph>(OptionalTargetGraph);
		}
		else
		{
			 FJointEdUtils::FireNotification(
			 	LOCTEXT("JointEditorFunctionLibrary_AddBaseNode_InvalidTargetGraph_Title", "Invalid Target Graph"),
			 	LOCTEXT("JointEditorFunctionLibrary_AddBaseNode_InvalidTargetGraph_Message", "The provided Target Graph does not belong to the Target Joint Manager."),
			 	EJointMDAdmonitionType::Error,
			 	10
			 );
			return nullptr;
		}
	}else
	{
		//if TargetGraph is null, use the root graph
		FinalTargetGraph = RootGraph;
	}
	
	//create the node
	UJointNodeBase* NewNodeInstance = NewObject<UJointNodeBase>(TargetJointManager, NodeClass, NAME_None, RF_Transactional);
	
	//create the ed graph node
	TSubclassOf<UJointEdGraphNode> TargetEdClass = FJointEdUtils::FindEdClassForNode(NodeClass.Get());
	
	if (!TargetEdClass) TargetEdClass = UJointEdGraphNode_Foundation::StaticClass();
	
	if (TargetEdClass)
	{
		UJointEdGraphNode* NewEdNode = NewObject<UJointEdGraphNode>(FinalTargetGraph, TargetEdClass, NAME_None, RF_Transactional);
		NewEdNode->NodeInstance = NewNodeInstance;
		NewEdNode->CreateNewGuid();
		NewEdNode->NodePosX = NodePosition.X;
		NewEdNode->NodePosY = NodePosition.Y;
		NewEdNode->ResizeNode(NodeSize);
		NewEdNode->NodeClassData = FJointEdUtils::FindClassDataForNodeClass(NodeClass);

		//Set up pins after placing node
		NewEdNode->AllocateDefaultPins();
		NewEdNode->UpdatePins();

		//Set up the connection from the from pin if needed.
		//MakeConnectionFromTheDraggedPin(FromPin, ResultNode);

		//Add the newly created node to the Joint graph.
		FinalTargetGraph->AddNode(NewEdNode, true);

		return NewEdNode;
	}
	
	return nullptr;
}

UJointEdGraphNode* UJointEditorFunctionLibrary::AddFragment(
	UJointManager* TargetJointManager, 
	UJointEdGraphNode* ParentEdNode,
	TSubclassOf<UJointNodeBase> NodeClass
)
{
	if (!TargetJointManager || !ParentEdNode || !NodeClass) return nullptr;

	//check if NodeClass is not abstract
	if (NodeClass->HasAnyClassFlags(CLASS_Abstract))
	{
		 FJointEdUtils::FireNotification(
		 	LOCTEXT("JointEditorFunctionLibrary_AddSubNode_InvalidNodeClass_Title", "Invalid Joint Node Class"),
		 	LOCTEXT("JointEditorFunctionLibrary_AddSubNode_InvalidNodeClass_Message", " The provided Joint Node class is abstract"),
		 	EJointMDAdmonitionType::Error,
		 	10
		 );
		
		return nullptr;
	}
	
	UJointEdGraph* ParentGraph = Cast<UJointEdGraph>(ParentEdNode->GetGraph());
	
	if (!ParentGraph)
	{
		 FJointEdUtils::FireNotification(
		 	LOCTEXT("JointEditorFunctionLibrary_AddSubNode_InvalidParentGraph_Title", "Invalid Parent Graph"),
		 	LOCTEXT("JointEditorFunctionLibrary_AddSubNode_InvalidParentGraph_Message", "The parent node does not belong to a valid Joint Ed Graph."),
		 	EJointMDAdmonitionType::Error,
		 	10
		 );
		return nullptr;
	}
	
	UJointNodeBase* ParentNodeInstance = ParentEdNode->GetCastedNodeInstance();
	
	//create the node
	UJointNodeBase* NewNodeInstance = NewObject<UJointNodeBase>(TargetJointManager, NodeClass, NAME_None, RF_Transactional);
	
	//create the ed graph node
	TSubclassOf<UJointEdGraphNode> TargetEdClass = FJointEdUtils::FindEdClassForNode(NodeClass.Get());
	
	if (!TargetEdClass) TargetEdClass = UJointEdGraphNode_Fragment::StaticClass();

	if (TargetEdClass)
	{
		UJointEdGraphNode* NewEdNode = NewObject<UJointEdGraphNode>(ParentGraph, TargetEdClass, NAME_None, RF_Transactional);
		NewEdNode->NodeInstance = NewNodeInstance;
		NewEdNode->CreateNewGuid();
		NewEdNode->PostPlacedNewNode();
		NewEdNode->AllocateDefaultPins();
		NewEdNode->NodeClassData = FJointEdUtils::FindClassDataForNodeClass(NodeClass);

		// Add to parent node
		ParentEdNode->AddSubNode(NewEdNode);
		
		if(UJointEdGraph* CastedGraph = ParentEdNode->GetCastedGraph())
		{
			NewEdNode->OptionalToolkit = CastedGraph->GetToolkit();
			ParentEdNode->GetCastedGraph()->CacheJointGraphNodes();
		}
		
		ParentEdNode->Update();
		
		return NewEdNode;
	}
	
	return nullptr;
}

UJointEdGraphNode* UJointEditorFunctionLibrary::AddManagerFragment(
	UJointManager* TargetJointManager, 
	TSubclassOf<UJointNodeBase> NodeClass)
{
	if (!TargetJointManager || !NodeClass) return nullptr;

	UJointEdGraph* RootGraph = TargetJointManager->GetJointGraphAs<UJointEdGraph>();
	
	if (!RootGraph) return nullptr;
	
	return AddFragment(
		TargetJointManager,
		RootGraph->FindEntryNode(),
		NodeClass
	);
}

bool UJointEditorFunctionLibrary::RemoveNodeFromJointManager(UJointEdGraphNode* TargetEdNode)
{
	if (!TargetEdNode) return false;
	
	FJointEdUtils::RemoveNode(TargetEdNode);
	
	return true;
}

bool UJointEditorFunctionLibrary::RemoveNodesByIds(
	UJointManager* TargetJointManager,
	const FJointScriptLinkerFileEntry& FileEntry, 
	const TArray<FString>& KnownIds)
{
	if (!TargetJointManager) return false;
	
	for (const FString& KnownId : KnownIds)
	{
		TArray<UJointEdGraphNode*> NodesToRemove = GetBaseNodesById(TargetJointManager, FileEntry, KnownId);
		NodesToRemove.Append(GetFragmentNodesById(TargetJointManager, FileEntry, KnownId));
		NodesToRemove.Append(GetManagerFragmentNodesById(TargetJointManager, FileEntry, KnownId));
		
		for (UJointEdGraphNode* NodeToRemove : NodesToRemove)
		{
			FJointEdUtils::RemoveNode(NodeToRemove);
		}
	}
	
	return true;
}

TArray<UJointEdGraphNode*> UJointEditorFunctionLibrary::GetBaseNodesById(
	const UJointManager* TargetJointManager, 
	const FJointScriptLinkerFileEntry& FileEntry,
	const FString& KnownId)
{
	TArray<UJointEdGraphNode*> ResultNodes;

	FJointScriptLinkerDataElement* LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(FileEntry));
	
	if (!LinkElem) return ResultNodes;
	
	for (FJointScriptLinkerMapping& Mapping : LinkElem->Mappings)
	{
		if (Mapping.JointManager == TargetJointManager)
		{
			if (FJointScriptLinkerNodeSet* FoundGuid = Mapping.NodeMappings.Find(KnownId))
			{
				for (const FGuid& NodeGuid : FoundGuid->NodeGuids)
				{
					if (UJointEdGraphNode* ExistingNode = GetBaseNodeGraphNodeForNodeGuid(TargetJointManager, NodeGuid))
					{
						ResultNodes.Add(ExistingNode);
					}
				}
			}
		}
	}
	
	return ResultNodes;
}

TArray<UJointEdGraphNode*> UJointEditorFunctionLibrary::GetOrCreateBaseNodesById(
	UJointManager* TargetJointManager,
	const FJointScriptLinkerFileEntry& FileEntry,
	const FString& KnownId, 
	UEdGraph* OptionalTargetGraphForCreation,
	TSubclassOf<UJointNodeBase> NodeClassForCreation,
	const FVector2D NodePositionForCreation,
	bool& bCreatedNewNode,
	const FVector2D NodeSizeForCreation,
	bool bLinkWithScript,
	bool bClearIdIfCreatedNewNode)
{
	TArray<UJointEdGraphNode*> Arr = GetBaseNodesById(TargetJointManager, FileEntry, KnownId);
	
	if (Arr.Num() == 0)
	{
		UE_LOG(LogJointEditor, Log, TEXT("No existing Base nodes found for KnownId: %s, creating new one, OptionalTargetGraphForCreation: %s, NodeClassForCreation: %s"),
		       *KnownId,
		       OptionalTargetGraphForCreation ? *OptionalTargetGraphForCreation->GetName() : TEXT("NULL"),
		       *NodeClassForCreation->GetName()
		);
		
		UJointEdGraphNode* NewNode = AddBaseNode(
			TargetJointManager,
			OptionalTargetGraphForCreation,
			NodeClassForCreation,
			NodePositionForCreation,
			NodeSizeForCreation
		);
		
		if (NewNode)
		{
			Arr.Add(NewNode);
			
			if (bClearIdIfCreatedNewNode)
			{
				ClearIdKeyFromScriptLinkage(FileEntry, KnownId);
			}
			
			if (bLinkWithScript)
			{
				LinkNodeWithScript(NewNode, FileEntry, KnownId);
			}
			
			bCreatedNewNode = true;
		}
	}
	
	return Arr;
}

TArray<UJointEdGraphNode*> UJointEditorFunctionLibrary::GetFragmentNodesById(
	const UJointManager* TargetJointManager, 
	const FJointScriptLinkerFileEntry& FileEntry,
	const FString& KnownId)
{
	TArray<UJointEdGraphNode*> ResultNodes;
	
	FJointScriptLinkerDataElement* LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(FileEntry));
	
	if (!LinkElem) return ResultNodes;
	
	for (FJointScriptLinkerMapping& Mapping : LinkElem->Mappings)
	{
		if (Mapping.JointManager == TargetJointManager)
		{
			if (FJointScriptLinkerNodeSet* FoundGuid = Mapping.NodeMappings.Find(KnownId))
			{
				for (const FGuid& NodeGuid : FoundGuid->NodeGuids)
				{
					if (UJointEdGraphNode* ExistingNode = GetFragmentGraphNodeForNodeGuid(TargetJointManager, NodeGuid))
					{
						ResultNodes.Add(ExistingNode);
					}
				}
			}
		}
	}
	
	return ResultNodes;
}

TArray<UJointEdGraphNode*> UJointEditorFunctionLibrary::GetOrCreateFragmentNodesByIds(
	UJointManager* TargetJointManager,
	const FJointScriptLinkerFileEntry& FileEntry,
	const FString& KnownId,
	UJointEdGraphNode* ParentEdNodeForCreation,
	TSubclassOf<UJointNodeBase> NodeClassForCreation,
	bool& bCreatedNewNode,
	bool bLinkWithScript,
	bool bClearIdIfCreatedNewNode)
{
	TArray<UJointEdGraphNode*> Arr = GetFragmentNodesById(TargetJointManager, FileEntry, KnownId);
	
	if (Arr.Num() == 0)
	{
		UE_LOG(LogJointEditor, Log, TEXT("No existing Fragment nodes found for KnownId: %s, creating new one, ParentEdNodeForCreation: %s, NodeClassForCreation: %s"),
		       *KnownId,
		       ParentEdNodeForCreation ? *ParentEdNodeForCreation->GetName() : TEXT("NULL"),
		       *NodeClassForCreation->GetName()
		);
		
		UJointEdGraphNode* NewNode = AddFragment(
			TargetJointManager,
			ParentEdNodeForCreation,
			NodeClassForCreation
		);
		
		if (NewNode)
		{
			Arr.Add(NewNode);
			
			if (bClearIdIfCreatedNewNode)
			{
				ClearIdKeyFromScriptLinkage(FileEntry, KnownId);
			}
			
			if (bLinkWithScript)
			{
				LinkNodeWithScript(NewNode, FileEntry, KnownId);
			}
			
			bCreatedNewNode = true;
		}
	}
	
	return Arr;
}

TArray<UJointEdGraphNode*> UJointEditorFunctionLibrary::GetManagerFragmentNodesById(
	const UJointManager* TargetJointManager,
	const FJointScriptLinkerFileEntry& FileEntry, 
	const FString& KnownId)
{
	TArray<UJointEdGraphNode*> ResultNodes;
	
	FJointScriptLinkerDataElement* LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(FileEntry));
	
	if (!LinkElem) return ResultNodes;
	
	for (FJointScriptLinkerMapping& Mapping : LinkElem->Mappings)
	{
		if (Mapping.JointManager == TargetJointManager)
		{
			if (FJointScriptLinkerNodeSet* FoundGuid = Mapping.NodeMappings.Find(KnownId))
			{
				for (const FGuid& NodeGuid : FoundGuid->NodeGuids)
				{
					if (UJointEdGraphNode* ExistingNode = GetManagerFragmentGraphNodeForNodeGuid(TargetJointManager, NodeGuid))
					{
						ResultNodes.Add(ExistingNode);
					}
				}
			}
		}
	}
	
	return ResultNodes;
}

TArray<UJointEdGraphNode*> UJointEditorFunctionLibrary::GetOrCreateManagerFragmentNodesById(
	UJointManager* TargetJointManager,
	const FJointScriptLinkerFileEntry& FileEntry, 
	const FString& KnownId,
	TSubclassOf<UJointNodeBase> NodeClassForCreation, 
	bool& bCreatedNewNode,
	bool bLinkWithScript,
	bool bClearIdIfCreatedNewNode)
{
	TArray<UJointEdGraphNode*> Arr = GetManagerFragmentNodesById(TargetJointManager, FileEntry, KnownId);
	
	if (Arr.Num() == 0)
	{
		UE_LOG(LogJointEditor, Log, TEXT("No existing Manager Fragment nodes found for KnownId: %s, creating new one, NodeClassForCreation: %s"),
		       *KnownId,
		       *NodeClassForCreation->GetName()
		);
		
		UJointEdGraphNode* NewNode = AddManagerFragment(
			TargetJointManager,
			NodeClassForCreation
		);
		
		if (NewNode)
		{
			Arr.Add(NewNode);
			
			if (bClearIdIfCreatedNewNode)
			{
				ClearIdKeyFromScriptLinkage(FileEntry, KnownId);
			}
			
			if (bLinkWithScript)
			{
				LinkNodeWithScript(NewNode, FileEntry, KnownId);
			}
			
			bCreatedNewNode = true;
		}
	}
	
	return Arr;
}


void UJointEditorFunctionLibrary::LinkNodeWithScript(
	UJointEdGraphNode* TargetEdNode,
	const FJointScriptLinkerFileEntry& FileEntry,
	const FString& Id
)
{
	TargetEdNode->ExternalSourceEntry = FileEntry;
	
	UJointManager* TargetManager = TargetEdNode->GetJointManager();
	
	if (!TargetManager) return;
	
	// TODO: do setting set up here
	
	
	FJointScriptLinkerDataElement* LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(FileEntry));
	
	if (!LinkElem)
	{
		//create new entry
		FJointScriptLinkerDataElement NewLinkElem;
		
		NewLinkElem.FileEntry = FileEntry;
		NewLinkElem.Mappings = TArray<FJointScriptLinkerMapping>();
		
		UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.Add(NewLinkElem);
		LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(FileEntry));
	}
	
	//find or create mapping for the Joint manager
	FJointScriptLinkerMapping* TargetMapping = nullptr;
	
	for (FJointScriptLinkerMapping& Mapping : LinkElem->Mappings)
	{
		if (Mapping.JointManager == TargetManager)
		{
			TargetMapping = &Mapping;
			break;
		}
	}
	
	if (!TargetMapping)
	{
		//create new mapping
		FJointScriptLinkerMapping NewMapping;
		NewMapping.JointManager = TargetManager;
		LinkElem->Mappings.Add(NewMapping);
		TargetMapping = &LinkElem->Mappings.Last();
	}
	
	//add node mapping
	
	UJointNodeBase* NodeInstance = TargetEdNode->GetCastedNodeInstance();
	
	if (NodeInstance)
	{
		TargetMapping->FindOrAddNodeMappingSetForId(Id).NodeGuids.Add(NodeInstance->GetNodeGuid());
	}
	
}

void UJointEditorFunctionLibrary::LinkParserWithScript(UJointScriptParser* Parser, const FJointScriptLinkerFileEntry& FileEntry)
{
	if (!Parser) return;
	
	FJointScriptLinkerDataElement* LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(FileEntry));
	
	if (!LinkElem)
	{
		//create new entry
		FJointScriptLinkerDataElement NewLinkElem;
		
		NewLinkElem.FileEntry = FileEntry;
		NewLinkElem.Mappings = TArray<FJointScriptLinkerMapping>();
		
		UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.Add(NewLinkElem);
		LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(FileEntry));
	}
	
	TSubclassOf<UJointScriptParser> AssetClassForCDO = nullptr;

	if (Parser->HasAnyFlags(RF_ClassDefaultObject))
	{
		UClass* ParserClass = Parser->GetClass();
		
		AssetClassForCDO = ParserClass;

		if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(ParserClass))
		{
			if (UBlueprint* BlueprintAsset = Cast<UBlueprint>(BPClass->ClassGeneratedBy))
			{
				if (UClass* GeneratedClass = BlueprintAsset->GeneratedClass)
				{
					AssetClassForCDO = GeneratedClass;
				}
			}
		}
	}
	
	LinkElem->ParserData.ScriptParser = AssetClassForCDO;
	
	// Serialize the object.
	FMemoryWriter MemoryWriter(LinkElem->ParserData.ParserInstanceData, true);
	Parser->Serialize(MemoryWriter);
}

void UJointEditorFunctionLibrary::UnlinkNodeFromScript(
	UJointEdGraphNode* TargetEdNode
)
{
	if (!TargetEdNode) return;
	
	UJointManager* TargetManager = TargetEdNode->GetJointManager();
	
	if (!TargetManager) return;
	
	const FJointScriptLinkerFileEntry& ExistingEntry = TargetEdNode->ExternalSourceEntry;
	
	// TODO: do setting clean up
	FJointScriptLinkerDataElement* LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(ExistingEntry));
	
	TargetEdNode->ExternalSourceEntry = FJointScriptLinkerFileEntry();
	
	if (!LinkElem)
	{
		FJointEdUtils::FireNotification(
			LOCTEXT("JointEditorFunctionLibrary_UnlinkNodeFromScript_NoMappingFound_Title", "No Mapping Found"),
			LOCTEXT("JointEditorFunctionLibrary_UnlinkNodeFromScript_NoMappingFound_Message", "ExistingEntry mapping not found in the Joint Script Settings."),
			EJointMDAdmonitionType::Warning,
			5
		);
		
		//just update the node and return
		TargetEdNode->Update();
		
		return;
	}
	
	for (FJointScriptLinkerMapping& Mapping : LinkElem->Mappings)
	{
		Mapping.RemoveNodeGuidFromAllMappings(TargetEdNode->GetCastedNodeInstance()->GetNodeGuid());
	}
	
	TargetEdNode->Update();
	

}

void UJointEditorFunctionLibrary::ClearIdKeyFromScriptLinkage(const FJointScriptLinkerFileEntry& FileEntry, const FString& Id)
{
	FJointScriptLinkerDataElement* LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(FileEntry));
	
	if (!LinkElem) return;
	
	for (FJointScriptLinkerMapping& Mapping : LinkElem->Mappings)
	{
		Mapping.NodeMappings.Remove(Id);
	}
}

void UJointEditorFunctionLibrary::LinkNodesWithScriptBulk(const TArray<UJointEdGraphNode*>& TargetEdNodes, const FJointScriptLinkerFileEntry& FileEntry, const FString& Id)
{
	for (UJointEdGraphNode* EdNode : TargetEdNodes)
	{
		LinkNodeWithScript(EdNode, FileEntry, Id);
	}
}

void UJointEditorFunctionLibrary::UnlinkNodesFromScriptBulk(const TArray<UJointEdGraphNode*>& TargetEdNodes)
{
	for (UJointEdGraphNode* EdNode : TargetEdNodes)
	{
		UnlinkNodeFromScript(EdNode);
	}
}

UJointNodeBase* UJointEditorFunctionLibrary::GetNodeInstanceAs(const UJointEdGraphNode* TargetEdNode, const TSubclassOf<UJointNodeBase> NodeClass)
{
	if (!TargetEdNode) return nullptr;

	UJointNodeBase* NodeInstance = TargetEdNode->GetCastedNodeInstance();
	
	if (!NodeInstance) return nullptr;
	
	TSubclassOf<UJointNodeBase> FinalNodeClass = NodeClass.Get() ? NodeClass.Get() : UJointNodeBase::StaticClass();
	
	if (NodeInstance->IsA(FinalNodeClass))
	{
		return NodeInstance;
	}
	
	return nullptr;
}

FJointScriptLinkerNodeSet UJointEditorFunctionLibrary::GetNodeGuidsLinkedWithScript(UJointManager* TargetJointManager, const FJointScriptLinkerFileEntry& FileEntry, const FString& Id)
{
	FJointScriptLinkerDataElement* LinkElem = UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks.FindByKey(FJointScriptLinkerDataElement(FileEntry));
	
	if (!LinkElem) return FJointScriptLinkerNodeSet();
	
	for (FJointScriptLinkerMapping& Mapping : LinkElem->Mappings)
	{
		if (Mapping.JointManager == TargetJointManager)
		{
			if (FJointScriptLinkerNodeSet* FoundGuid = Mapping.NodeMappings.Find(Id))
			{
				return *FoundGuid;
			}
		}
	}
	
	return FJointScriptLinkerNodeSet();
}

void UJointEditorFunctionLibrary::SetBaseNodeFitSizeToContent(UJointEdGraphNode* TargetEdNode, bool bFit)
{
	if (!TargetEdNode) return;
	
	TargetEdNode->DefaultEdNodeSetting.bIsNodeResizeable = !bFit;
	
	if (UJointNodeBase* NodeInstance = TargetEdNode->GetCastedNodeInstance())
	{
		NodeInstance->EdNodeSetting.bIsNodeResizeable = !bFit;
	}
}

void UJointEditorFunctionLibrary::ResizeNode(UJointEdGraphNode* TargetEdNode, const FVector2D& NewSize)
{
	if (!TargetEdNode) return;
	
	TargetEdNode->ResizeNode(NewSize);
}

FVector2D UJointEditorFunctionLibrary::GetNodePosition(const UJointEdGraphNode* TargetEdNode)
{
	if (!TargetEdNode) return FVector2D::ZeroVector;
	
	return FVector2D(TargetEdNode->NodePosX, TargetEdNode->NodePosY);
}

void UJointEditorFunctionLibrary::SetNodePosition(UJointEdGraphNode* TargetEdNode, const FVector2D& NewPosition)
{
	if (!TargetEdNode) return;
	
	TargetEdNode->NodePosX = NewPosition.X;
	TargetEdNode->NodePosY = NewPosition.Y;
}

FJointEdNodeSetting UJointEditorFunctionLibrary::GetEdNodeSettings(UJointEdGraphNode* TargetEdNode)
{
	if (!TargetEdNode) return FJointEdNodeSetting();
	
	// if there is a node instance, return the setting from the node instance, otherwise return the default setting from the ed node 
	if (UJointNodeBase* NodeInstance = TargetEdNode->GetCastedNodeInstance())
	{
		return NodeInstance->EdNodeSetting;
	}
	else
	{
		return TargetEdNode->DefaultEdNodeSetting;
	}
}

void UJointEditorFunctionLibrary::SetEdNodeSettings(UJointEdGraphNode* TargetEdNode, const FJointEdNodeSetting& NewSettings)
{
	if (!TargetEdNode) return;
	
	TargetEdNode->DefaultEdNodeSetting = NewSettings;
	
	if (UJointNodeBase* NodeInstance = TargetEdNode->GetCastedNodeInstance())
	{
		NodeInstance->EdNodeSetting = NewSettings;
	}
}

void UJointEditorFunctionLibrary::SetSubNodeBoxOrientation(UJointEdGraphNode* TargetEdNode, EOrientation NewOrientation)
{
	if (!TargetEdNode) return;
	
	TargetEdNode->SubNodeBoxOrientation = NewOrientation;
}

void UJointEditorFunctionLibrary::ConnectPins(UJointEdGraphNode* ANode, FJointEdPinData APin, UJointEdGraphNode* BNode, FJointEdPinData BPin)
{
	if (!ANode || !BNode) return;
	
	UEdGraphPin* APinPtr = ANode->GetPinForPinDataFromHierarchy(APin);
	UEdGraphPin* BPinPtr = BNode->GetPinForPinDataFromHierarchy(BPin);
	
	if (APinPtr && BPinPtr)
	{
		FJointEdUtils::TryMakeConnectionBetweenPins(APinPtr,BPinPtr);
	}
}

void UJointEditorFunctionLibrary::DisconnectPins(UJointEdGraphNode* Node, FJointEdPinData Pin)
{
	if (!Node) return;
	
	UEdGraphPin* TargetPin = Node->GetPinForPinDataFromHierarchy(Pin);
	
	if (TargetPin)
	{
		TargetPin->BreakAllPinLinks();
	}
}

void UJointEditorFunctionLibrary::ConnectToRootNode(UJointEdGraphNode* Node, FJointEdPinData Pin)
{
	if (!Node) return;
	
	if (UJointEdGraph* Graph = Node->GetCastedGraph())
	{
		if (UJointEdGraphNode* Entry = Graph->FindEntryNode())
		{
			UEdGraphPin* TargetPin = Node->GetPinForPinDataFromHierarchy(Pin);
			UEdGraphPin* EntryPin = nullptr;
			
			if (TargetPin && Entry)
			{
				//find the appropriate pin from the entry node
				if (TargetPin->Direction == EGPD_Input)
				{
					EntryPin = Entry->GetPinForPinDataFromHierarchy(GetOutputPin(Entry, 0, true));
				}
				else if (TargetPin->Direction == EGPD_Output)
				{
					EntryPin = Entry->GetPinForPinDataFromHierarchy(GetInputPin(Entry, 0, true));
				}
				
				if (EntryPin) FJointEdUtils::TryMakeConnectionBetweenPins(TargetPin,EntryPin);
			}
		}
	}
}

FJointEdPinData UJointEditorFunctionLibrary::GetInputPin(
	UJointEdGraphNode* TargetEdNode,
	int Index,
	bool bSearchSubNodes)
{
	int CurrentIndex = -1;
	
	if (!TargetEdNode) return FJointEdPinData::PinData_Null;
	
	for (const FJointEdPinData& PinData : TargetEdNode->GetPinDataFromThis())
	{
		if (PinData.Direction == EGPD_Input)
		{
			CurrentIndex++;
			
			if (CurrentIndex == Index)
			{
				return PinData;
			}
		}
	}
	
	if (bSearchSubNodes)
	{
		for (UJointEdGraphNode* SubNode : TargetEdNode->GetAllSubNodesInHierarchy())
		{
			for (const FJointEdPinData& PinData : SubNode->GetPinDataFromThis())
			{
				if (PinData.Direction == EGPD_Input)
				{
					CurrentIndex++;
			
					if (CurrentIndex == Index)
					{
						return PinData;
					}
				}
			}
		}
	}
	
	return FJointEdPinData::PinData_Null;
}

TArray<FJointEdPinData> UJointEditorFunctionLibrary::GetInputPins(UJointEdGraphNode* TargetEdNode, bool bSearchSubNodes)
{
	TArray<FJointEdPinData> ResultPins;
	
	if (!TargetEdNode) return ResultPins;
	
	for (const FJointEdPinData& PinData : TargetEdNode->GetPinDataFromThis())
	{
		if (PinData.Direction == EGPD_Input)
		{
			ResultPins.Add(PinData);
		}
	}
	
	if (bSearchSubNodes)
	{
		for (UJointEdGraphNode* SubNode : TargetEdNode->GetAllSubNodesInHierarchy())
		{
			for (const FJointEdPinData& PinData : SubNode->GetPinDataFromThis())
			{
				if (PinData.Direction == EGPD_Input)
				{
					ResultPins.Add(PinData);
				}
			}
		}
	}
	
	return ResultPins;
}

FJointEdPinData UJointEditorFunctionLibrary::GetOutputPin(
	UJointEdGraphNode* TargetEdNode, 
	int Index,
	bool bSearchSubNodes)
{
	int CurrentIndex = -1;
	
	if (!TargetEdNode) return FJointEdPinData::PinData_Null;
	
	for (const FJointEdPinData& PinData : TargetEdNode->GetPinDataFromThis())
	{
		if (PinData.Direction == EGPD_Output)
		{
			CurrentIndex++;
			
			if (CurrentIndex == Index)
			{
				return PinData;
			}
		}
	}
	
	if (bSearchSubNodes)
	{
		for (UJointEdGraphNode* SubNode : TargetEdNode->GetAllSubNodesInHierarchy())
		{
			for (const FJointEdPinData& PinData : SubNode->GetPinDataFromThis())
			{
				if (PinData.Direction == EGPD_Output)
				{
					CurrentIndex++;
			
					if (CurrentIndex == Index)
					{
						return PinData;
					}
				}
			}
		}
	}
	
	return FJointEdPinData::PinData_Null;
}

TArray<FJointEdPinData> UJointEditorFunctionLibrary::GetOutputPins(UJointEdGraphNode* TargetEdNode, bool bSearchSubNodes)
{
	TArray<FJointEdPinData> ResultPins;
	
	if (!TargetEdNode) return ResultPins;
	
	for (const FJointEdPinData& PinData : TargetEdNode->GetPinDataFromThis())
	{
		if (PinData.Direction == EGPD_Output)
		{
			ResultPins.Add(PinData);
		}
	}
	
	if (bSearchSubNodes)
	{
		for (UJointEdGraphNode* SubNode : TargetEdNode->GetAllSubNodesInHierarchy())
		{
			for (const FJointEdPinData& PinData : SubNode->GetPinDataFromThis())
			{
				if (PinData.Direction == EGPD_Output)
				{
					ResultPins.Add(PinData);
				}
			}
		}
	}
	
	return ResultPins;
}

void UJointEditorFunctionLibrary::ModifyObject(UObject* TargetObject)
{
	if (TargetObject)
	{
		TargetObject->Modify();
	}
}

FGameplayTag UJointEditorFunctionLibrary::GetGameplayTagFromString(const FString& TagString)
{
	FGameplayTag ResultTag;
	
	if (FGameplayTag::IsValidGameplayTagString(TagString))
	{
		ResultTag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
	}
	
	return ResultTag;
}
