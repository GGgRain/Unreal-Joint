//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Variables/VoltVariables.h"

#include "VariableActions/VoltVariableActions.h"


#define MACRO_REGISTER_VARIABLE_ACTION(VariableActionClassName) \
{ \
	if(!ActionsForVariables.Contains(VariableActionClassName::StaticClass())) ActionsForVariables.Add(VariableActionClassName::StaticClass()); \
}

UVoltVar_Opacity::UVoltVar_Opacity()
{
	MACRO_REGISTER_VARIABLE_ACTION(UVoltVarAction_Opacity)
}

UVoltVar_WidgetTransform::UVoltVar_WidgetTransform()
{
	MACRO_REGISTER_VARIABLE_ACTION(UVoltVarAction_WidgetTransform)
}

UVoltVar_WidgetTransformPivot::UVoltVar_WidgetTransformPivot()
{
	MACRO_REGISTER_VARIABLE_ACTION(UVoltVarAction_WidgetTransformPivot)
}

UVoltVar_ColorAndOpacity::UVoltVar_ColorAndOpacity()
{
	MACRO_REGISTER_VARIABLE_ACTION(UVoltVarAction_ColorAndOpacity)
}

UVoltVar_BackgroundColor::UVoltVar_BackgroundColor()
{
	MACRO_REGISTER_VARIABLE_ACTION(UVoltVarAction_BackgroundColor)
}

UVoltVar_ForegroundColor::UVoltVar_ForegroundColor()
{
	MACRO_REGISTER_VARIABLE_ACTION(UVoltVarAction_ForegroundColor)
}

UVoltVar_ChildSlotPadding::UVoltVar_ChildSlotPadding()
{
	MACRO_REGISTER_VARIABLE_ACTION(UVoltVarAction_ChildSlotPadding)
}

UVoltVar_ParentSlotPadding::UVoltVar_ParentSlotPadding()
{
	MACRO_REGISTER_VARIABLE_ACTION(UVoltVarAction_ParentSlotPadding)
}

UVoltVar_Box::UVoltVar_Box()
{
	MACRO_REGISTER_VARIABLE_ACTION(UVoltVarAction_Box)
}

#undef MACRO_REGISTER_VARIABLE_ACTION