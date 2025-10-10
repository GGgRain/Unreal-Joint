//Copyright 2022~2024 DevGrain. All Rights Reserved.

#undef UCONFIG_NO_TRANSLITERATION

using UnrealBuildTool;

public class Joint : ModuleRules
{
	public Joint(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		//bEnforceIWYU = true;


		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...

			}
			);
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Slate",
				"SlateCore",
				"UMG",
				"ICU",
				
				"GameplayAbilities",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"ICU",
				"DeveloperSettings",

				// Gameplay abilities with
				"GameplayTags",
				"GameplayTasks",
				"GameplayAbilities",
				
				"MovieScene",
			}
			);
		
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"DeveloperToolSettings",
				}
			);
		}
		
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		if (Target.bCompileICU)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "ICU");
		}

		//PublicDefinitions.Remove("UE_ENABLE_ICU");
		//PublicDefinitions.Add("UE_ENABLE_ICU=" + (Target.bCompileICU ? "1" : "0")); // Enable/disable (=1/=0) ICU usage in the codebase. NOTE: This flag is for use while integrating ICU and will be removed afterward.

		//PublicDefinitions.Remove("UCONFIG_NO_TRANSLITERATION");
		//PublicDefinitions.Add("UCONFIG_NO_TRANSLITERATION=" + (Target.bCompileICU ? "0" : "1")); // Enable/disable (=0 /=1) ICU usage in the codebase. NOTE: This flag is for use while integrating ICU and will be removed afterward.

	}
}
