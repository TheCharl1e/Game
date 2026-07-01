// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPCSpawner.generated.h"

class ACharacter;
class UNavigationInvokerComponent;

/**
 * G1-K1 scene populator. Places N NPCs on the navmesh within a radius of this actor.
 * No Tick — one-shot on BeginPlay (or BlueprintCallable SpawnWave for manual/param bumps).
 * NPC self-registers in NPCRegistry via its UNPCIdentityComponent::BeginPlay.
 */
UCLASS()
class STAN_PIERWOTNY_API ANPCSpawner : public AActor
{
    GENERATED_BODY()

public:
    ANPCSpawner();

    /** How many NPCs to spawn. Param — bump to 200/500 for the scale DoD. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    int32 SpawnCount = 50;

    /** Navmesh sampling radius around this actor's location (uu). Keep inside the playable floor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    float SpawnRadius = 3500.f;

    /** NPC class to spawn — set to BP_NPC_Character on the placed instance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    TSubclassOf<ACharacter> NPCClass;

    /** Auto-run the wave on BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    bool bSpawnOnBeginPlay = true;

    /** Lift spawned NPC by this Z so the capsule is not embedded in the floor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    float SpawnZOffset = 100.f;

    /** Per-NPC nav-invoker generation radius (uu) — dynamic navmesh builds this far around each NPC. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|NavInvoker")
    float NPCInvokerGenerationRadius = 3000.f;

    /** Per-NPC nav-invoker removal radius (uu) — tiles beyond this are dropped as the NPC leaves. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|NavInvoker")
    float NPCInvokerRemovalRadius = 5000.f;

    /** Delay (s) before the wave, so the spawner's own invoker seeds navmesh first (invoker-only mode). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|NavInvoker")
    float SpawnDelay = 1.0f;

    /** Spawn the wave now. Returns how many actually spawned. BlueprintCallable for debug/param bumps. */
    UFUNCTION(BlueprintCallable, Category = "Spawn")
    int32 SpawnWave();

protected:
    virtual void BeginPlay() override;

private:
    /** Seeds navmesh around the spawn area so nav-projection works before any NPC exists (invoker-only mode). */
    UPROPERTY(VisibleAnywhere, Category = "Spawn|NavInvoker")
    TObjectPtr<UNavigationInvokerComponent> SpawnerInvoker;

    FTimerHandle SpawnTimerHandle;
};
