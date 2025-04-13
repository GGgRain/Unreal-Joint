//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "JointBuildTargetPresetFactory.generated.h"


UCLASS()
class JOINTEDITOR_API UJointBuildTargetPresetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:
	
	//Create new JointBuildTargetPreset asset. 
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	//Returns true if this factory should be shown in the New Asset menu (by default calls CanCreateNew).
	virtual bool ShouldShowInNewMenu() const override;
	

};
