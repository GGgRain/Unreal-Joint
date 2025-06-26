//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Debug/JointNodeDebugData.h"

bool FJointNodeDebugData::CheckWhetherNecessary() const
{
	if(Node != nullptr)
	{
		if(bHasBreakpoint == true)
		{
			return true;
		}
	}

	return false;
}
