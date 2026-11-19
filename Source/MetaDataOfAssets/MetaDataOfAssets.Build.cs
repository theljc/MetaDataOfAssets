// Copyright 2026 ljc

using UnrealBuildTool;

public class MetaDataOfAssets : ModuleRules
{
	public MetaDataOfAssets(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;
		OptimizeCode = CodeOptimization.Never;
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Blutility",
				"UMG",
				"StructUtils",
				"DeveloperSettings",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"InputCore",
				"UnrealEd",
				"Projects",
				"UMGEditor",
				"EditorSubsystem",
				"AssetRegistry",
				"Settings",
				"EditorScriptingUtilities",
				"StatusBar",
				"LevelEditor",
				"MainFrame",
			}
		);
	}
}
