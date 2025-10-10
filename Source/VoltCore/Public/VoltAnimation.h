//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltSubModuleInterface.h"
#include "Templates/SubclassOf.h"
#include "UObject/NoExportTypes.h"
#include "VoltAnimation.generated.h"

class UVoltAnimation;
class UVoltModuleItem;

/**
 * The animation class for the slates in Volt.
 */
UCLASS()
class VOLTCORE_API UVoltAnimation : public UObject, public IVoltSubModuleInterface
{
	GENERATED_BODY()

public:

	//Modules that this animation uses.
	UPROPERTY(Instanced, BlueprintReadWrite, Category="Animation")
		TArray<TObjectPtr<UVoltModuleItem>> Modules; 

	VOLT_DECLARE_SUBMODULE_FUNCTIONS(Modules)
	
public:

	UFUNCTION(BlueprintCallable, Category="Animation")
	const bool IsActive() const;
	
};
