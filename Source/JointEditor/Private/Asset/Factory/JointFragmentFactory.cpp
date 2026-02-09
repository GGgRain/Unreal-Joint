//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointFragmentFactory.h"

#include "JointEdUtils.h"
#include "Node/JointFragment.h"

UJointFragmentFactory::UJointFragmentFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UJointFragment::StaticClass();
}

UObject* UJointFragmentFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// bp 
	//UJointFragment* Asset = NewObject<UJointFragment>(InParent, Class, Name, Flags | RF_Transactional);
	
	UObject* Asset = FJointEdUtils::CreateNewBlueprintAssetForClass(Class, FPaths::GetPath(InParent->GetPathName()));
	
	return Asset;
}

bool UJointFragmentFactory::ShouldShowInNewMenu() const {
	return true;
}
