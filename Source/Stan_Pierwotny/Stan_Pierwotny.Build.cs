// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Stan_Pierwotny : ModuleRules
{
	public Stan_Pierwotny(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "GameplayStateTreeModule", "StateTreeModule", "EnhancedInput", "AIModule", "NavigationSystem", "UMG" });

		PublicDefinitions.Add("ENABLE_FORMAT_STRING_SANITIZATION=0");

		PublicIncludePaths.AddRange(
			new string[] {
				"Stan_Pierwotny",
				"Stan_Pierwotny/Map",
				"Stan_Pierwotny/AI",
				"Stan_Pierwotny/NPC",
				"Stan_Pierwotny/RTS",
				"Stan_Pierwotny/TP_ThirdPerson",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_SideScrolling",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_SideScrolling/Interfaces",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_SideScrolling/Gameplay",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_SideScrolling/AI",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_SideScrolling/UI",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_Combat",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_Combat/Interfaces",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_Combat/AI",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_Combat/Gameplay",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_Combat/UI",
				"Stan_Pierwotny/TP_ThirdPerson/Variant_Platforming"
			}
		);

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
