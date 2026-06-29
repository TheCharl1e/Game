#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HeatSourceRegistrySubsystem.generated.h"

class UHeatSourceComponent;

/**
 * L1-09b (fire): one per world. Heat sources (UHeatSourceComponent) register here; NPC Maslow components
 * query GetRadiantHeatAt(their location) each metabolism cadence to add local warmth to AmbientTemp.
 *
 * Perf (500+ NPC): fires are SPARSE (a village ~10-50), so per-NPC iteration over sources is cheap
 * (500 NPC × ~50 sources × once/~10s). No spatial grid for now — add only if sources reach thousands.
 * Zero tick: only Register/Unregister + the on-demand query.
 */
UCLASS()
class STAN_PIERWOTNY_API UHeatSourceRegistrySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Add a heat source (idempotent). Called by UHeatSourceComponent::BeginPlay. */
    void RegisterSource(UHeatSourceComponent* Source);

    /** Remove a heat source (idempotent). Called by UHeatSourceComponent::EndPlay. */
    void UnregisterSource(UHeatSourceComponent* Source);

    /**
     * Total radiant heat (°C added to AmbientTemp) at a world location: SUM over lit, in-range sources of
     * Warmth × (1 − d/Radius)² (huddle falloff), CLAMPED to MaxRadiantHeat (a stack of fires can't cook an NPC).
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Temperature")
    float GetRadiantHeatAt(const FVector& Location) const;

    /** Górny limit zsumowanego RadiantHeat (°C) — chroni przed ugotowaniem przez stos ognisk. [tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta = (ClampMin = "0.0"))
    float MaxRadiantHeat = 40.0f;

private:
    /** Weak ptrs — auto-null on source destruction (safety net), location read live (fire może się ruszać). */
    UPROPERTY()
    TArray<TWeakObjectPtr<UHeatSourceComponent>> Sources;
};
