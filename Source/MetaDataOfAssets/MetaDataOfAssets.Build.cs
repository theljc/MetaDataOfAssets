// Copyright 2026 ljc

using UnrealBuildTool;

public class MetaDataOfAssets : ModuleRules
{
	public MetaDataOfAssets(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;
		OptimizeCode = CodeOptimization.Never;
		// bUseUnity = false;
		
		PublicIncludePaths.AddRange(
			new string[] 
			{
				
			}
		);


		PrivateIncludePaths.AddRange(
			new string[] 
			{
				
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",

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

			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				
			}
		);
		
	}
}
