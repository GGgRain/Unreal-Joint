//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointEditorSettings.h"

#include "HAL/PlatformFileManager.h"

UJointEditorSettings::UJointEditorSettings(): bUseGrid(false), SmallestGridSize(0)
{
}

void UJointEditorSettings::AddCoreRedirect(const FJointCoreRedirect& Redirect)
{
	JointCoreRedirects.AddUnique(Redirect);
	Save();
}

void UJointEditorSettings::RemoveCoreRedirect(const FJointCoreRedirect& Redirect)
{
	JointCoreRedirects.Remove(Redirect);
	Save();
}

const float UJointEditorSettings::GetJointGridSnapSize()
{
	const float& Size = Get()->GridSnapSize;
	
	const float MinSize = 1;
	
	return Size > MinSize ? Size : MinSize;
}

UJointEditorSettings* UJointEditorSettings::Get()
{
	return GetMutableDefault<UJointEditorSettings>();
}

void UJointEditorSettings::Save()
{
	Get()->SaveConfig(CPF_Config, *Get()->GetDefaultConfigFilename());
}
