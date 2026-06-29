// AffordanceType.h
// L0 Track A / Slice 1 — the kinds of "verbs the world offers" that NPCs discover and consume.
//
// An affordance is a LIGHTWEIGHT RECORD in UWorldAffordanceSubsystem (NOT an Actor per berry) —
// see the perf budget in that header. This enum is the single classifier; new world resources add an
// entry here and register a record, never a new C++ system.
//
// Slice 1 ships Nourishment (berry bush), Hydration (river point) and Shelter (Safe Zone cold-dampen).
// Shelter is an EMERGENT affordance (decision #3): the cold-dampen lives on the affordance record, NOT
// on FZoneDef — Safe Zones are claimed places, not biomes.

#pragma once

#include "CoreMinimal.h"
#include "AffordanceType.generated.h"

// ── Debug log categories for the whole Slice 1 world/exploration layer (brief §7) ────────────────────
//  Declared here (lightweight, widely included) so every subsystem / BT task shares one set.
//  Definitions live in WorldAffordanceSubsystem.cpp (one TU each).
DECLARE_LOG_CATEGORY_EXTERN(LogWorldAffordance, Log, All); // register / consume / exhaust / reserve
DECLARE_LOG_CATEGORY_EXTERN(LogExploration,     Log, All); // wander dir, safezone hit, attach, home lost

/**
 * What an affordance lets an NPC do. uint8 + BlueprintType so designers can branch and so the
 * EQS generator / BT tasks can take it as a typed parameter.
 */
UENUM(BlueprintType)
enum class EAffordanceType : uint8
{
	None         = 0 UMETA(DisplayName = "None"),
	Nourishment  = 1 UMETA(DisplayName = "Nourishment (food)"),   // berry bush — feeds Hunger (L1 nutrition)
	Hydration    = 2 UMETA(DisplayName = "Hydration (water)"),    // river point — feeds Thirst (L1 hydration)
	Shelter      = 3 UMETA(DisplayName = "Shelter (cold dampen)") // Safe Zone — read-side cold-deficit dampen (decision #3)
};
