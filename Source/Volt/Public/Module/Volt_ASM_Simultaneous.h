//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltDecl.h"
#include "VoltModuleItem.h"
#include "VoltSubModuleInterface.h"
#include "Layout/Margin.h"
#include "Volt_ASM_Simultaneous.generated.h"

/**
 * A module for the simultaneous animation playback.
 */
UCLASS(EditInlineNew, Blueprintable, BlueprintType)
class VOLT_API UVolt_ASM_Simultaneous : public UVoltModuleItem, public IVoltSubModuleInterface
{
	
	GENERATED_BODY()

public:

	VOLT_MODULE_BEGIN_ARGS(UVolt_ASM_Simultaneous) :
		_bShouldEndWhenAnyEnd(false)
	{}
	VOLT_MODULE_ARGUMENT( bool, bShouldEndWhenAnyEnd )
	VOLT_SUBMODULE_CONTAINER_ARGUMENT(SubModules) //It let you use () operator on the declaration to grab other modules and pass them into the InArgs in Construct().
	VOLT_MODULE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

public:
	
	UPROPERTY(Instanced, BlueprintReadWrite, Category="Animation")
		TArray<TObjectPtr<UVoltModuleItem>> Modules; 
	
	VOLT_DECLARE_SUBMODULE_FUNCTIONS(Modules)
	
public:
	
	virtual void ModifySlateVariable(const float DeltaTime, const TScriptInterface<IVoltInterface>& Volt) override;

public:

	virtual void OnModuleBeginPlay_Implementation() override;
	
	virtual void OnModuleEndPlay_Implementation() override;
	
	virtual bool IsActive() override;

public:

	/**
	 * Whether to loop the playback.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bShouldEndWhenAnyEnd = false;
	
};


