//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltVariableActionBase.generated.h"

/*
 * A class that defines the actual action for the variable for each slate types.
 * Override this class and define the action for your slate of the variable that this class will be used.
 */

class UVoltVariableBase;
class SWidget;

UCLASS()
class VOLTCORE_API UVoltVariableActionBase : public UObject
{
	
	GENERATED_BODY()
	
public:
	
	UVoltVariableActionBase() {}
	virtual ~UVoltVariableActionBase() {}

public:
	
	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate);

public:

	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply);
	
};
