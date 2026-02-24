//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointScriptParserFactory.h"

#include "JointEdUtils.h"
#include "JointScriptParser.h"

UJointScriptParserFactory::UJointScriptParserFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UJointScriptParser::StaticClass();
}

UObject* UJointScriptParserFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// bp 
	//UJointFragment* Asset = NewObject<UJointFragment>(InParent, Class, Name, Flags | RF_Transactional);
	
	UObject* Asset = FJointEdUtils::CreateNewBlueprintAssetForClass(Class, FPaths::GetPath(InParent->GetPathName()));
	
	return Asset;
}

bool UJointScriptParserFactory::ShouldShowInNewMenu() const {
	return true;
}
