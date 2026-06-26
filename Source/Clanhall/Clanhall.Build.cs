// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Clanhall : ModuleRules
{
	public Clanhall(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Clanhall",
			"Clanhall/Animation",
			"Clanhall/Variant_Platforming",
			"Clanhall/Variant_Platforming/Animation",
			"Clanhall/Variant_Combat",
			"Clanhall/Variant_Combat/AI",
			"Clanhall/Variant_Combat/Animation",
			"Clanhall/Variant_Combat/Gameplay",
			"Clanhall/Variant_Combat/Interfaces",
			"Clanhall/Variant_Combat/UI",
			"Clanhall/Variant_SideScrolling",
			"Clanhall/Variant_SideScrolling/AI",
			"Clanhall/Variant_SideScrolling/Gameplay",
			"Clanhall/Variant_SideScrolling/Interfaces",
			"Clanhall/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
