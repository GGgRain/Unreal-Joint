//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointEditorFunctionLibrary.h"

#include "JointEdUtils.h"
#include "Markdown/SJointMDSlate_Admonitions.h"
#include "Misc/EngineVersionComparison.h"
#include "Node/JointFragment.h"
#include "Node/JointNodeBase.h"

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

UJointEdGraphNode* UJointEditorFunctionLibrary::AddBaseNode(UJointManager* TargetJointManager, UEdGraph* OptionalTargetGraph, const TSubclassOf<UJointNodeBase> NodeClass, const FVector2D& NodePosition)
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
	UJointNodeBase* NewNodeInstance = NewObject<UJointNodeBase>(TargetJointManager, NodeClass);
	
	//create the ed graph node
	TSubclassOf<UJointEdGraphNode> TargetEdClass = FJointEdUtils::FindEdClassForNode(NodeClass.Get());
	
	if (TargetEdClass)
	{
		UJointEdGraphNode* NewEdNode = NewObject<UJointEdGraphNode>(FinalTargetGraph, TargetEdClass);
		NewEdNode->NodeInstance = NewNodeInstance;
		NewEdNode->CreateNewGuid();
		NewEdNode->NodePosX = NodePosition.X;
		NewEdNode->NodePosY = NodePosition.Y;
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

UJointEdGraphNode* UJointEditorFunctionLibrary::AddSubNode(UJointManager* TargetJointManager, UJointEdGraphNode* ParentEdNode, TSubclassOf<UJointNodeBase> NodeClass)
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
	UJointNodeBase* NewNodeInstance = NewObject<UJointNodeBase>(TargetJointManager, NodeClass);
	
	//create the ed graph node
	TSubclassOf<UJointEdGraphNode> TargetEdClass = FJointEdUtils::FindEdClassForNode(NodeClass.Get());
	
	if (TargetEdClass)
	{
		UJointEdGraphNode* NewEdNode = NewObject<UJointEdGraphNode>(ParentGraph, TargetEdClass);
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

UJointEdGraphNode* UJointEditorFunctionLibrary::AddOrGetBaseNodeGraphNodeForNodeGuid(UJointManager* TargetJointManager, const FGuid& NodeGuid, UEdGraph* OptionalTargetGraph, TSubclassOf<UJointNodeBase> NodeClass,
	const FVector2D& NodePosition)
{
	if (!TargetJointManager) return nullptr;
	
	UJointEdGraphNode* ExistingNode = GetBaseNodeGraphNodeForNodeGuid(TargetJointManager, NodeGuid);
	
	if (ExistingNode) return ExistingNode;
	
	return AddBaseNode(TargetJointManager, OptionalTargetGraph, NodeClass, NodePosition);
}

UJointEdGraphNode* UJointEditorFunctionLibrary::AddOrGetFragmentGraphNodeForNodeGuid(UJointManager* TargetJointManager, const FGuid& NodeGuid, UJointEdGraphNode* ParentEdNode, TSubclassOf<UJointNodeBase> NodeClass)
{
	if (!TargetJointManager) return nullptr;
	
	UJointEdGraphNode* ExistingNode = GetFragmentGraphNodeForNodeGuid(TargetJointManager, NodeGuid);
	
	if (ExistingNode) return ExistingNode;
	
	return AddSubNode(TargetJointManager, ParentEdNode, NodeClass);
}

void UJointEditorFunctionLibrary::FitBaseNodeSizeToContent(UJointEdGraphNode* TargetEdNode)
{
}

#undef LOCTEXT_NAMESPACE