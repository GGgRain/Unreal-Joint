//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphNode.h"
#include "JointEdGraphNode_Foundation.generated.h"

/**
 * 
 */
UCLASS()
class JOINTEDITOR_API UJointEdGraphNode_Foundation : public UJointEdGraphNode
{
	GENERATED_BODY()

public:

	UJointEdGraphNode_Foundation();

public:
	
	UPROPERTY(EditAnywhere, Category = "Nodes")
	bool bMakeOutputPinAlways = false;

public:

	virtual TSubclassOf<UJointNodeBase> SupportedNodeClass() override;
	
	virtual void ReallocatePins() override;

	virtual void AllocateDefaultPins() override;

	virtual void NodeConnectionListChanged() override;
	
public:
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
};
