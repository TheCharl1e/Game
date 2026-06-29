// Editor-only module: Caldreth map-import tooling. Never compiled into the packaged game.

using UnrealBuildTool;

public class Stan_PierwotnyEditor : ModuleRules
{
	public Stan_PierwotnyEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Stan_Pierwotny = runtime types (EZoneType, FZoneDef, ACaldrethZone).
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Stan_Pierwotny" });

		// UnrealEd = GEditor / editor world / FScopedTransaction; ImageWrapper = decode the biome PNG;
		// Json = parse caldreth_manifest.json for the POI import.
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "ImageWrapper", "Json" });
	}
}
