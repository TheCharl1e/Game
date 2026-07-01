// CaldrethImportLibrary.h
// Editor-only brain for the Caldreth map import: decode caldreth_biome.png, flood-fill it
// into contiguous single-biome regions, and place one ACaldrethZone per region.
//
// This is the C++ "brain"; the front-end is a UEditorUtilityWidget (Blueprint "body") with
// a button that calls ImportCaldrethZones. Kept as a static BlueprintFunctionLibrary so the
// widget asset stays a thin caller.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CaldrethImportLibrary.generated.h"

class UDataTable;
class UMaterialInterface;

// Editor-side log category. The runtime LogCaldreth lives in the Stan_Pierwotny module and
// is not exported across modules, so the import tool logs through its own category here.
DECLARE_LOG_CATEGORY_EXTERN(LogCaldrethImport, Log, All);

UCLASS()
class STAN_PIERWOTNYEDITOR_API UCaldrethImportLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Decode an 8-bit greyscale biome PNG, flood-fill into contiguous single-biome regions,
	 * and spawn one ACaldrethZone per region into the current editor world.
	 *
	 * @param BiomePngPath    Path to caldreth_biome.png. Empty = <Project>/MapData/caldreth_biome.png.
	 * @param ZoneTable       FZoneDef DataTable assigned to every spawned zone (may be null; flags then stay false).
	 * @param WorldSizeUU     Island edge length in unreal units; normalized coords map onto [-WorldSizeUU/2, +WorldSizeUU/2], centered on origin.
	 * @param MinRegionPixels Regions smaller than this are skipped as noise.
	 * @param bUse8Connected  8-connectivity for flood-fill instead of 4 (merges diagonal touches).
	 * @param bSkipOcean      Skip biome id 0 (OCEAN) so the sea is not turned into a giant zone actor.
	 * @return Number of ACaldrethZone actors placed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Caldreth|Import")
	static int32 ImportCaldrethZones(
		FString BiomePngPath,
		UDataTable* ZoneTable,
		float WorldSizeUU = 1000000.f,
		int32 MinRegionPixels = 8,
		bool bUse8Connected = false,
		bool bSkipOcean = false);

	/**
	 * Read caldreth_manifest.json, spawn one ACaldrethPOIMarker per points_of_interest entry,
	 * and tag each marker with its "role". Normalized x/y map onto the same centered world box
	 * as ImportCaldrethZones, so pass the SAME WorldSizeUU to keep markers aligned to zones.
	 *
	 * @param ManifestPath Path to caldreth_manifest.json. Empty = <Project>/MapData/caldreth_manifest.json.
	 * @param WorldSizeUU  Island edge length in unreal units; normalized coords map onto [-WorldSizeUU/2, +WorldSizeUU/2], centered on origin.
	 * @return Number of ACaldrethPOIMarker actors placed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Caldreth|Import")
	static int32 ImportCaldrethPOIs(
		FString ManifestPath,
		float WorldSizeUU = 1000000.f);

	/**
	 * Data-driven Landscape build: read a 16-bit little-endian heightmap (.r16), spawn one ALandscape
	 * covering [-WorldSizeUU/2, +WorldSizeUU/2] centered on origin, and import the heights.
	 * Static one-time bake; re-callable to regenerate terrain from a fresh MapGen heightmap.
	 *
	 * Height mapping: stored uint16 [0..65535] -> world Z via Landscape ScaleZ + ZOffsetUU.
	 * Defaults (Caldreth): 505x505 verts (63 quads x 8 components), XY scale = WorldSizeUU/(SizeVerts-1),
	 * ScaleZ 175.78 (elevation span 0..1 -> 90000 UU), ZOffsetUU +45000 (elev 0 -> Z 0, sea_level 0.2 -> Z 18000, peak -> Z 90000).
	 *
	 * @param HeightmapR16Path  Path to .r16. Empty = <Project>/MapData/caldreth_height_505.r16.
	 * @param SizeVerts         Heightmap edge in vertices (must equal file: SizeVerts*SizeVerts uint16).
	 * @param SubsectionSizeQuads Quads per section (63 for the 505 layout).
	 * @param NumSubsections    Sections per component (1 for the 505 layout).
	 * @param WorldSizeUU       Island edge in unreal units (XY), centered on origin.
	 * @param ZScale            Landscape actor Scale.Z (full 16-bit span -> 65535 * (1/128) * ZScale UU).
	 * @param ZOffsetUU         Actor Location.Z offset so elevation 0 lands at this Z minus half-span.
	 * @param LandscapeMaterial Material assigned before import so component material instances build and the
	 *                          surface actually RENDERS. Null = /Engine/EngineMaterials/WorldGridMaterial.
	 * @return Spawned ALandscape (nullptr on failure).
	 */
	UFUNCTION(BlueprintCallable, Category = "Caldreth|Import")
	static AActor* ImportCaldrethLandscape(
		FString HeightmapR16Path,
		int32 SizeVerts = 505,
		int32 SubsectionSizeQuads = 63,
		int32 NumSubsections = 1,
		float WorldSizeUU = 1000000.f,
		float ZScale = 175.78f,
		float ZOffsetUU = 45000.f,
		UMaterialInterface* LandscapeMaterial = nullptr);
};
