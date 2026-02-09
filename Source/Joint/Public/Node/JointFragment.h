//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Node/JointNodeBase.h"
#include "JointFragment.generated.h"

/**
 * Fragment is a sub-node class that will be attached to the Joint nodes and patch up various features for the node.
 * Each fragment can have its own pins, and the pins will be implemented on the outermost node that hold this fragment.
 * Fragments will have its priority on the node. Fragments with higher priority will be tested and played first.
 */

UCLASS(Abstract, Blueprintable, BlueprintType)
class JOINT_API UJointFragment : public UJointNodeBase
{
	GENERATED_BODY()

public:
	
	UJointFragment();

public:

	UFUNCTION(BlueprintPure, Category = "Fragment")
	static bool IsManagerFragment(UJointNodeBase* InFragment);
};
