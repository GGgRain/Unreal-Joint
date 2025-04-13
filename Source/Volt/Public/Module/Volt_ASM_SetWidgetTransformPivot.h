//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltDecl.h"
#include "VoltModuleItem.h"

#include "UObject/NoExportTypes.h"
#include "Volt_ASM_SetWidgetTransformPivot.generated.h"

UCLASS(EditInlineNew, Blueprintable, BlueprintType)
class VOLT_API UVolt_ASM_SetWidgetTransformPivot : public UVoltModuleItem
{
	
	GENERATED_BODY()

	VOLT_MODULE_BEGIN_ARGS(UVolt_ASM_SetWidgetTransformPivot) {}
	VOLT_MODULE_ARGUMENT( FVector2D, TargetWidgetTransformPivot )
	VOLT_MODULE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

public:

	virtual void ModifySlateVariable(const float DeltaTime, const TScriptInterface<IVoltInterface>& Volt) override;

	virtual bool IsActive() override;
	
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	FVector2D TargetWidgetTransformPivot;

private:
	
	UPROPERTY(Transient)
	bool bEverUpdated = false; //If not specified, do nothing.
	
};