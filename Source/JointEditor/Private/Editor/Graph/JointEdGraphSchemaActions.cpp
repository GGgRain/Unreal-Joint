//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointEdGraphSchemaActions.h"

#include "JointEdGraph.h"
#include "JointEdGraphNode.h"
#include "JointEdGraphNode_Connector.h"
#include "JointEdGraphNode_Foundation.h"
#include "JointManager.h"
#include "EdGraphNode_Comment.h"
#include "Editor.h"
#include "GraphEditor.h"
#include "JointEdGraphNode_Composite.h"
#include "Node/JointNodeBase.h"
#include "ScopedTransaction.h"
#include "GraphNode/SJointGraphNodeBase.h"
#include "Misc/EngineVersionComparison.h"


FJointSchemaAction_NewNode::FJointSchemaAction_NewNode()
	: FEdGraphSchemaAction(),
	  NodeTemplate(nullptr)
{
}

FJointSchemaAction_NewNode::FJointSchemaAction_NewNode(FText InNodeCategory, FText InMenuDesc, FText InToolTip,
                                                             const int32 InGrouping)
	: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping),
	  NodeTemplate(nullptr)
{
}

FJointSchemaAction_NewSubNode::FJointSchemaAction_NewSubNode()
	: FEdGraphSchemaAction(),
	  NodeTemplate(nullptr)
{
}

FJointSchemaAction_NewSubNode::FJointSchemaAction_NewSubNode(FText InNodeCategory, FText InMenuDesc,
                                                                   FText InToolTip, const int32 InGrouping)
	: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping),
	  NodeTemplate(nullptr)
{
}

UEdGraphNode* FJointSchemaAction_NewSubNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin,
                                                              const FVector2D Location, bool bSelectNewNode)
{

	//Check if we are okay to proceed.
	if (!NodeTemplate|| !ParentGraph) return nullptr;
	
	GEditor->BeginTransaction(FText::Format(NSLOCTEXT("JointEdTransaction", "TransactionTitle_AddNewSubNode", "Add new sub node (Fragment): {0}"), FText::FromString(NodeTemplate->NodeClassData.GetClass()->GetName())));

	//Notify the modification for the transaction.
	ParentGraph->Modify();

	if (NodeTemplate) NodeTemplate->Modify();
	
	for (UObject* Node : NodesToAttachTo)
	{
		if(Node == nullptr || !Node->IsValidLowLevel()) continue;

		if(UJointEdGraphNode* ParentJointEdGraphNode = Cast<UJointEdGraphNode>(Node); ParentJointEdGraphNode && ParentJointEdGraphNode->GetCastedNodeInstance())
		{
			ParentJointEdGraphNode->Modify();
			ParentJointEdGraphNode->GetCastedNodeInstance()->Modify();
		}
	}

	for (UObject* Node : NodesToAttachTo)
	{
		if(Node == nullptr) continue;

		if(UJointEdGraphNode* ParentJointEdGraphNode = Cast<UJointEdGraphNode>(Node); ParentJointEdGraphNode)
		{
			ParentJointEdGraphNode->Modify();
			if(ParentJointEdGraphNode->GetCastedNodeInstance()) ParentJointEdGraphNode->GetCastedNodeInstance()->Modify();

			UJointEdGraphNode* GraphNode = DuplicateObject<UJointEdGraphNode>(NodeTemplate, NodeTemplate->GetOuter());
		
			GraphNode->SetFlags(RF_Transactional);

			GraphNode->Rename(nullptr, ParentGraph, REN_NonTransactional);

			UJointNodeBase* NodeData = NewObject<UJointNodeBase>(ParentGraph->GetOuter(),
																	   NodeTemplate->NodeClassData.GetClass(), NAME_None,
																	   RF_Transactional);
			GraphNode->NodeInstance = NodeData;
			GraphNode->CreateNewGuid();
			GraphNode->PostPlacedNewNode();
			GraphNode->AllocateDefaultPins();
			
			ParentJointEdGraphNode->AddSubNode(GraphNode);

			if(UJointEdGraph* CastedGraph = ParentJointEdGraphNode->GetCastedGraph())
			{
				GraphNode->OptionalToolkit = CastedGraph->GetToolkit();

				ParentJointEdGraphNode->GetCastedGraph()->CacheJointGraphNodes();
			}
		}
	}

	

	GEditor->EndTransaction();
	
	return NULL;
}

UEdGraphNode* FJointSchemaAction_NewSubNode::PerformAction(class UEdGraph* ParentGraph,
                                                              TArray<UEdGraphPin*>& FromPins, const FVector2D Location,
                                                              bool bSelectNewNode)
{
	return PerformAction(ParentGraph, NULL, Location, bSelectNewNode);
}


void FJointSchemaAction_NewSubNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);

	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around


#if UE_VERSION_OLDER_THAN(5,3,0)
	Collector.AddReferencedObject(NodeTemplate);
	Collector.AddReferencedObjects(NodesToAttachTo);
#else
	TObjectPtr<UObject> NodeTemplateObj = NodeTemplate;
	Collector.AddReferencedObject(NodeTemplateObj);

	for (UObject* ToAttachTo : NodesToAttachTo)
	{
		if(ToAttachTo == nullptr) continue;
		TObjectPtr<UObject> ParentNodeObj = ToAttachTo;
		
		Collector.AddReferencedObject(ParentNodeObj);
	}
#endif
	
}


void FJointSchemaAction_NewNode::MakeConnectionFromTheDraggedPin(UEdGraphPin* FromPin, UJointEdGraphNode* ConnectedNode)
{
	if (FromPin == nullptr || ConnectedNode == nullptr) return;

	switch (FromPin->Direction)
	{
	case EGPD_Input:

		for (UEdGraphPin* AllPin : ConnectedNode->GetAllPins())
		{
			if (AllPin->Direction != EEdGraphPinDirection::EGPD_Output) continue;
			AllPin->Modify();
			AllPin->GetSchema()->TryCreateConnection(AllPin, FromPin);
		}

		break;

	case EGPD_Output:

		for (UEdGraphPin* AllPin : ConnectedNode->GetAllPins())
		{
			if (AllPin->Direction != EEdGraphPinDirection::EGPD_Input) continue;
			AllPin->Modify();
			AllPin->GetSchema()->TryCreateConnection(AllPin, FromPin);
		}

		break;

	case EGPD_MAX: break;

	default: break;
	}

	//Force the node to update its connections.
	if(UEdGraphNode* GraphNode = FromPin->GetOwningNode())
	{
		GraphNode->NodeConnectionListChanged();
	}

	ConnectedNode->NodeConnectionListChanged();
	
}

UEdGraphNode* FJointSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
														   const FVector2D Location, bool bSelectNewNode)
{
	UJointEdGraphNode* ResultNode = NodeTemplate;

	if (!ResultNode || !ResultNode->NodeClassData.GetClass()) return nullptr;
	if (!ParentGraph) return nullptr;

	UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(ParentGraph);

	if (!CastedGraph) return nullptr;

	UJointManager* Manager = CastedGraph->GetJointManager();

	if (!Manager) return nullptr;


	GEditor->BeginTransaction(FText::Format(NSLOCTEXT("JointEdTransaction", "TransactionTitle_AddNewNode", "Add new node: {0}"), FText::FromString(ResultNode->NodeClassData.GetClass()->GetName())));

	//Notify the modification for the transaction.
	ResultNode->Modify();
	ParentGraph->Modify();
	Manager->Modify();


	ResultNode->SetFlags(RF_Transactional);
	//Set outer to be the graph so it doesn't go away
	ResultNode->Rename(nullptr, ParentGraph, REN_NonTransactional);

	UJointNodeBase* NodeData = NewObject<UJointNodeBase>(ParentGraph->GetOuter(),
															   ResultNode->NodeClassData.GetClass(), NAME_None,
															   RF_Transactional);

	ResultNode->NodeInstance = NodeData;
	ResultNode->NodeClassData = FJointGraphNodeClassData(ResultNode->NodeClassData.GetClass(),
												FJointGraphNodeClassHelper::GetDeprecationMessage(NodeData->GetClass()));
	ResultNode->CreateNewGuid();
	ResultNode->NodePosX = Location.X;
	ResultNode->NodePosY = Location.Y;

	//Set up pins after placing node
	ResultNode->AllocateDefaultPins();
	ResultNode->UpdatePins();

	//Set up the connection from the from pin if needed.
	MakeConnectionFromTheDraggedPin(FromPin, ResultNode);

	//Add the newly created node to the Joint graph.
	ParentGraph->AddNode(ResultNode, true);

	GEditor->EndTransaction();


	return ResultNode;
}

UEdGraphNode* FJointSchemaAction_NewNode::PerformAction_Command(UEdGraph* ParentGraph, TSubclassOf<UJointEdGraphNode> EdClass, TSubclassOf<UJointNodeBase> NodeClass, const FVector2D Location,
	bool bSelectNewNode)
{
	if (!NodeClass || !ParentGraph || !EdClass) return nullptr;

	UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(ParentGraph);

	if (!CastedGraph) return nullptr;

	UJointEdGraphNode* ResultNode = NewObject<UJointEdGraphNode>(ParentGraph,EdClass);
	
	UJointManager* Manager = CastedGraph->GetJointManager();

	if (!Manager) return nullptr;

	GEditor->BeginTransaction(FText::Format(NSLOCTEXT("JointEdTransaction", "TransactionTitle_AddNewNode", "Add new node: {0}"), FText::FromString(NodeClass->GetName())));

	//Notify the modification for the transaction.
	ResultNode->Modify();
	ParentGraph->Modify();
	Manager->Modify();


	ResultNode->SetFlags(RF_Transactional);
	//Set outer to be the graph so it doesn't go away
	ResultNode->Rename(nullptr, ParentGraph, REN_NonTransactional);

	UJointNodeBase* NodeData = NewObject<UJointNodeBase>(Manager, NodeClass, NAME_None,RF_Transactional);

	ResultNode->NodeInstance = NodeData;
	ResultNode->NodeClassData = FJointGraphNodeClassData(NodeClass, FJointGraphNodeClassHelper::GetDeprecationMessage(NodeData->GetClass()));
	ResultNode->CreateNewGuid();
	ResultNode->NodePosX = Location.X;
	ResultNode->NodePosY = Location.Y;

	//Set up pins after placing node
	ResultNode->AllocateDefaultPins();
	ResultNode->UpdatePins();

	//Add the newly created node to the Joint graph.
	ParentGraph->AddNode(ResultNode, true);
	
	GEditor->EndTransaction();


	return ResultNode;
}


UEdGraphNode* FJointSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins,
                                                           const FVector2D Location, bool bSelectNewNode)
{
	if (FromPins.Num() > 0) return PerformAction(ParentGraph, FromPins[0], Location, bSelectNewNode);

	return PerformAction(ParentGraph, nullptr, Location, bSelectNewNode);
}

void FJointSchemaAction_NewNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around

#if UE_VERSION_OLDER_THAN(5,3,0)
	Collector.AddReferencedObject(NodeTemplate);
#else
	TObjectPtr<UObject> NodeTemplateObj = NodeTemplate;
	
	Collector.AddReferencedObject(NodeTemplateObj);
#endif

	
}


UEdGraphNode* FJointSchemaAction_AddComment::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin,
                                                              const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode_Comment* const CommentTemplate = NewObject<UEdGraphNode_Comment>();
	CommentTemplate->bCanRenameNode = true; // make it able to rename.

	FVector2D SpawnLocation = Location;
	FSlateRect Bounds;

	TSharedPtr<SGraphEditor> GraphEditorPtr = SGraphEditor::FindGraphEditorForGraph(ParentGraph);
	if (GraphEditorPtr.IsValid() && GraphEditorPtr->GetBoundsForSelectedNodes(/*out*/ Bounds, 50.0f))
	{
		CommentTemplate->SetBounds(Bounds);
		SpawnLocation.X = CommentTemplate->NodePosX;
		SpawnLocation.Y = CommentTemplate->NodePosY;
	}

	UEdGraphNode* const NewNode = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(
		ParentGraph, CommentTemplate, SpawnLocation, bSelectNewNode);

	return NewNode;
}

UEdGraphNode* FJointSchemaAction_AddConnector::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
                                                                const FVector2D Location, bool bSelectNewNode)
{
	UJointEdGraphNode_Connector* const ConnectorTemplate = NewObject<UJointEdGraphNode_Connector>();

	FVector2D SpawnLocation = Location;

	UEdGraphNode* const NewNode = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UJointEdGraphNode_Connector>(
		ParentGraph, ConnectorTemplate, SpawnLocation, bSelectNewNode);

	return NewNode;
}

