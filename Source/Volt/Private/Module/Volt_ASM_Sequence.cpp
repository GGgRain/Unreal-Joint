//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Volt_ASM_Sequence.h"
#include "VoltInterface.h"

void UVolt_ASM_Sequence::PickupNextModule()
{
	++CurrentlyPlayingModuleIdx;

	if (!bShouldLoop)
	{
		return;
	}

	const int32 NumModules = Modules.Num();
	const bool bHasMaxLoop = (MaxLoopCount > 0);

	// Loop boundary check
	if (CurrentlyPlayingModuleIdx >= NumModules)
	{
		if (bHasMaxLoop)
		{
			++CurrentLoopedCount;

			if (CurrentLoopedCount >= MaxLoopCount)
			{
				// Invalid index to stop playing
				CurrentlyPlayingModuleIdx = NumModules;
				return;
			}
		}

		// Wrap index (cheaper than %)
		CurrentlyPlayingModuleIdx -= NumModules;
	}
}

void UVolt_ASM_Sequence::Construct(const FArguments& InArgs)
{
	bShouldLoop = InArgs._bShouldLoop;
	MaxLoopCount = InArgs._MaxLoopCount;
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
