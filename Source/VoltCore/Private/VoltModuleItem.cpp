//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "VoltModuleItem.h"

#include "VoltSubModuleInterface.h"

void UVoltModuleItem::ModifySlateVariable(const float DeltaTime,
                                          const TScriptInterface<IVoltInterface>& Volt)
{
	K2_ModifySlateVariable(DeltaTime, Volt);
}

const TScriptInterface<IVoltInterface>& UVoltModuleItem::GetVoltSlate()
{
	return TargetVoltSlate;
}

void UVoltModuleItem::SetSubModulesVoltSlate(const TScriptInterface<IVoltInterface>& Slate)
{
	//Volt 1.1 : If this module supports sub-modules, set them as well.
	
	if(IVoltSubModuleInterface* TheInterface = Cast<IVoltSubModuleInterface>(this))
	{
		TArray<TObjectPtr<UVoltModuleItem>>* Container = TheInterface->GetModuleContainer();

		if(!Container) return;

		for (UVoltModuleItem* VoltModuleItem : *Container)
		{
			if(!VoltModuleItem) continue;
			
			VoltModuleItem->SetVoltSlate(Slate);
		}
	}
}

void UVoltModuleItem::ReloadModule()
{
	bIsModuleBegunPlay = false;

	//Endplay first if it's still active.
	if(!IsEndedPlay()) EndPlayModule();

	bIsModuleEndedPlay = false;
	
	//Propagate it to the children modules
	if(IVoltSubModuleInterface* TheInterface = Cast<IVoltSubModuleInterface>(this))
	{
		TArray<TObjectPtr<UVoltModuleItem>>* Container = TheInterface->GetModuleContainer();
		
		if(!Container) return;

		for (UVoltModuleItem* VoltModuleItem : *Container)
		{
			if(!VoltModuleItem) continue;
			
			VoltModuleItem->ReloadModule();
		}
	}
}

void UVoltModuleItem::BeginPlayModule()
{
	if(IsBegunPlay()) return;
	
	bIsModuleBegunPlay = true;
	
	OnModuleBeginPlay();
}

void UVoltModuleItem::EndPlayModule()
{
	if(!IsBegunPlay() || IsEndedPlay()) return;
	
	bIsModuleEndedPlay = true;

	OnModuleEndPlay();
}

void UVoltModuleItem::SetVoltSlate(const TScriptInterface<IVoltInterface>& Slate)
{
	TargetVoltSlate = Slate;

	//Volt 1.1 : If this module supports sub-modules, set them as well.

	SetSubModulesVoltSlate(Slate);
}

void UVoltModuleItem::OnModuleBeginPlay_Implementation()
{
}

void UVoltModuleItem::OnModuleEndPlay_Implementation()
{
}

bool UVoltModuleItem::IsBegunPlay() const
{
	return bIsModuleBegunPlay;
}

bool UVoltModuleItem::IsEndedPlay() const
{
	return bIsModuleEndedPlay;
}

bool UVoltModuleItem::IsActive()
{
	return K2_IsActive();
}
