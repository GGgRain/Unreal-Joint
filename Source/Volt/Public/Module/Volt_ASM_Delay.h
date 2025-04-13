//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltDecl.h"
#include "VoltModuleItem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Layout/Margin.h"
#include "Volt_ASM_Delay.generated.h"

UCLASS(EditInlineNew, Blueprintable, BlueprintType)
class VOLT_API UVolt_ASM_Delay : public UVoltModuleItem
{
	
	GENERATED_BODY()

public:

	VOLT_MODULE_BEGIN_ARGS(UVolt_ASM_Delay) :
		_Duration(3)
	{}
		VOLT_MODULE_ARGUMENT( double, Duration )
	
	VOLT_MODULE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

public:

	virtual void ModifySlateVariable(const float DeltaTime, const TScriptInterface<IVoltInterface>& Volt) override;

public:

	virtual void OnModuleBeginPlay_Implementation() override;
	
	virtual bool IsActive() override;

public:

	/**
	 * Duration of the delay.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Delay")
	double Duration = -1;
	
private:
	
	/**
	 * For the duration calculation.
	 */
	UPROPERTY(Transient)
	double AccumulatedTime = 0;
	
};