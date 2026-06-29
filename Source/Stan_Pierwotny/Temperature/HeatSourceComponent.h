#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeatSourceComponent.generated.h"

/**
 * L1-09b (fire): makes the owning actor a heat source (campfire, torch, forge).
 * Registers itself in the world's UHeatSourceRegistrySubsystem on BeginPlay, unregisters on EndPlay
 * (same lifecycle pattern as UNPCIdentityComponent ↔ UNPCRegistrySubsystem). The Maslow component
 * of nearby NPCs queries the registry each metabolism cadence and adds the radiant heat to AmbientTemp
 * (local warmth bubble) — reusing the existing thermoregulation regimes, no new regime.
 *
 * Falloff is "huddle close": Warmth × (1 − d/Radius)² — warmth concentrated near the fire, sharp drop
 * toward the edge, so NPCs must come close. Multiple overlapping sources SUM, clamped in the registry.
 */
UCLASS(ClassGroup=(Temperature), meta=(BlueprintSpawnableComponent))
class STAN_PIERWOTNY_API UHeatSourceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHeatSourceComponent();

    /** Ciepło (°C) dodawane do AmbientTemp w SAMYM centrum (d=0). Falloff (1−d/Radius)². [tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta = (ClampMin = "0.0"))
    float Warmth = 25.0f;

    /** Zasięg (uu). Poza nim 0 ciepła. [tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta = (ClampMin = "1.0"))
    float Radius = 600.0f;

    /** Czy zapalone. Zgaszone ognisko = zarejestrowane, ale nie grzeje (BP może gasić/palić). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
    bool bIsLit = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
