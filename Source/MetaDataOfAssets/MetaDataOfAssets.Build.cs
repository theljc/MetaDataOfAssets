// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MetaDataOfAssets : ModuleRules
{
	public MetaDataOfAssets(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.Never;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",

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
				"Blutility",
				"InputCore",
				"UnrealEd",
				"Projects",
				"UMGEditor",
				"UMG",
				"EditorSubsystem",
				"BlueprintEditorLibrary",
				"AssetRegistry",
				"StructUtils",
				"DeveloperSettings",
				"Settings",
				"EditorScriptingUtilities",

				// ... add private dependencies that you statically link with here ...
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
