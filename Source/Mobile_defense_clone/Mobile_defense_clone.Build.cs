// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Mobile_defense_clone : ModuleRules
{
	public Mobile_defense_clone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Mobile_defense_clone",
			"Mobile_defense_clone/Variant_Strategy",
			"Mobile_defense_clone/Variant_Strategy/UI",
			"Mobile_defense_clone/Variant_TwinStick",
			"Mobile_defense_clone/Variant_TwinStick/AI",
			"Mobile_defense_clone/Variant_TwinStick/Gameplay",
			"Mobile_defense_clone/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
