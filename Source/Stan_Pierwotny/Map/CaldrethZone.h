// CaldrethZone.h
// A single contiguous biome region placed on the Caldreth island.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZoneType.h"
#include "ZoneDef.h"
#include "CaldrethZone.generated.h"

class UDataTable;

// Dedicated log category for the Caldreth map-import / zone layer.
DECLARE_LOG_CATEGORY_EXTERN(LogCaldreth, Log, All);

/**
 * Lightweight data marker for one biome region (no mesh, no Tick) — built to scale to
 * hundreds of zones with 500+ NPCs querying them.
 *
 * Gameplay flags (spawnable / habitable) are NOT stored on the actor: they are looked up
 * once from ZoneTable (FZoneDef) by ZoneType and cached, so design stays data-driven and
 * per-frame NPC queries never touch the DataTable.
 */
UCLASS()
class STAN_PIERWOTNY_API ACaldrethZone : public AActor
{
	GENERATED_BODY()

public:
	ACaldrethZone();

	/** Which biome this region is. Drives the FZoneDef row lookup. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caldreth|Zone")
	EZoneType ZoneType = EZoneType::Ocean;

	/**
	 * Region outline in normalized island space (0..1): x grows east, y grows south —
	 * the same convention as caldreth_manifest.json POIs. World placement multiplies
	 * these by WorldSizeUU, so the data survives a change of island world size.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caldreth|Zone")
	TArray<FVector2D> NormalizedOutline;

	/**
	 * Island edge length in unreal units this zone was placed with (the same WorldSizeUU passed to
	 * the importer). Lets the zone convert a world location back into its own normalized space, so
	 * GetZoneAtLocation needs no scale argument. Keep every zone on one island consistent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caldreth|Zone")
	float WorldSizeUU = 1000000.f;

	/** DataTable of FZoneDef rows (one per EZoneType). Source of bSpawnable / bHabitable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caldreth|Zone",
		meta = (RequiredAssetDataTags = "RowStructure=/Script/Stan_Pierwotny.ZoneDef"))
	TObjectPtr<UDataTable> ZoneTable = nullptr;

	/** True if this zone's FZoneDef allows spawning. False-safe if table/row missing. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Caldreth|Zone")
	bool IsSpawnable() const;

	/** True if this zone's FZoneDef allows habitation. False-safe if table/row missing. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Caldreth|Zone")
	bool IsHabitable() const;

	/** Copy out the resolved FZoneDef (DisplayName / DebugColor / flags). Returns false if unresolved. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Caldreth|Zone")
	bool GetZoneDef(FZoneDef& OutDef) const;

	/** DataTable row name for a zone type == its short enum name (e.g. "Ocean"). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Caldreth|Zone")
	static FName ZoneTypeToRowName(EZoneType InZoneType);

	/**
	 * Is WorldLocation (XY) inside this zone's region? Converts to normalized space via WorldSizeUU
	 * and runs a point-in-polygon test against NormalizedOutline. Z is ignored (island is 2D-zoned).
	 * Cheap: a handful of FVector2D, no allocation.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Caldreth|Zone")
	bool ContainsWorldLocation(const FVector& WorldLocation) const;

	/** Normalized area of NormalizedOutline (shoelace). Used to pick the most specific zone in a query. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Caldreth|Zone")
	float GetNormalizedArea() const;

	/**
	 * The zone an NPC is standing in — the cheap runtime query the whole layer exists for.
	 *
	 * Iterates the ACaldrethZone actors already placed in the world (NO 512x512 grid is kept in RAM),
	 * returns the SMALLEST-area zone whose outline contains Location, so a small inner region (Beach,
	 * Oasis) wins over the big region it sits inside (Ocean). Returns nullptr if no zone contains it.
	 * No Tick, no state: call it on demand (path decisions, on-enter events), not every frame.
	 */
	UFUNCTION(BlueprintCallable, Category = "Caldreth|Zone", meta = (WorldContext = "WorldContextObject"))
	static ACaldrethZone* GetZoneAtLocation(const UObject* WorldContextObject, const FVector& Location);

	/** Convenience wrapper: the EZoneType at Location, defaulting to Ocean when no zone contains it. */
	UFUNCTION(BlueprintCallable, Category = "Caldreth|Zone", meta = (WorldContext = "WorldContextObject"))
	static EZoneType GetZoneTypeAtLocation(const UObject* WorldContextObject, const FVector& Location);

	// ==== L0 Track A / Slice 1 — Safe Zone (emergent shelter affordance) ====
	/** Designer flag: this zone is a Safe Zone. On BeginPlay it self-registers a Shelter affordance so
	 *  EQS_FindSafeZone (affordance generator, type Shelter) can find it and NPCs can attach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SafeZone")
	bool bIsSafeZone = false;

	/** Authored seed for the emergent Shelter affordance's cold-DEFICIT dampen (0..1). Lives on the ACTOR,
	 *  NOT on FZoneDef (decision #3: Safe Zones are emergent affordances, not biomes). Read-side only —
	 *  GetFeltTemperature consumes it (ADDITIVE: 0 = no shelter = raw ambient, 1 = full shelter = felt pulled
	 *  to ComfortTempMin); it never feeds the locked ambient→body coupling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SafeZone", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShelterColdDampenFactor = 0.5f;

	/** NPCs currently homed to this zone (weak — dead members auto-null). Not Blueprint-exposed: UHT does
	 *  not support a TArray<TWeakObjectPtr> as a Blueprint property. C++ (BTTask_AttachToSafeZone) owns it. */
	UPROPERTY(VisibleAnywhere, Category = "SafeZone")
	TArray<TWeakObjectPtr<AActor>> AssignedMembers;

	/** Register an NPC as a member of this zone (deduped, prunes stale). Called by BTTask_AttachToSafeZone. */
	UFUNCTION(BlueprintCallable, Category = "SafeZone")
	void RegisterMember(AActor* Member);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Id of this zone's Shelter affordance in UWorldAffordanceSubsystem (INDEX_NONE if not a safe zone). */
	int32 ShelterAffordanceId = INDEX_NONE;

	/** Resolve ZoneTable's row for ZoneType into CachedDef. One lookup, then cached. */
	void ResolveZoneDef();

	/** Cached copy of the resolved row so NPC queries never hit the DataTable. */
	UPROPERTY(Transient)
	FZoneDef CachedDef;

	/** True once ResolveZoneDef found a valid row; gates the accessors' fast path. */
	UPROPERTY(Transient)
	bool bZoneDefResolved = false;
};
