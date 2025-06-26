//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Volt_ASM_SetWidgetTransformPivot.h"

#include "VoltInterface.h"
#include "VoltVariableCollection.h"
#include "Variables/VoltVariables.h"


void UVolt_ASM_SetWidgetTransformPivot::Construct(const FArguments& InArgs)
{
	TargetWidgetTransformPivot = InArgs._TargetWidgetTransformPivot;
}

void UVolt_ASM_SetWidgetTransformPivot::ModifySlateVariable(const float DeltaTime,
                                                            const TScriptInterface<IVoltInterface>& Volt)
{
	if(Volt == nullptr) return;
	if(Volt->GetVoltVariableCollection() == nullptr) return;

	UVoltVariableBase* Var = Volt->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_WidgetTransformPivot::StaticClass());

	UVoltVar_WidgetTransformPivot* CastedVar = Cast<UVoltVar_WidgetTransformPivot>(Var);
	
	CastedVar->Value = TargetWidgetTransformPivot;

	bEverUpdated = true;

}

bool UVolt_ASM_SetWidgetTransformPivot::IsActive()
{
	return !bEverUpdated;
}
