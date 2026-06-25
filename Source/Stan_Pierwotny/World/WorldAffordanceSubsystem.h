// WorldAffordanceSubsystem.h
// L0 Track A / Slice 1 — the registry of "what the world offers" (brief §2).
//
// Lightweight RECORDS, NOT an Actor per berry. Nearest-of-a-type via a 2D spatial hash. Claim via
// ReservedBy so two NPCs never eat the same berry. Regen on a TIMER (never Tick).
//
// PERF BUDGET (500+ NPC ceiling): FAffordanceHandle ~64 B × ~1-2k records = ~128 KB. QueryNearest is
// O(local cells), not O(all records). Reservation + consume are O(1) map lookups. Regen walks the live
// set once per RegenInterval, off the metabolism cadence — zero per-NPC cost.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AffordanceType.h"
#include "WorldAffordanceSubsystem.generated.h"

/**
 * One world resource the NPCs can use. Plain record — copied into a flat array, addressed by a stable Id.
 * Layout mirrors brief §2 (+ ColdDampenFactor moved here per decision #3, + private hashing/validity book-keeping).
 */
USTRUCT(BlueprintType)
struct FAffordanceHandle
{
	GENERATED_BODY()

	/** What this resource lets an NPC do. None == a freed slot (do not query). */
	UPROPERTY(BlueprintReadWrite, Category = "Affordance")
	EAffordanceType Type = EAffordanceType::None;

	/** World location of the resource (XY used for the spatial hash; Z kept for MoveTo/EQS). */
	UPROPERTY(BlueprintReadWrite, Category = "Affordance")
	FVector Location = FVector::ZeroVector;

	/** How much consumable yield is left. 0 == exhausted (filtered out of queries). */
	UPROPERTY(BlueprintReadWrite, Category = "Affordance")
	float RemainingYield = 0.f;

	/** Per game-hour regrowth. 0 == non-renewable (a depleted bush stays empty). */
	UPROPERTY(BlueprintReadWrite, Category = "Affordance")
	float MaxYield = 0.f;

	/** Per game-hour regrowth toward MaxYield. 0 == non-renewable. */
	UPROPERTY(BlueprintReadWrite, Category = "Affordance")
	float RegenPerHour = 0.f;

	/** Shelter only (decision #3): how strongly this place dampens the cold DEFICIT. 0 = none, 1 = full.
	 *  READ-SIDE ONLY in Slice 1 (GetFeltTemperature) — never feeds the locked ambient→body coupling. */
	UPROPERTY(BlueprintReadWrite, Category = "Affordance")
	float ColdDampenFactor = 0.f;

	/** The actor that owns/represents this record (e.g. the bush BP). Weak — auto-null on EndPlay. */
	UPROPERTY(BlueprintReadWrite, Category = "Affordance")
	TWeakObjectPtr<AActor> Owner;

	/** GetUniqueID() of the reserving actor, or INDEX_NONE. Brief §2 claim field. */
	UPROPERTY(BlueprintReadOnly, Category = "Affordance")
	int32 ReservedBy = INDEX_NONE;

	// --- runtime book-keeping (not part of the design contract) ---
	/** Weak ref to the reserver so a claim auto-frees if the NPC dies without Release (death fail-safe). */
	TWeakObjectPtr<AActor> ReservedByActor;
	/** Hash cell this record currently sits in, so Unregister can prune the cell list in O(1). */
	FIntVector HashCell = FIntVector::ZeroValue;
	/** Logged-once flag so "exhausted" fires on the transition, not every consume. */
	bool bExhaustLogged = false;
};

/**
 * World registry of affordances + claim. One per UWorld. NPCs/BT/EQS query it; nothing else owns this data.
 */
UCLASS()
class STAN_PIERWOTNY_API UWorldAffordanceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- UWorldSubsystem ---
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ── Registration (brief §2) ───────────────────────────────────────────────────────────────────
	/** Register a resource. Returns a STABLE Id (survives other (un)registers). BlueprintCallable so a
	 *  BP_BerryBush / BP_RiverPoint / Safe-Zone helper can self-register in BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "World|Affordance")
	int32 RegisterAffordance(const FAffordanceHandle& Handle);

	/** Convenience overload for Blueprints / simple spawners (no struct pin needed). */
	UFUNCTION(BlueprintCallable, Category = "World|Affordance")
	int32 RegisterAffordanceSimple(EAffordanceType Type, FVector Location, float Yield,
		float RegenPerHour, AActor* Owner, float ColdDampenFactor = 0.f);

	/** Fail-safe: call from the owner's EndPlay so a destroyed bush leaves no ghost record. */
	UFUNCTION(BlueprintCallable, Category = "World|Affordance")
	void UnregisterAffordance(int32 Id);

	// ── Query / claim / consume (brief §2) ────────────────────────────────────────────────────────
	/** Nearest VALID (IsValid Owner OR ownerless, RemainingYield>0, unreserved) affordance of Type within
	 *  Radius of From. INDEX_NONE if none. O(local cells) via the spatial hash. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "World|Affordance")
	int32 QueryNearest(EAffordanceType Type, const FVector& From, float Radius) const;

	/** Atomic claim. True only if the slot is valid and currently unreserved. */
	UFUNCTION(BlueprintCallable, Category = "World|Affordance")
	bool TryReserve(int32 Id, AActor* By);

	/** Drop a claim (must match the reserver). Called on task finish AND in the NPC destructor/EndPlay. */
	UFUNCTION(BlueprintCallable, Category = "World|Affordance")
	void Release(int32 Id, AActor* By);

	/** Take up to Amount of yield (revalidates on arrival per §2). OutGranted = actually given. */
	UFUNCTION(BlueprintCallable, Category = "World|Affordance")
	bool Consume(int32 Id, float Amount, float& OutGranted);

	/** Read a record by Id (false if freed/out of range). For BT tasks that need Location/yield. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "World|Affordance")
	bool GetAffordance(int32 Id, FAffordanceHandle& OutHandle) const;

	// ── EQS support (custom generator, non-actor records) ─────────────────────────────────────────
	/** Fill Out with locations of every valid, unreserved, yield>0 affordance of Type within Radius of
	 *  Center. Used by UEnvQueryGenerator_Affordances so EQS can distance-score + path-test them. */
	void GatherAffordanceLocations(EAffordanceType Type, const FVector& Center, float Radius,
		TArray<FVector>& OutLocations) const;

	// ── Thermo read-side (decision #3) ────────────────────────────────────────────────────────────
	/** Strongest shelter cold-dampen covering Location (0 = none). Read by Maslow::GetFeltTemperature only;
	 *  NEVER feeds the ambient→body coupling. "Covering" = within the shelter record's effect radius. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "World|Affordance")
	float GetShelterColdDampenAt(const FVector& Location) const;

	/** Effect radius (uu) a Shelter affordance covers for the cold-dampen read. [TBD→tune] */
	UPROPERTY(EditAnywhere, Category = "World|Affordance")
	float ShelterEffectRadius = 2500.f;

	/** Spatial-hash cell edge in uu. Pick ~ the common query radius so a query touches few cells. [TBD→tune] */
	UPROPERTY(EditAnywhere, Category = "World|Affordance")
	float CellSize = 2000.f;

	/** Seconds between regen passes (real time — Slice 1 has no world clock; RegenPerHour scaled by this). */
	UPROPERTY(EditAnywhere, Category = "World|Affordance")
	float RegenInterval = 10.f;

private:
	/** Flat record store; index == stable Id. Freed slots have Type==None and are recycled via FreeList. */
	UPROPERTY()
	TArray<FAffordanceHandle> Affordances;

	/** Recycled Ids (freed slots) so registration reuses memory and Ids stay stable for live records. */
	TArray<int32> FreeList;

	/** cell -> Ids living in that cell. Pruned on Unregister; rebuilt entries on Register. */
	TMap<FIntVector, TArray<int32>> SpatialHash;

	FTimerHandle RegenTimerHandle;

	/** World -> 2D hash cell (Z ignored, like ACaldrethZone's 2D zoning). */
	FIntVector ToCell(const FVector& Location) const;

	/** True if Id addresses a live, non-exhausted record whose owner (if any) is still valid. */
	bool IsLive(int32 Id) const;

	/** Timer callback: regrow renewable affordances toward MaxYield. Off the metabolism cadence. */
	void RegenTick();
};
