//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointManagerFactory.h"

#include "JointManager.h"

UJointManagerFactory::UJointManagerFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UJointManager::StaticClass();
}

UObject* UJointManagerFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UJointManager* Asset = NewObject<UJointManager>(InParent, Class, Name, Flags | RF_Transactional);
	
	return Asset;
}

bool UJointManagerFactory::ShouldShowInNewMenu() const {
	return true;
}
