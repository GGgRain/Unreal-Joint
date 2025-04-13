//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointBuildTargetPresetFactory.h"

#include "SharedType/JointBuildPreset.h"

UJointBuildTargetPresetFactory::UJointBuildTargetPresetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UJointBuildPreset::StaticClass();
}

UObject* UJointBuildTargetPresetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UJointBuildPreset* Asset = NewObject<UJointBuildPreset>(InParent, Class, Name, Flags | RF_Transactional);
	
	return Asset;
}

bool UJointBuildTargetPresetFactory::ShouldShowInNewMenu() const {
	return true;
}
