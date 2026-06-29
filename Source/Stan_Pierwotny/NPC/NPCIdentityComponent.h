#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCIdentityComponent.generated.h"

/**
 * OCEAN / Big-Five personality (L3-02). Five traits in [0,1], per-instance.
 * Slice #1 hand-sets these on individual NPCs; the future hybrid (Q1) rolls them
 * from a UDataAsset archetype (means + variances) into this same shape.
 * 20 bytes/NPC — cheap to store per-instance for 500+ NPCs.
 */
USTRUCT(BlueprintType)
struct FOceanProfile
{
    GENERATED_BODY()

    /** Curiosity / explore-vs-exploit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category="NPC|OCEAN")
    float Openness = 0.5f;

    /** Reliability / storehouse-deposit discipline. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category="NPC|OCEAN")
    float Conscientiousness = 0.5f;

    /** Social drive / seeks contracts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category="NPC|OCEAN")
    float Extraversion = 0.5f;

    /** Cooperation / respect for property law (low → theft-prone). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category="NPC|OCEAN")
    float Agreeableness = 0.5f;

    /** Emotional volatility. Slice #1: high → panics at higher HP%. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category="NPC|OCEAN")
    float Neuroticism = 0.5f;
};

/**
 * The NPC's identity carrier (L3-01). Registers its owner with the
 * UNPCRegistrySubsystem on BeginPlay and unregisters on EndPlay, holding the
 * assigned int32 registry ID. Identity != biology: the whole social layer
 * (reputation, OCEAN, alibi) will hang on this component, not on metabolism.
 */
UCLASS(ClassGroup=(NPC), meta=(BlueprintSpawnableComponent))
class STAN_PIERWOTNY_API UNPCIdentityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNPCIdentityComponent();

    /** Registry ID. 0 = not registered. Assigned on BeginPlay, read by all social systems. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="NPC|Identity")
    int32 GetNPCId() const { return NPCId; }

    /** OCEAN personality (L3-02). Per-instance, editable in the editor. Read by Maslow (panic roll) & later BT. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|OCEAN")
    FOceanProfile Ocean;

    /** Cheap typed accessor for the panic roll. */
    UFUNCTION(BlueprintPure, Category="NPC|OCEAN")
    float GetNeuroticism() const { return Ocean.Neuroticism; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    /** Assigned by registry on BeginPlay. Never changes once set. 0 = unregistered. */
    UPROPERTY(VisibleInstanceOnly, Category="NPC|Identity")
    int32 NPCId = 0;
};
