//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Volt_ASM_Simultaneous.h"
#include "VoltInterface.h"


void UVolt_ASM_Simultaneous::Construct(const FArguments& InArgs)
{
	bShouldEndWhenAnyEnd = InArgs._bShouldEndWhenAnyEnd;
	Modules = InArgs._SubModules;
}

void UVolt_ASM_Simultaneous::ModifySlateVariable(const float DeltaTime,
                                             const TScriptInterface<IVoltInterface>& Volt)
{
	const int NumModule = Modules.Num();

	for(int i = 0; i < NumModule; ++i)
	{
		if(!Modules.IsValidIndex(i)) continue;
		
		UVoltModuleItem* Item = Modules[i];

		if(Item == nullptr || !Item->IsValidLowLevel()) continue;
		
		//if the module is not began played, Play it.
		if (!Item->IsBegunPlay())
		{
			Item->BeginPlayModule();
		}
			
		if(Item->IsEndedPlay()) continue;

		if (!Item->IsActive())
		{
			Item->EndPlayModule();
			continue;
		}

		Item->ModifySlateVariable(DeltaTime, Volt);
	}

}

void UVolt_ASM_Simultaneous::OnModuleBeginPlay_Implementation()
{
	for (UVoltModuleItem* Module : Modules)
	{
		if(!Module) continue;

		Module->SetVoltSlate(GetVoltSlate());
	}
}

void UVolt_ASM_Simultaneous::OnModuleEndPlay_Implementation()
{
	Super::OnModuleEndPlay_Implementation();
}

bool UVolt_ASM_Simultaneous::IsActive()
{
	const int NumModule = Modules.Num();

	int EndCount = 0;

	for(int i = 0; i < NumModule; ++i)
	{
		if(!Modules[i])
		{
			EndCount++;
			continue;
		}

		if(Modules[i]->IsEndedPlay())
		{
			EndCount++;
		}
	}

	return bShouldEndWhenAnyEnd ? false : EndCount < NumModule;
}
