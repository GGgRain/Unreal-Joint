// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JointManager.h"
#include "JointScriptLinker.h"
#include "JointScriptParser.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/NoExportTypes.h"
#include "JointScriptSettings.generated.h"

class UJointNodeBase;

/**
 * Joint Script Linker is an 
*/
UCLASS(config = JointScriptSettings, defaultconfig)
class JOINTEDITOR_API UJointScriptSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	
	UJointScriptSettings();

public:

	/**
	 * Saved Script Links for the project.
	 * It holds the mapping between the file entries and the node mappings for the Joint nodes in different Joint managers.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Joint Script", meta = (DisplayName = "Script Link Data"))
	FJointScriptLinkerData ScriptLinkData;

public:
	//Instance Obtain Related

	static UJointScriptSettings* Get();

	static void Save();
	
public:
	
	virtual FName GetCategoryName() const override final { return TEXT("Joint"); }
	virtual FText GetSectionText() const override final { return FText::FromString("Joint Script Settings"); }
	
};
