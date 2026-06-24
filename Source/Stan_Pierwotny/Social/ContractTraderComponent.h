#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.h"   // EItemType
#include "ContractTraderComponent.generated.h"

class UContractPoolSubsystem;
class UInventoryComponent;
class UMaslowBiologicalComponent;
class UNPCIdentityComponent;

/**
 * L3-05 P2P slice 1 (Tier 1, virtual) DRIVER. One per NPC. On a slow timer it:
 *   1. reads its Maslow needs → wanted item CATEGORIES (hunger→Food, cold→Clothing),
 *   2. POSTS a contract if it wants something and holds a surplus of another type (and has no open one),
 *   3. SCANS the global pool and ACCEPTS a contract whose offered item is something it wants AND
 *      it can pay with an item of that contract's wanted type — a true double-coincidence-of-wants.
 *
 * No BT, no movement — fulfillment is an instant virtual swap in the pool (physical meeting = slice 2).
 *
 * PERF (500+ NPC): the accept scan is O(open contracts) per trader per cadence — acceptable at slice-1
 * scale (handful of traders, slow cadence). Zone-scoped pool + event-driven matching = slice 2 (flagged).
 */
UCLASS(ClassGroup = (NPC), meta = (BlueprintSpawnableComponent))
class STAN_PIERWOTNY_API UContractTraderComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UContractTraderComponent();

    /** Seconds between trade evaluations (real time). Slow on purpose — bartering is not per-frame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "P2P|Trade")
    float TradeInterval = 5.0f;

    /** Master switch — lets an NPC opt out of bartering (e.g. hostile/feral). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "P2P|Trade")
    bool bTradingEnabled = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    /** Timer callback: post (if needed) then try to accept a matching contract. */
    UFUNCTION()
    void EvaluateTrade();

    FTimerHandle TradeTimerHandle;

private:
    /** Item categories this NPC currently wants, highest-priority first (cold before hunger, per Maslow). */
    void GetDesiredTypes(TArray<EItemType>& OutTypes) const;

    /** Resolve the contract pool for this world (nullptr if unavailable). */
    UContractPoolSubsystem* GetPool() const;

    // Lazy sibling caches (no FindComponentByClass per cadence; weak so a dying NPC self-nulls).
    TWeakObjectPtr<UInventoryComponent>        CachedInventory;
    TWeakObjectPtr<UMaslowBiologicalComponent> CachedMaslow;
    TWeakObjectPtr<UNPCIdentityComponent>      CachedIdentity;

    UInventoryComponent*        ResolveInventory();
    UMaslowBiologicalComponent* ResolveMaslow();
    UNPCIdentityComponent*      ResolveIdentity();
};
