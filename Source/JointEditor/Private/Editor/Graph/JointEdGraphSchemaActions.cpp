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
#include "JointEditorFunctionLibrary.h"
#include "JointEdUtils.h"
#include "Node/JointNodeBase.h"
#include "ScopedTransaction.h"
#include "GraphNode/SJointGraphNodeBase.h"
#include "Markdown/SJointMDSlate_Admonitions.h"
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

UEdGraphNode* FJointSchemaAction_NewSubNode::PerformAction(
	class UEdGraph* ParentGraph,
	UEdGraphPin* FromPin,
	const FVector2D Location, 
	bool bSelectNewNode)
{
	if (!NodeTemplate || !ParentGraph) return nullptr;

	UClass* NodeClass = NodeTemplate->NodeClassData.GetClass();
	
	if (!NodeClass) return nullptr;

	GEditor->BeginTransaction(
		FText::Format(NSLOCTEXT("JointEdTransaction","TransactionTitle_AddNewSubNode","Add new sub node (Fragment): {0}"),
			FText::FromString(NodeClass->GetName())
		)
	);
	
	UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(ParentGraph);
	if (!CastedGraph) return nullptr;

	CastedGraph->Modify();
	NodeTemplate->Modify();

	UJointEdGraphNode* LastCreatedNode = nullptr;

	for (UObject* NodeObj : NodesToAttachTo)
	{
		UJointEdGraphNode* ParentJointEdGraphNode = Cast<UJointEdGraphNode>(NodeObj);

		if (!ParentJointEdGraphNode) continue;

		ParentJointEdGraphNode->Modify();

		if (UJointNodeBase* Inst = ParentJointEdGraphNode->GetCastedNodeInstance())
		{
			Inst->Modify();
		}

		LastCreatedNode = UJointEditorFunctionLibrary::AddFragment(
			CastedGraph->GetJointManager(),
			ParentJointEdGraphNode,
			NodeClass
		);
	}
	
	if (!LastCreatedNode)
	{
		FJointEdUtils::FireNotification(	
			NSLOCTEXT("JointEdSchemaAction_NewSubNode", "AddNewSubNode_Failed_Title", "Failed to add new sub node"),
			NSLOCTEXT("JointEdSchemaAction_NewSubNode", "AddNewSubNode_Failed_Message", "No valid parent nodes to attach the new sub node to."),
			EJointMDAdmonitionType::Error,
			10
		);
	}else
	{
		FJointEdUtils::FireNotification(	
			NSLOCTEXT("JointEdSchemaAction_NewSubNode", "AddNewSubNode_Success_Title", "New sub node added"),
			FText::Format(
			NSLOCTEXT("JointEdSchemaAction_NewSubNode", "AddNewSubNode_Success_Message", "A new sub node \'{0}\' has been successfully added to the parent nodes."),
				LastCreatedNode->GetNodeTitle(ENodeTitleType::FullTitle)
			),
			EJointMDAdmonitionType::Mention,
			7
		);
	}

	GEditor->EndTransaction();

	return LastCreatedNode;
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

UEdGraphNode* FJointSchemaAction_NewNode::PerformAction(
	UEdGraph* ParentGraph,
	UEdGraphPin* FromPin,
	const FVector2D Location,
	bool bSelectNewNode)
{
	if (!NodeTemplate || !ParentGraph) return nullptr;

	UClass* NodeClass = NodeTemplate->NodeClassData.GetClass();
	if (!NodeClass) return nullptr;
	UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(ParentGraph);
	if (!CastedGraph) return nullptr;
	UJointManager* Manager = CastedGraph->GetJointManager();
	if (!Manager) return nullptr;

	GEditor->BeginTransaction(
		FText::Format(
			NSLOCTEXT("JointEdTransaction",
			          "TransactionTitle_AddNewNode",
			          "Add new node: {0}"),
			FText::FromString(NodeClass->GetName())
		)
	);

	ParentGraph->Modify();
	Manager->Modify();

	UJointEdGraphNode* ResultNode = UJointEditorFunctionLibrary::AddBaseNode(
		Manager,
		ParentGraph,
		NodeClass,
		Location
	);

	if (ResultNode)
	{
		ResultNode->Modify();

		FJointEdUtils::MakeConnectionFromTheDraggedPin(FromPin, ResultNode);
	}

	GEditor->EndTransaction();

	return ResultNode;
}

UEdGraphNode* FJointSchemaAction_NewNode::PerformAction_FromShortcut(UEdGraph* ParentGraph, TSubclassOf<UJointEdGraphNode> EdClass, TSubclassOf<UJointNodeBase> NodeClass, const FVector2D Location,
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
	ResultNode->NodeClassData = FJointSharedClassData(NodeClass, FJointGraphNodeClassHelper::GetDeprecationMessage(NodeData->GetClass()));
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

FJointSchemaAction_NewNodePreset::FJointSchemaAction_NewNodePreset()
{
}

FJointSchemaAction_NewNodePreset::FJointSchemaAction_NewNodePreset(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
	: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
{
}

UEdGraphNode* FJointSchemaAction_NewNodePreset::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	return FEdGraphSchemaAction::PerformAction(ParentGraph, FromPin, Location, bSelectNewNode);
}

UEdGraphNode* FJointSchemaAction_NewNodePreset::PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode)
{
	return FEdGraphSchemaAction::PerformAction(ParentGraph, FromPins, Location, bSelectNewNode);
}

void FJointSchemaAction_NewNodePreset::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
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

