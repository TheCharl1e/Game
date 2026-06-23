#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"   // FAIStimulus (by-value UFUNCTION param needs the full type)
#include "NPCPerceptionComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNPCPerception, Log, All);

class AItemBase;
class UAIPerceptionComponent;

/**
 * TECH-11 step 1: the single source of truth for "what this NPC sees RIGHT NOW".
 *
 * Lives on the AIController (BP_NPC_AI), wraps the controller's AIPerception_Sight,
 * and turns its OnTargetPerceptionUpdated events into a clean, self-pruning list of
 * currently-perceived food. Replaces the old BP add-only `Food` array + monotonic
 * `HowMuchFood` counter, which lied: it never removed on loss/destroy, grew forever,
 * and bloated with nulls after DestroyActor (×500 = leak + rising ForEach cost).
 *
 * Scope: FOOD ONLY (Drink/Shelter merge = TECH-11b). MaxAge stays 0 — "see now"
 * with no forgetting; forgetting (memory) is the next gate (L3-MEM). The empty
 * OnFoodPerceptionLost hook is emitted now so memory can subscribe later at zero cost.
 *
 * Storage is TWeakObjectPtr (auto-nulls on actor destruction = safety net); the
 * count getter counts only IsValid entries, so the count NEVER lies structurally.
 * No tick, no brute GetAllActorsOfClass scan — purely event-driven. 500+ NPC safe:
 * ~Num(perceived-food) ints/NPC, work only on perception-change events.
 */
UCLASS(ClassGroup=(NPC), meta=(BlueprintSpawnableComponent))
class STAN_PIERWOTNY_API UNPCPerceptionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNPCPerceptionComponent();

    /** Currently-perceived, still-valid food count. Counts only IsValid → never lies (TECH-11 fix). */
    UFUNCTION(BlueprintPure, Category="NPC|Perception")
    int32 GetPerceivedFoodCount() const;

    /** Nearest currently-perceived valid food to FromLocation, or nullptr if none. Drives BP FindFood. */
    UFUNCTION(BlueprintPure, Category="NPC|Perception")
    AItemBase* GetNearestPerceivedFood(const FVector& FromLocation) const;

    /**
     * Fired when a previously-seen food leaves perception — living-but-out-of-range
     * (LoseSightRadius) reported by a loss update. EMPTY for now: krok 2 (NPC memory,
     * L3-MEM) will subscribe to copy the fact into memory. Zero cost today.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="NPC|Perception")
    void OnFoodPerceptionLost();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    /** Bound to AIPerception_Sight.OnTargetPerceptionUpdated. Gain → AddUnique; loss → Remove + OnFoodPerceptionLost. */
    UFUNCTION()
    void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    /** Drops null (destroyed) weak-ptrs. Event-driven housekeeping — no tick, no brute scan. */
    void CompactPerceivedFood();

    /** Currently-perceived food. Weak → auto-nulls on destroy. Gain adds, loss removes. */
    UPROPERTY()
    TArray<TWeakObjectPtr<AItemBase>> PerceivedFood;

    /** The controller's perception component (AIPerception_Sight). Cached for bind/unbind. */
    UPROPERTY()
    TWeakObjectPtr<UAIPerceptionComponent> CachedPerception;

    /** Tag that marks a world actor as food (set in BP on food actors). */
    static const FName FoodTag;
};
