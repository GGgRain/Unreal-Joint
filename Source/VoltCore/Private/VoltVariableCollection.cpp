//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "VoltVariableCollection.h"
#include "VoltVariableBase.h"

UVoltVariableBase* UVoltVariableCollection::FindOrAddVariable(TSubclassOf<UVoltVariableBase> Type)
{
	if(!Type->IsValidLowLevel()) return nullptr;

	if(UVoltVariableBase* FoundVariable = FindVariable(Type)) return FoundVariable;

	//Add a new one.

	UVoltVariableBase* Base = NewObject<UVoltVariableBase>(this,Type);

	EnqueueVariableOnQueue(Base);
	
	return Base;
}

UVoltVariableBase* UVoltVariableCollection::FindVariable(TSubclassOf<UVoltVariableBase> Type)
{
	if(!Type || !Type->IsValidLowLevel()) return nullptr;

	const int QueuedVariablesNum = QueuedVariables.Num();
	for (int i = QueuedVariablesNum - 1; i >= 0; --i)
	{
		UVoltVariableBase* Variable = QueuedVariables[i];
		
		if(Variable == nullptr) continue;
		if(Variable->GetClass() != Type) continue;

		return Variable;
	} 

	const int VariablesNum = Variables.Num();
	for (int i = VariablesNum - 1; i >= 0; --i)
	{
		UVoltVariableBase* Variable = Variables[i];
		
		if(Variable == nullptr) continue;
		if(Variable->GetClass() != Type) continue;

		return Variable;
	} 
	
	return nullptr;
}

const TArray<UVoltVariableBase*>& UVoltVariableCollection::GetVariables()
{
	return Variables;
}

UVoltVariableBase* UVoltVariableCollection::EnqueueVariableOnQueue(UVoltVariableBase* Variable)
{
	QueuedVariables.Add(Variable);
	return Variable;
}

void UVoltVariableCollection::ProcessQueue()
{
	for (UVoltVariableBase* QueuedVariable : QueuedVariables)
	{
		if(QueuedVariable == nullptr) continue;
		Variables.Add(QueuedVariable);
	}

	QueuedVariables.Empty();
}

