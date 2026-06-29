#pragma once

#include "CoreMinimal.h"
#include "InventoryComponent.h"   // EItemType (WantType is a category, not a specific item)
#include "ContractTypes.generated.h"

/** L3-05 P2P: lifecycle of a barter contract in the virtual pool. */
UENUM(BlueprintType)
enum class EContractStatus : uint8
{
    Open       UMETA(DisplayName = "Open"),
    Fulfilled  UMETA(DisplayName = "Fulfilled"),
    Cancelled  UMETA(DisplayName = "Cancelled")
};

/**
 * L3-05 P2P: a barter offer in the virtual pool. The poster OFFERS a specific item it holds and WANTS
 * any item of a category (driven by its Maslow need: hunger→Food, cold→Clothing). Tier 1 = virtual:
 * fulfillment is an instant inventory swap, no movement (physical meeting = slice 2).
 */
USTRUCT(BlueprintType)
struct FContract
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Contract")
    int32 ContractId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Contract")
    int32 PosterID = 0;                       // NPC registry ID (UNPCIdentityComponent::GetNPCId)

    UPROPERTY(BlueprintReadOnly, Category = "Contract")
    FName OfferItem = NAME_None;              // specific item the poster gives

    UPROPERTY(BlueprintReadOnly, Category = "Contract")
    int32 OfferAmount = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Contract")
    EItemType WantType = EItemType::None;     // CATEGORY the poster wants (from its need)

    UPROPERTY(BlueprintReadOnly, Category = "Contract")
    int32 WantAmount = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Contract")
    EContractStatus Status = EContractStatus::Open;

    UPROPERTY(BlueprintReadOnly, Category = "Contract")
    int32 AccepterID = 0;                      // 0 = none yet
};
