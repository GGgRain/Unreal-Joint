// Fill out your copyright notice in the Description page of Project Settings.


#include "VoltSettings.h"
#include "HAL/PlatformFileManager.h"

UVoltSettings::UVoltSettings()
{
	
}


UVoltSettings* UVoltSettings::Get()
{
	return GetMutableDefault<UVoltSettings>();
}
