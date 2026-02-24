// Fill out your copyright notice in the Description page of Project Settings.

#include "Editor/Script/JointScriptSettings.h"

UJointScriptSettings::UJointScriptSettings()
{
}

UJointScriptSettings* UJointScriptSettings::Get()
{
	return GetMutableDefault<UJointScriptSettings>();
}

void UJointScriptSettings::Save()
{
	Get()->SaveConfig(CPF_Config, *Get()->GetDefaultConfigFilename());
}
