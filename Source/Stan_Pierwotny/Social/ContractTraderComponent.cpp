#include "ContractTraderComponent.h"
#include "ContractPoolSubsystem.h"
#include "ContractTypes.h"
#include "InventoryComponent.h"
#include "MaslowBiologicalComponent.h"
#include "NPC/NPCIdentityComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogP2PTrader, Log, All);

UContractTraderComponent::UContractTraderComponent()
{
    PrimaryComponentTick.bCanEverTick = false;   // timer-driven, no Tick (project rule)
}

void UContractTraderComponent::BeginPlay()
{
    Super::BeginPlay();

    // Stagger across NPCs would need per-instance offset; slice 1 keeps a plain repeating timer.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TradeTimerHandle, this, &UContractTraderComponent::EvaluateTrade, TradeInterval, true);
    }
}

void UContractTraderComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TradeTimerHandle);
    }
    Super::EndPlay(Reason);
}

UInventoryComponent* UContractTraderComponent::ResolveInventory()
{
    if (!CachedInventory.IsValid())
    {
        CachedInventory = GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
    }
    return CachedInventory.Get();
}

UMaslowBiologicalComponent* UContractTraderComponent::ResolveMaslow()
{
    if (!CachedMaslow.IsValid())
    {
        CachedMaslow = GetOwner() ? GetOwner()->FindComponentByClass<UMaslowBiologicalComponent>() : nullptr;
    }
    return CachedMaslow.Get();
}

UNPCIdentityComponent* UContractTraderComponent::ResolveIdentity()
{
    if (!CachedIdentity.IsValid())
    {
        CachedIdentity = GetOwner() ? GetOwner()->FindComponentByClass<UNPCIdentityComponent>() : nullptr;
    }
    return CachedIdentity.Get();
}

UContractPoolSubsystem* UContractTraderComponent::GetPool() const
{
    return GetWorld() ? GetWorld()->GetSubsystem<UContractPoolSubsystem>() : nullptr;
}

void UContractTraderComponent::GetDesiredTypes(TArray<EItemType>& OutTypes) const
{
    OutTypes.Reset();
    UMaslowBiologicalComponent* Maslow = const_cast<UContractTraderComponent*>(this)->ResolveMaslow();
    if (!Maslow)
    {
        return;
    }

    // Maslow priority: temperature (Clothing) sits ABOVE hunger (Food). Same order as EvaluateCurrentNeed.
    if (Maslow->CurrentTemp < Maslow->ComfortTempMin)
    {
        OutTypes.Add(EItemType::Clothing);
    }
    if (Maslow->Glucose <= Maslow->EffectiveKcalThreshold)
    {
        OutTypes.Add(EItemType::Food);
    }
}

void UContractTraderComponent::EvaluateTrade()
{
    if (!bTradingEnabled)
    {
        return;
    }

    UContractPoolSubsystem* Pool = GetPool();
    UInventoryComponent*    Inv  = ResolveInventory();
    UNPCIdentityComponent*  Id   = ResolveIdentity();
    if (!Pool || !Inv || !Id)
    {
        return;
    }

    const int32 MyId = Id->GetNPCId();
    if (MyId == 0)
    {
        return;   // not yet registered
    }

    TArray<EItemType> Wants;
    GetDesiredTypes(Wants);
    if (Wants.Num() == 0)
    {
        return;   // content NPC — no barter drive this cadence
    }

    // --- 1) ACCEPT first: satisfying a need now beats waiting on my own listing. ---------------
    for (const FContract& C : Pool->GetOpenContracts())
    {
        if (C.PosterID == MyId)
        {
            continue;
        }
        const EItemType OfferedType = Inv->GetItemTypeForID(C.OfferItem);
        if (!Wants.Contains(OfferedType))
        {
            continue;   // they offer something I don't need
        }
        // I must be able to PAY: hold an item of the category they want.
        if (Inv->FindFirstHeldItemOfType(C.WantType, C.WantAmount).IsNone())
        {
            continue;
        }
        if (Pool->AcceptAndFulfill(C.ContractId, MyId))
        {
            UE_LOG(LogP2PTrader, Log, TEXT("[P2P] NPC %d accepted contract #%d"), MyId, C.ContractId);
            return;   // one trade per cadence
        }
    }

    // --- 2) POST: I still want something — list a surplus to attract a counterpart. -------------
    if (Pool->HasOpenContractFrom(MyId))
    {
        return;   // already advertising
    }

    const EItemType TopWant = Wants[0];
    // Offer a held item that is NOT of a type I want (a genuine surplus, not my own lifeline).
    FName OfferItem = Inv->FindFirstHeldItemNotOfType(TopWant);
    // If both wants exist, also avoid offering the secondary want away.
    if (!OfferItem.IsNone() && Wants.Num() > 1)
    {
        const EItemType OfferType = Inv->GetItemTypeForID(OfferItem);
        if (Wants.Contains(OfferType))
        {
            OfferItem = NAME_None;   // the only surplus is itself a want → don't trade it away
        }
    }
    if (OfferItem.IsNone())
    {
        return;   // nothing to give
    }

    const int32 NewId = Pool->PostContract(MyId, OfferItem, 1, TopWant, 1);
    if (NewId != 0)
    {
        UE_LOG(LogP2PTrader, Log, TEXT("[P2P] NPC %d posted contract #%d (offer %s, want %s)"),
            MyId, NewId, *OfferItem.ToString(), *UEnum::GetValueAsString(TopWant));
    }
}
