//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/SubNode/JointEdGraphNode_Fragment.h"

#include "GraphNode/SJointGraphNodeBase.h"

#include "Misc/EngineVersionComparison.h"
#include "Node/JointFragment.h"

#define LOCTEXT_NAMESPACE "JointGraphNode_Fragment"

UJointEdGraphNode_Fragment::UJointEdGraphNode_Fragment()
{
	NodeWidth = JointGraphNodeResizableDefs::MinFragmentSize.X;
	NodeHeight = JointGraphNodeResizableDefs::MinFragmentSize.Y;
	bIsNodeResizeable = false;
}

void UJointEdGraphNode_Fragment::ResizeNode(const FVector2D& NewSize)
{
	
	const FVector2D& Min = GetNodeMinimumSize();
	const FVector2D& Max = GetNodeMaximumSize();

	const FVector2D& NodeSize = FVector2D(
		FMath::Clamp<float>(NewSize.X, Min.X, Max.X),
		FMath::Clamp<float>(NewSize.Y, Min.Y, Max.Y)
	);
	
	NodeWidth = NodeSize.X;
	NodeHeight = NodeSize.Y;
	
}

TSubclassOf<UJointNodeBase> UJointEdGraphNode_Fragment::SupportedNodeClass()
{
	return UJointFragment::StaticClass();
}

void UJointEdGraphNode_Fragment::AllocateDefaultPins()
{
	//Doesn't have anything by default
}

bool UJointEdGraphNode_Fragment::ShouldManuallyImplementSlate() const
{
	return false;
}

void UJointEdGraphNode_Fragment::DissolveSelf()
{
	bIsDissolvedSubNode = true;
	ReconstructNode();
}

void UJointEdGraphNode_Fragment::SolidifySelf()
{
	bIsDissolvedSubNode = false;
	ReconstructNode();
}

#undef LOCTEXT_NAMESPACE
