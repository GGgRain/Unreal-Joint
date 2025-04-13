//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointEditorSettings.h"

#include "HAL/PlatformFileManager.h"

UJointEditorSettings* UJointEditorSettings::Instance = nullptr;

UJointEditorSettings::UJointEditorSettings(): bUseGrid(false), SmallestGridSize(0)
{
}

const float UJointEditorSettings::GetJointGridSnapSize()
{
	const float& Size = Get()->GridSnapSize;
	
	const float MinSize = 1;
	
	return Size > MinSize ? Size : MinSize;
}

UJointEditorSettings* UJointEditorSettings::Get()
{
	if(!Instance) Instance = CollectEditorSettingInstance();

	return Instance;
}

UJointEditorSettings* UJointEditorSettings::CollectEditorSettingInstance()
{
	UJointEditorSettings* ProxyDevSettings = CastChecked<UJointEditorSettings>(UJointEditorSettings::StaticClass()->GetDefaultObject());
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	return ProxyDevSettings;
}
