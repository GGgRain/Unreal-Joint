//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/JointFragment.h"

UJointFragment::UJointFragment()
{
}

bool UJointFragment::IsManagerFragment(UJointNodeBase* InFragment)
{
	if (!InFragment) return false;

	if (UJointNodeBase* ParentmostNode = InFragment->GetParentmostNode())
	{
		//if the ParentmostNode is a fragment, it's a manager fragment.
		
		return Cast<UJointFragment>(ParentmostNode) != nullptr;
	}

	//No parentmost node found - this cannot happen, so return false.
	return false;
}
