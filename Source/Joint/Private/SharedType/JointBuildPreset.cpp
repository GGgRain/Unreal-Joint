// Fill out your copyright notice in the Description page of Project Settings.


#include "SharedType/JointBuildPreset.h"

#include "Interfaces/ITargetPlatform.h"

#include "Misc/EngineVersionComparison.h"


#if !UE_VERSION_OLDER_THAN(5, 2, 0) && WITH_EDITORONLY_DATA
		
#include "Settings/PlatformsMenuSettings.h"

#endif


#if UE_VERSION_OLDER_THAN(5, 2, 0) && WITH_EDITORONLY_DATA
		
#include "Settings/ProjectPackagingSettings.h"

#endif





UJointBuildPreset::UJointBuildPreset()
#if WITH_EDITORONLY_DATA
	:
	OnClientBehavior(EJointBuildPresetBehavior::Include),
	OnServerBehavior(EJointBuildPresetBehavior::Include),
	DefaultBuildTargetBehavior(EJointBuildPresetBehavior::Include)
#endif
{
#if WITH_EDITORONLY_DATA

	PresetColor = FLinearColor(0.1, 0.1, 0.1);

	PresetInitial = INVTEXT("BTPS");

#endif
}

const bool UJointBuildPreset::AllowForClient() const
{
#if WITH_EDITORONLY_DATA

	if(OnClientBehavior == EJointBuildPresetBehavior::Exclude) return false;
	
#endif

	return true;
}

const bool UJointBuildPreset::AllowForServer() const
{
#if WITH_EDITORONLY_DATA

	if(OnServerBehavior == EJointBuildPresetBehavior::Exclude) return false;
	
#endif

	return true;
}

const bool UJointBuildPreset::AllowForBuildTarget(const ITargetPlatform* TargetPlatform)
{
#if WITH_EDITORONLY_DATA

	if(TargetPlatform)
	{
		
#if UE_VERSION_OLDER_THAN(5, 2, 0)
		
		const FName PlatformName = FName(GetDefault<UProjectPackagingSettings>()->GetBuildTargetForPlatform(FName(TargetPlatform->IniPlatformName())));

#elif UE_VERSION_OLDER_THAN(5, 7, 0)

		const FName PlatformName = FName(GetDefault<UPlatformsMenuSettings>()->GetBuildTargetForPlatform(FName(TargetPlatform->IniPlatformName())));
#else
		
		const FName PlatformName = FName(GetDefault<UPlatformsMenuSettings>()->PackageBuildTarget);

#endif
		
		if(PerBuildTargetSetting.Contains(PlatformName))
		{
			if(PerBuildTargetSetting[PlatformName] == EJointBuildPresetBehavior::Exclude) return false;
		}else
		{
			if(DefaultBuildTargetBehavior == EJointBuildPresetBehavior::Exclude) return false;
		}
	}

#endif
	
	return true;

}
