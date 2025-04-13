//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Node/JointEdGraphNode.h"
#include "JointEdGraphNode_Connector.generated.h"

class UJointManager;
/**
 * 
 */
UCLASS()
class JOINTEDITOR_API UJointEdGraphNode_Connector : public UJointEdGraphNode
{
	GENERATED_BODY()

public:

	UJointEdGraphNode_Connector();


public:

	//UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	
	virtual FLinearColor GetNodeTitleColor() const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

public:
	
	virtual FVector2D GetNodeMinimumSize() const override;

public:

	virtual void AllocateDefaultPins() override;

	virtual void ReallocatePins() override;

public:

	virtual bool CanDuplicateNode() const override;
	
	virtual void ReconstructNode() override;

	virtual void PostPlacedNewNode() override;

	virtual bool CanHaveSubNode() const override;


public:
	
	void NotifyConnectionChangedToConnectedNodes();
	
	virtual void NodeConnectionListChanged() override;

public:

	virtual void AllocateReferringNodeInstancesOnConnection(TArray<UJointNodeBase*>& Nodes) override;
	
	virtual void UpdateNodeInstance() override;
	
	virtual void UpdateNodeInstanceOuter() const override;

	virtual void DestroyNode() override;

	virtual void PostPasteNode() override;

	virtual void ModifyGraphNodeSlate() override;

	virtual void OnCompileNode() override;

public:

	virtual bool CanHaveBreakpoint() const override;

public:
	
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

public:

	void OnAddInputNodeButtonPressed();

	//Direction of the connector node.
	UPROPERTY()
	TEnumAsByte<EEdGraphPinDirection> Direction;

	/**
	 * The guid of the connector node.
	 * The connector nodes that share the same guid will be connected by the system.
	 */
	UPROPERTY(EditAnywhere, Category="Connector Info")
	FGuid ConnectorGuid;

	/**
	 * Name of the connector link.
	 * This name is cosmetic.
	 */
	UPROPERTY(EditAnywhere, Category="Connector Info")
	FText ConnectorName;

	/**
	 * Nodes that are connected to this node.
	 */
	UPROPERTY(EditAnywhere, Category="Connector Info")
	TArray<UJointNodeBase*> ConnectedNodes;

private:
	
	UPROPERTY()
	TSoftObjectPtr<UJointEdGraphNode_Connector> CachedPairOutputConnector;

public:

	UJointEdGraphNode_Connector* GetCachedPairOutputConnector();

	void CachesPairOutputConnector();

	TArray<UJointEdGraphNode_Connector*> GetPairInputConnector();
	
	
};
