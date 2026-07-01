// Fill out your copyright notice in the Description page of Project Settings.

#include "NPCSpawner.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "NavigationInvokerComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPCSpawner, Log, All);

ANPCSpawner::ANPCSpawner()
{
    PrimaryActorTick.bCanEverTick = false;

    // The spawner is itself a nav invoker: in invoker-only mode this seeds the dynamic navmesh
    // around the spawn area so GetRandomReachablePointInRadius has nav to sample before any NPC exists.
    SpawnerInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("SpawnerInvoker"));
}

void ANPCSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (SpawnerInvoker)
    {
        // Cover the whole spawn ring (SpawnRadius may have been bumped on the placed instance).
        SpawnerInvoker->SetGenerationRadii(SpawnRadius + 2000.f, SpawnRadius + 4000.f);
    }

    if (bSpawnOnBeginPlay)
    {
        // Defer so the spawner invoker's dynamic navmesh is built before we sample it.
        if (SpawnDelay > 0.f)
        {
            GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ANPCSpawner::SpawnWave, SpawnDelay, false);
        }
        else
        {
            SpawnWave();
        }
    }
}

int32 ANPCSpawner::SpawnWave()
{
    UWorld* World = GetWorld();
    if (!World || !NPCClass)
    {
        UE_LOG(LogNPCSpawner, Warning, TEXT("[Spawner] Abort: World=%d NPCClass=%d"),
            World != nullptr, NPCClass != nullptr);
        return 0;
    }

    UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!Nav)
    {
        UE_LOG(LogNPCSpawner, Warning, TEXT("[Spawner] Abort: no NavigationSystem in world"));
        return 0;
    }

    const FVector Origin = GetActorLocation();
    int32 Spawned = 0;
    int32 NavMisses = 0;

    for (int32 i = 0; i < SpawnCount; ++i)
    {
        FNavLocation NavPt;
        if (!Nav->GetRandomReachablePointInRadius(Origin, SpawnRadius, NavPt))
        {
            ++NavMisses;
            continue;
        }

        FVector Loc = NavPt.Location;
        Loc.Z += SpawnZOffset;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ACharacter* NPC = World->SpawnActor<ACharacter>(
            NPCClass, Loc, FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Params);
        if (NPC)
        {
            ++Spawned;

            // Each NPC carries a nav invoker so the dynamic navmesh follows it across the 10 km island
            // (invoker-only mode: fine cells stay affordable because nav is built only in these radii).
            UNavigationInvokerComponent* Invoker =
                NewObject<UNavigationInvokerComponent>(NPC, TEXT("NPCNavInvoker"));
            if (Invoker)
            {
                Invoker->RegisterComponent();
                Invoker->SetGenerationRadii(NPCInvokerGenerationRadius, NPCInvokerRemovalRadius);
            }
        }
    }

    UE_LOG(LogNPCSpawner, Log,
        TEXT("[Spawner] Spawned %d/%d NPCs within %.0f uu of %s (nav misses=%d)"),
        Spawned, SpawnCount, SpawnRadius, *Origin.ToString(), NavMisses);
    return Spawned;
}
