//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/JointFragment.h"

UJointFragment::UJointFragment() 
	: UJointNodeBase()
{
	//Fragments are not resizeable, as they will automatically adjust their size to fit their content.
#if WITH_EDITORONLY_DATA
	EdNodeSetting.bDefaultIsNodeResizeable = false;
#endif
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

#if WITH_EDITOR
void UJointFragment::PostPlacedNewNode_Implementation()
{
	EdNodeSetting.bDefaultIsNodeResizeable = false;
}
#endif