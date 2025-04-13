//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "VoltVariableActionBase.h"

bool UVoltVariableActionBase::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	return false;
}

void UVoltVariableActionBase::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
	//Does nothing on base class.
}
