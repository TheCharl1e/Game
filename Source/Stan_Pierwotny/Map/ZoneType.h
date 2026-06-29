// ZoneType.h
// Biome / zone classification for the Caldreth island map-import layer.
//
// The underlying uint8 value of each enumerator is IDENTICAL to the biome id written
// into MapData/caldreth_biome.png (greyscale pixel value) and listed in
// caldreth_manifest.json -> biome_legend. This 1:1 mirror lets the importer cast a
// raw pixel byte straight to EZoneType with no lookup table.
//
// Source of truth for the legend = MapConfig in Tools/MapGen/caldreth_mapgen.py.
// ZoneType.cpp holds static_asserts that break the build if these values ever drift.

#pragma once

#include "CoreMinimal.h"
#include "ZoneType.generated.h"

/**
 * Caldreth biome / zone type. uint8 value == biome id in caldreth_biome.png.
 * BlueprintType so designers can branch on it; data-driven rules (spawnable/habitable)
 * live in the FZoneDef DataTable (ETAP 2), never hard-coded against this enum.
 */
UENUM(BlueprintType)
enum class EZoneType : uint8
{
	Ocean		= 0		UMETA(DisplayName = "Ocean"),
	Beach		= 1		UMETA(DisplayName = "Beach"),
	Savanna		= 2		UMETA(DisplayName = "Savanna"),
	Desert		= 3		UMETA(DisplayName = "Desert"),
	Grassland	= 4		UMETA(DisplayName = "Grassland"),
	SlopeForest	= 5		UMETA(DisplayName = "Slope Forest"),
	Mountain	= 6		UMETA(DisplayName = "Mountain"),
	AshSlope	= 7		UMETA(DisplayName = "Ash Slope"),
	Caldera		= 8		UMETA(DisplayName = "Caldera"),
	River		= 9		UMETA(DisplayName = "River"),
	Lava		= 10	UMETA(DisplayName = "Lava"),
	Oasis		= 11	UMETA(DisplayName = "Oasis"),
};
