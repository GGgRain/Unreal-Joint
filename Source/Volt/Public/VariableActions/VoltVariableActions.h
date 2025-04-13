//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltVariableActionBase.h"
#include "Layout/Margin.h"
#include "VoltVariableActions.generated.h"

/**
 * Handle and define the way to modify the native slate in unreal slate architecture.
 * See how each actions effect each slate type and how it gets the type of the slate.
 *
 * You can still define your own variable action for your own slate type.
 * Override a class from UVoltVariableActionBase and override CheckSupportWidget to make it support the slate type you want, and override ApplyVariable to make the variable applied to your slate.
 */

/**
 * A variable that affects the opacity of the slate.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVarAction_Opacity : public UVoltVariableActionBase 
{

	GENERATED_BODY()
	
public:

	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate) override;
	
	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply) override;
	
};

/**
 * A variable that affects the widget transform of the slate.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVarAction_WidgetTransform : public UVoltVariableActionBase 
{

	GENERATED_BODY()

public:

	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate) override;
	
	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply) override;
};


/**
 * A variable that affects the pivot of widget transform of the slate.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVarAction_WidgetTransformPivot : public UVoltVariableActionBase 
{

	GENERATED_BODY()
	
public:

	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate) override;
	
	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply) override;
};

/**
 * 
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVarAction_ColorAndOpacity : public UVoltVariableActionBase 
{

	GENERATED_BODY()
	
public:

	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate) override;
	
	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply) override;
	
};

/**
 *
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVarAction_BackgroundColor : public UVoltVariableActionBase 
{

	GENERATED_BODY()
	
public:

	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate) override;
	
	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply) override;
	
};

/**
 *
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVarAction_ForegroundColor : public UVoltVariableActionBase 
{

	GENERATED_BODY()
	
public:

	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate) override;
	
	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply) override;
	
};


/**
 * A variable action that affects the padding of the slate's child slots.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVarAction_ChildSlotPadding : public UVoltVariableActionBase 
{

	GENERATED_BODY()

public:

	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate) override;
	
	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply) override;
	
};

/**
 * A variable action that affects the padding of the slot that contains the animated slate.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVarAction_ParentSlotPadding : public UVoltVariableActionBase 
{

	GENERATED_BODY()

public:

	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate) override;
	
	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply) override;
	
};

/**
 * A variable action that handles SBox related properties and functionalities.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVarAction_Box : public UVoltVariableActionBase 
{

	GENERATED_BODY()
	
public:

	virtual bool CheckSupportWidget(TWeakPtr<SWidget> Slate) override;
	
	virtual void ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply) override;
	
};

