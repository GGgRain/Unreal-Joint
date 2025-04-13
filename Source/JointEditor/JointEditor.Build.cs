//Copyright 2022~2024 DevGrain. All Rights Reserved.

using UnrealBuildTool;

public class JointEditor : ModuleRules
{
	public JointEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Joint", "EditorWidgets" });
		
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Any;

		//The path for the source files
		PrivateIncludePaths.AddRange(new[]
		{
			"JointEditor",

			"JointEditor/Public",

			"JointEditor/Public/Asset",
			"JointEditor/Public/Asset/Action",
			"JointEditor/Public/Asset/Factory",

			"JointEditor/Public/Editor",
			"JointEditor/Public/Editor/Graph",
			"JointEditor/Public/Editor/SearchTree",
			"JointEditor/Public/Editor/Management",
			"JointEditor/Public/Editor/Setting",
			"JointEditor/Public/Editor/Slate",
			"JointEditor/Public/Editor/Style",
			"JointEditor/Public/Editor/Toolkit",
			

			"JointEditor/Public/Node",
			"JointEditor/Public/Node/SubNode",
		});


		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Joint",
			
			"VoltCore",
			"Volt",

			"ApplicationCore",

			"EditorStyle",
			"EditorWidgets",

			"PropertyEditor",
			"ToolMenus",
			"ContentBrowser",
			"DesktopWidgets",

			"InputCore",
			"Projects",

			"Slate",
			"SlateCore",

			"RenderCore",

			"UMG",
			"LevelSequence",

			"KismetWidgets",
			"Kismet",

			"AIGraph",
			"GraphEditor",

			"ApplicationCore",

			"InputDevice",

			"EditorStyle",
			"AnimationBlueprintEditor",

			"DeveloperSettings",
			"AppFramework",

			// Gameplay abilities related
			"GameplayTags",
			"GameplayTasks",
			"GameplayAbilities",
			
			//Test
			"SessionServices",
			"MessageLog",
			
			//"SceneOutliner",
			//"ConfigEditor"
		});

		if (Target.Version.MajorVersion >= 5)
		{
			PrivateDependencyModuleNames.Add("ToolWidgets");
		}
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"BlueprintGraph",

			"AssetTools",
			"AssetRegistry",

			"ClassViewer",
		});
	}
}