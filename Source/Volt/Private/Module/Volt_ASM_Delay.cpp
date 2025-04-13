//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Volt_ASM_Delay.h"
#include "VoltInterface.h"


void UVolt_ASM_Delay::Construct(const FArguments& InArgs)
{
	Duration = InArgs._Duration;
}


void UVolt_ASM_Delay::ModifySlateVariable(const float DeltaTime,
                                                const TScriptInterface<IVoltInterface>& Volt)
{
	AccumulatedTime += DeltaTime;
}

void UVolt_ASM_Delay::OnModuleBeginPlay_Implementation()
{
	AccumulatedTime = 0;
}

bool UVolt_ASM_Delay::IsActive()
{
	return (Duration > 0) ? AccumulatedTime < Duration : false;
}
