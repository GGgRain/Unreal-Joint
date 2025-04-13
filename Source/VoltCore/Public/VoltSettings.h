// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "VoltVariableBase.h"
#include "VoltVariableActionBase.h"
#include "Templates/SubclassOf.h"

#include "VoltSettings.generated.h"


/**
 * An array wrapper for the UVoltVariableActionBase sub classes.
 */

USTRUCT(BlueprintType)
struct FVoltVariableActionList
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Variable Actions")
	TArray<TSubclassOf<UVoltVariableActionBase>> Actions;
	
};

/**
 * The Developer Settings class that holds the fundamental data for Volt.
 */

UCLASS(config = VoltSetting, Defaultconfig, meta = (DisplayName = "Volt Settings"))
class VOLTCORE_API UVoltSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UVoltSettings();

public:
	
	/**
	 * Whether to utilize multithreading on the animation module update process.
	 * This will let your application doesn't be bottle-necked by Volt, and with slight advantage on the performance.
	 */
	UPROPERTY(config, EditAnywhere, Category="Performance", DisplayName="Use Multithreading On Module Update")
	bool bUseMultithreadingOnModuleUpdate = true;

	/**
	 * Interval for the Volt Subsystem clean-up (GC) code.
	 */
	UPROPERTY(config, EditAnywhere, Category="Performance", DisplayName="Volt Subsystem Clean Up Interval")
	double VoltSubsystemCleanUpInterval = 37;

	
public:
	
	/**
	 * Get the singleton instance of the class.
	 * @return The singleton instance of UVoltSettings.
	 */
	static UVoltSettings* Get();

public:

	virtual FName GetCategoryName() const override final { return TEXT("Volt"); }

#if WITH_EDITOR
	
	virtual FText GetSectionText() const override final { return FText::FromString("Volt Preferences"); }

#endif
	
};
