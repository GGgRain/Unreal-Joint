//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Node/JointNodeBase.h"
#include "JN_Foundation.generated.h"

/**
 * A native Joint node type that can be directly placed on the graph.
 * Place this node on the graph and attach sub nodes to implement features on your Joint manager.
 */
UCLASS(Category = "Base Node")
class JOINT_API UJN_Foundation : public UJointNodeBase
{
	GENERATED_BODY()

public:

	UJN_Foundation();
	
public:

	UPROPERTY(AdvancedDisplay, BlueprintReadWrite, VisibleAnywhere, Category = "Nodes")
	TArray<TObjectPtr<UJointNodeBase>> NextNode;
	
public:

	virtual TArray<UJointNodeBase*> SelectNextNodes_Implementation(AJointActor* InHostingJointInstance) override;

public:

	virtual bool IsSupportedForNetworking() const override;
	
};
