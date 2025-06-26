//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "VoltAnimation.h"
#include "VoltModuleItem.h"

const bool UVoltAnimation::IsActive() const
{
	if (Modules.IsEmpty()) return false; // Inactive since there is no module to play.

	for (UVoltModuleItem* Module : Modules)
	{
		if (!Module) continue; //Skip the check if not valid.

		if (Module->IsBegunPlay() && Module->IsEndedPlay()) continue;

		if (Module->IsActive()) return true;
	}

	return false;
}

