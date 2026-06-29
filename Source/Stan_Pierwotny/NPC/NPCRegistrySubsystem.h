#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NPCRegistrySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNPCRegistry, Log, All);

class ACharacter;

/**
 * KEYSTONE (L3-01): resolves an int32 ID -> a live NPC.
 * One per world (lifecycle = world). Nothing social (P2P L3, reputation L4,
 * detective L5) works without this. Designed for 500-1000+ NPCs.
 *
 * IDs are simple, monotonically increasing (NextNPCId++), and NEVER recycled:
 * a dead NPC takes its number to the grave. No save/load -> IDs are runtime-only.
 * Zero tick, zero iteration: O(1) Add/Remove/Find only, logged only on
 * register/unregister/self-heal.
 */
UCLASS()
class STAN_PIERWOTNY_API UNPCRegistrySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Registers an NPC, assigns a fresh unique int32 ID (never recycled). Returns new ID, or 0 on failure. */
    int32 RegisterNPC(ACharacter* NPC);

    /** Removes an NPC by ID. The ID is retired and NEVER reused. Idempotent. */
    void UnregisterNPC(int32 NPCId);

    /** Resolves an int32 ID to a live NPC. nullptr if unknown/dead. Self-heals stale entries. */
    UFUNCTION(BlueprintCallable, Category="NPC|Registry")
    ACharacter* GetNPCById(int32 NPCId);

    /** Live registered NPC count (debug/telemetry). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="NPC|Registry")
    int32 GetRegisteredCount() const { return RegisteredNPCs.Num(); }

private:
    /** int32 ID -> live NPC. TWeakObjectPtr auto-nulls on destruction (safety net). */
    UPROPERTY()
    TMap<int32, TWeakObjectPtr<ACharacter>> RegisteredNPCs;

    /** Next ID to hand out. ONLY increments — IDs never recycled. 0 reserved as INVALID. */
    int32 NextNPCId = 1;
};
