//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "VoltSubModuleInterface.h"

#include "VoltModuleItem.h"

bool IVoltSubModuleInterface::K2_HasModule_Implementation(UVoltModuleItem* InModule)
{
	return HasModule(InModule);
}

bool IVoltSubModuleInterface::K2_HasModuleForClass_Implementation(TSubclassOf<UVoltModuleItem> InModuleClass)
{
	return HasModuleForClass(InModuleClass);
}

UVoltModuleItem* IVoltSubModuleInterface::K2_GetModuleAt_Implementation(const int& Index)
{
	return GetModuleAt(Index);
}

UVoltModuleItem* IVoltSubModuleInterface::K2_GetModuleForClass_Implementation(TSubclassOf<UVoltModuleItem> InModuleClass)
{
	return GetModuleForClass(InModuleClass);
}

void IVoltSubModuleInterface::K2_AddModule_Implementation(UVoltModuleItem* InModule)
{
	AddModule(InModule);
}

void IVoltSubModuleInterface::K2_InsertModuleAt_Implementation(UVoltModuleItem* InModule, const int& Index)
{
	InsertModuleAt(InModule,Index);
}

void IVoltSubModuleInterface::K2_RemoveModule_Implementation(UVoltModuleItem* InModule)
{
	RemoveModule(InModule);
}

void IVoltSubModuleInterface::K2_RemoveAllModuleForClass_Implementation(TSubclassOf<UVoltModuleItem> InModuleClass)
{
	RemoveAllModuleForClass(InModuleClass);
}

void IVoltSubModuleInterface::K2_RemoveModuleAt_Implementation(const int& Index)
{
	RemoveModuleAt(Index);
}

bool IVoltSubModuleInterface::HasModule(UVoltModuleItem* InModule)
{
	if(const TArray<TObjectPtr<UVoltModuleItem>>* Container = GetModuleContainer())
	{
		return Container->Contains(InModule);
	}
	
	return false;
}

bool IVoltSubModuleInterface::HasModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass)
{
	if(const TArray<TObjectPtr<UVoltModuleItem>>* Container = GetModuleContainer())
	{
		return Container->ContainsByPredicate([InModuleClass](const UVoltModuleItem* Item) { return Item->GetClass() == InModuleClass; });
	}
	
	return false;
}

UVoltModuleItem* IVoltSubModuleInterface::GetModuleAt(const int& Index)
{
	if(TArray<TObjectPtr<UVoltModuleItem>>* Container = GetModuleContainer())
	{
		return (Container->IsValidIndex(Index)) ? (*Container)[Index] : nullptr;
	}
	
	return nullptr;
}

UVoltModuleItem* IVoltSubModuleInterface::GetModuleForClass(const TSubclassOf<UVoltModuleItem> InModuleClass)
{
	if(TArray<TObjectPtr<UVoltModuleItem>>* Container = GetModuleContainer())
	{
		return *Container->FindByPredicate([InModuleClass](const UVoltModuleItem* Item) { return Item->GetClass() == InModuleClass; });
	}
	
	return nullptr;
}

void IVoltSubModuleInterface::AddModule(UVoltModuleItem* InModule)
{
	if(TArray<TObjectPtr<UVoltModuleItem>>* Container = GetModuleContainer())
	{
		if(!Container->Contains(InModule)) Container->Add(InModule);
	}
}

void IVoltSubModuleInterface::InsertModuleAt(UVoltModuleItem* InModule, const int& Index)
{
	if(TArray<TObjectPtr<UVoltModuleItem>>* Container = GetModuleContainer())
	{
		if(!Container->Contains(InModule)) Container->Insert(InModule,Index);
	}
}

void IVoltSubModuleInterface::RemoveModule(UVoltModuleItem* InModule)
{
	if(TArray<TObjectPtr<UVoltModuleItem>>* Container = GetModuleContainer())
	{
		Container->Remove(InModule);
	}
}

void IVoltSubModuleInterface::RemoveAllModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass)
{
	if(TArray<TObjectPtr<UVoltModuleItem>>* Container = GetModuleContainer())
	{
		Container->RemoveAll([InModuleClass](const UVoltModuleItem* Item) { return Item->GetClass() == InModuleClass; });
	}
}

void IVoltSubModuleInterface::RemoveModuleAt(const int& Index)
{
	if(TArray<TObjectPtr<UVoltModuleItem>>* Container = GetModuleContainer())
	{
		if(Container->IsValidIndex(Index)) Container->RemoveAt(Index);
	}
}

