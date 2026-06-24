#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ContractTypes.h"
#include "ContractPoolSubsystem.generated.h"

class UInventoryComponent;

/**
 * L3-05 P2P slice 1 (Tier 1, virtual): the "virtual pool" of open barter contracts. One per world
 * (pattern like UNPCRegistrySubsystem). NPCs (UContractTraderComponent) post offers and accept matching
 * ones; AcceptAndFulfill does an INSTANT inventory swap (no movement — physical meeting = slice 2).
 *
 * Resolves the OwnerID/NPC link via UNPCRegistrySubsystem (P2P is its first hard consumer of GetNPCById).
 * Global pool for slice 1 (everyone sees everything); zone-scoped = later.
 */
UCLASS()
class STAN_PIERWOTNY_API UContractPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Post a barter offer. Returns the new ContractId (0 on failure). */
    int32 PostContract(int32 PosterID, FName OfferItem, int32 OfferAmount, EItemType WantType, int32 WantAmount);

    /** Cancel a contract (poster withdrew / goods gone). Idempotent. */
    void CancelContract(int32 ContractId);

    /** Snapshot of all Open contracts (for traders to scan). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "P2P")
    TArray<FContract> GetOpenContracts() const;

    /** Does this poster already have an Open contract? (avoid duplicate posts). */
    bool HasOpenContractFrom(int32 PosterID) const;

    /**
     * Accepter fulfills a contract: validates poster still holds the offer + accepter holds an item of the
     * wanted type, then does the VIRTUAL swap (RemoveItem+AddItem both ways) and marks it Fulfilled.
     * Returns true on a completed trade. Self-cancels the contract if the poster no longer has the goods.
     */
    bool AcceptAndFulfill(int32 ContractId, int32 AccepterID);

    /** Live contract count (debug/telemetry). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "P2P")
    int32 GetContractCount() const { return Contracts.Num(); }

private:
    /** Resolve an NPC's inventory by registry ID (via UNPCRegistrySubsystem). nullptr if unknown/dead. */
    UInventoryComponent* GetInventoryFor(int32 NPCId) const;

    UPROPERTY()
    TMap<int32, FContract> Contracts;

    int32 NextContractId = 1;   // never recycled
};
