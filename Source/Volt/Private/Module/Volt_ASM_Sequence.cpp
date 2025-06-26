//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Volt_ASM_Sequence.h"
#include "VoltInterface.h"

void UVolt_ASM_Sequence::PickupNextModule()
{
	CurrentlyPlayingModuleIdx++;

	if(bShouldLoop)
	{
		CurrentlyPlayingModuleIdx = CurrentlyPlayingModuleIdx % Modules.Num();
	}
}

void UVolt_ASM_Sequence::Construct(const FArguments& InArgs)
{
	bShouldLoop = InArgs._bShouldLoop;
	Modules = InArgs._SubModules;
}

void UVolt_ASM_Sequence::ModifySlateVariable(const float DeltaTime,
                                             const TScriptInterface<IVoltInterface>& Volt)
{
	if(Modules.IsValidIndex(CurrentlyPlayingModuleIdx))
	{
		UVoltModuleItem* Module = Modules[CurrentlyPlayingModuleIdx];

		//Reload if we have a module that has been fully played.
		if (Module->IsBegunPlay() && Module->IsEndedPlay())
		{
			Module->ReloadModule();
			Module->BeginPlayModule();
		}

		//if the module is not began played, Play it.
		if(!Module->IsBegunPlay())
		{
			Module->BeginPlayModule();
		}
		
		if (!Module->IsActive())
		{
			Module->EndPlayModule();
			PickupNextModule();
		}

		Module->ModifySlateVariable(DeltaTime, Volt);
	}
}

void UVolt_ASM_Sequence::OnModuleBeginPlay_Implementation()
{
	for (UVoltModuleItem* Module : Modules)
	{
		if(!Module) continue;

		Module->SetVoltSlate(GetVoltSlate());
	}
}

void UVolt_ASM_Sequence::OnModuleEndPlay_Implementation()
{
	Super::OnModuleEndPlay_Implementation();
}

bool UVolt_ASM_Sequence::IsActive()
{
	return Modules.Num() > CurrentlyPlayingModuleIdx;
}
