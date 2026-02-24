
// Fill out your copyright notice in the Description page of Project Settings.


#include "Asset/JointNodePreset.h"

#include "JointEdGraph.h"
#include "JointEdGraphSchema.h"
#include "JointEditorStyle.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "JointNodePreset"

UJointNodePreset::UJointNodePreset()
{
	// Create a new graph for the preset (Default Subobject)
	//PresetGraph = CreateDefaultSubobject<UJointEdGraph>(TEXT("PresetGraph"));
	//PresetGraph->Schema = UJointEdGraphSchema::StaticClass();
	
	InternalJointManager = CreateDefaultSubobject<UJointManager>(TEXT("InternalJointManager"));
	
	PresetCategory = LOCTEXT("DefaultPresetCategory", "Joint Node Presets|Default");
	PresetDisplayName = LOCTEXT("DefaultPresetDisplayName", "Default Preset Name");
	PresetDescription = LOCTEXT("DefaultPresetDescription", "This is the default preset for Joint nodes.");
	PresetColor = FLinearColor(0.1,0.25,0.1);
	bUseCustomIcon = false;
	
}


#undef LOCTEXT_NAMESPACE