
// Fill out your copyright notice in the Description page of Project Settings.


#include "Asset/JointNodePreset.h"

#include "JointEdGraph.h"
#include "JointEdGraphSchema.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "JointNodePreset"

UJointNodePreset::UJointNodePreset()
{
	// Create a new graph for the preset (Default Subobject)
	//PresetGraph = CreateDefaultSubobject<UJointEdGraph>(TEXT("PresetGraph"));
	//PresetGraph->Schema = UJointEdGraphSchema::StaticClass();
	
	InternalJointManager = CreateDefaultSubobject<UJointManager>(TEXT("InternalJointManager"));
	
}


#undef LOCTEXT_NAMESPACE