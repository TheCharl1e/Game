// ZoneDef.h
// Designer-authored rules for one biome / zone type, stored in a UDataTable
// (one row per EZoneType; row name == short enum name, e.g. "Ocean", "Beach").
//
// ACaldrethZone reads its flags from here at spawn time — NEVER hard-coded against the
// enum — so balance tweaks (which biomes are spawnable / habitable) stay data-only and
// survive without a C++ recompile.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ZoneType.h"
#include "ZoneDef.generated.h"

/**
 * One row of zone rules, keyed by EZoneType (row name = short enum name).
 * Keep this struct small: it is copied per zone actor and there can be hundreds.
 */
USTRUCT(BlueprintType)
struct STAN_PIERWOTNY_API FZoneDef : public FTableRowBase
{
	GENERATED_BODY()

	/** Can resources / props / NPCs be spawned inside this zone during world-gen? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	bool bSpawnable = false;

	/** Can NPCs settle and live here (shelter, sleep, claim territory)? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	bool bHabitable = false;

	/** Human-readable label for HUD / debug overlays. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FText DisplayName;

	/** Tint used by the editor import tool and debug draw. Magenta = row not authored yet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FColor DebugColor = FColor::Magenta;

	/** Bazowa temperatura OTOCZENIA tej strefy (°C) — wejście dla AmbientTemp. Modyfikowana porą doby
	 *  (warstwa doby = następny etap; offset=0 teraz). Data-driven w DT_ZoneDefs. [TBD→tune] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	float BaseTemp = 20.0f;
};
