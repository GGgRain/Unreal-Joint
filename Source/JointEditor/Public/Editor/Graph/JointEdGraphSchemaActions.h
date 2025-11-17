//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "JointEdGraphSchemaActions.generated.h"


class UJointNodeBase;
class UJointEdGraphNode;

USTRUCT()
struct JOINTEDITOR_API FJointSchemaAction_NewSubNode : public FEdGraphSchemaAction
{

public:
	
	GENERATED_BODY();

public:

	FJointSchemaAction_NewSubNode();
    
	FJointSchemaAction_NewSubNode(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping);
	
public:
	
	//Template of node we want to create.
	UPROPERTY()
	TObjectPtr<UJointEdGraphNode> NodeTemplate;

	//parent node that we will attach this node at, must be casted to UJointEdGraphNode first.
	UPROPERTY()
	TArray<TObjectPtr<UObject>> NodesToAttachTo;
	
public:
	
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode = true) override;
	
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
};


USTRUCT()
struct JOINTEDITOR_API FJointSchemaAction_NewNode : public FEdGraphSchemaAction
{

public:

	GENERATED_BODY();

public:
	
	FJointSchemaAction_NewNode();

	FJointSchemaAction_NewNode(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping);
	
public:
	
	//Template of node we want to create.
	UPROPERTY()
	TObjectPtr<UJointEdGraphNode> NodeTemplate;

public:
	static void MakeConnectionFromTheDraggedPin(UEdGraphPin* FromPin, UJointEdGraphNode* ConnectedNode);
	
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode = true) override;
	
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

public:

	UEdGraphNode* PerformAction_Command(class UEdGraph* ParentGraph, TSubclassOf<UJointEdGraphNode> EdClass, TSubclassOf<UJointNodeBase> NodeClass, const FVector2D Location, bool bSelectNewNode = true);

	template<typename T=UJointEdGraphNode>
	static T* SpawnNode(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true)
	{
		T* const ConnectorTemplate = NewObject<T>();
		
		T* const NewNode = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<T>(
			ParentGraph, ConnectorTemplate, Location, bSelectNewNode);

		return NewNode;
	}
	
};



/** Action to add a comment to the graph */

USTRUCT()
struct JOINTEDITOR_API FJointSchemaAction_AddComment : public FEdGraphSchemaAction
{
	GENERATED_BODY()
	
	FJointSchemaAction_AddComment() : FEdGraphSchemaAction() {}
	FJointSchemaAction_AddComment(FText Category, FText InDescription, FText InToolTip)
		: FEdGraphSchemaAction(MoveTemp(Category), MoveTemp(InDescription), MoveTemp(InToolTip), 0)
	{
	}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override final;
	// End of FEdGraphSchemaAction interface
};


/** Action to add a pair of connector to the graph */

USTRUCT()
struct JOINTEDITOR_API FJointSchemaAction_AddConnector : public FEdGraphSchemaAction
{
	GENERATED_BODY()
	
	FJointSchemaAction_AddConnector() : FEdGraphSchemaAction() {}
	FJointSchemaAction_AddConnector(FText Category, FText InDescription, FText InToolTip)
		: FEdGraphSchemaAction(MoveTemp(Category), MoveTemp(InDescription), MoveTemp(InToolTip), 0)
	{
	}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override final;
	// End of FEdGraphSchemaAction interface
};

/** Action to add a pair of connector to the graph */
