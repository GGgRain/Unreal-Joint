//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "VoltVariableActionBase.h"
#include "VoltVariableBase.generated.h"

/*
 * A struct that actually communicate with the widget's variables.
 * 
 * Override this class to make a new variable for the slates,
 * And implement a new UVoltVariableActionBase that handle how each specific slate type must be updated via this variable,
 * And assign the UVoltVariableActionBase to this class on Volt Settings.
 */

class UVoltAnimationManager;
class SWidget;

UCLASS()
class VOLTCORE_API UVoltVariableBase : public UObject
{
	
	GENERATED_BODY()
	
public:
	
	UVoltVariableBase() {}
	virtual ~UVoltVariableBase() {}

public:

	/**
	 * Return true if cached the actions.
	 * It will also return even when the cache action fails for some reason to prevent cache action being executed multiple times.
	 */
	UFUNCTION(BlueprintCallable, Category="Variable Action")
	bool CheckCachedActions();
	
	/**
	 * Caches the actions this variable will execute when this variable need to be applied.
	 */
	UFUNCTION(BlueprintCallable, Category="Variable Action")
	void CacheActions();

	/**
	 * Clear the caches for the actions stored on this variable.
	 */
	UFUNCTION(BlueprintCallable, Category="Variable Action")
	void ClearCachedActions();

public:
	
	/**
	 * Iterate through the action and execute the action if possible.
	 */
	void ApplyVariable(const TWeakPtr<SWidget>& SlateToApply);

public:

	/**
	 * Class of the variable actions that this variable will use.
	 */
	UPROPERTY(EditAnywhere, Category="Variable Action")
	TArray<TSubclassOf<UVoltVariableActionBase>> ActionsForVariables;

public:


	/**
	 * Cached actions for this variable.
	 * We still need to implement a solid object because some of the action might can use some variables.
	 * Though it's not that harsh on the performance. It's fine.
	 */
	UPROPERTY(EditAnywhere, Category="Variable Action")
	TArray<TObjectPtr<UVoltVariableActionBase>> CachedActions;

	/**
	 * Whether we cached the actions.
	 */
	UPROPERTY(VisibleAnywhere, Category="Variable Action")
	bool bCachedActions;
};
