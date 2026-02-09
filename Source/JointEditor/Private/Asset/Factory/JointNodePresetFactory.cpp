//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointNodePresetFactory.h"
#include "JointNodePreset.h"

UJointNodePresetFactory::UJointNodePresetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UJointNodePreset::StaticClass();
}

UObject* UJointNodePresetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UJointNodePreset* Asset = NewObject<UJointNodePreset>(InParent, Class, Name, Flags | RF_Transactional);
	
	return Asset;
}

bool UJointNodePresetFactory::ShouldShowInNewMenu() const {
	return true;
}
