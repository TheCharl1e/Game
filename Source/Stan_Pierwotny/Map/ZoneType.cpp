// ZoneType.cpp
// EZoneType has no runtime behaviour of its own (it is a header-only UENUM). The only
// purpose of this translation unit is to ENFORCE, at compile time, that every enumerator
// keeps the exact integer value of its matching biome id in caldreth_biome.png /
// caldreth_manifest.json biome_legend (Tools/MapGen/caldreth_mapgen.py MapConfig).
//
// If someone reorders or renumbers EZoneType, the importer would otherwise silently place
// the wrong zones for every pixel. These static_asserts turn that into a loud build error
// pointing at the exact mismatch instead.

#include "ZoneType.h"

static_assert((uint8)EZoneType::Ocean       == 0,  "EZoneType::Ocean must equal biome id 0 (OCEAN)");
static_assert((uint8)EZoneType::Beach       == 1,  "EZoneType::Beach must equal biome id 1 (BEACH)");
static_assert((uint8)EZoneType::Savanna     == 2,  "EZoneType::Savanna must equal biome id 2 (SAVANNA)");
static_assert((uint8)EZoneType::Desert      == 3,  "EZoneType::Desert must equal biome id 3 (DESERT)");
static_assert((uint8)EZoneType::Grassland   == 4,  "EZoneType::Grassland must equal biome id 4 (GRASSLAND)");
static_assert((uint8)EZoneType::SlopeForest == 5,  "EZoneType::SlopeForest must equal biome id 5 (SLOPE_FOREST)");
static_assert((uint8)EZoneType::Mountain    == 6,  "EZoneType::Mountain must equal biome id 6 (MOUNTAIN)");
static_assert((uint8)EZoneType::AshSlope    == 7,  "EZoneType::AshSlope must equal biome id 7 (ASH_SLOPE)");
static_assert((uint8)EZoneType::Caldera     == 8,  "EZoneType::Caldera must equal biome id 8 (CALDERA)");
static_assert((uint8)EZoneType::River       == 9,  "EZoneType::River must equal biome id 9 (RIVER)");
static_assert((uint8)EZoneType::Lava        == 10, "EZoneType::Lava must equal biome id 10 (LAVA)");
static_assert((uint8)EZoneType::Oasis       == 11, "EZoneType::Oasis must equal biome id 11 (OASIS)");
