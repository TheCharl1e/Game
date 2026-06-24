#include "ContractPoolSubsystem.h"
#include "InventoryComponent.h"
#include "NPC/NPCRegistrySubsystem.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogP2P, Log, All);

UInventoryComponent* UContractPoolSubsystem::GetInventoryFor(int32 NPCId) const
{
    if (NPCId == 0)
    {
        return nullptr;
    }
    UNPCRegistrySubsystem* Registry = GetWorld() ? GetWorld()->GetSubsystem<UNPCRegistrySubsystem>() : nullptr;
    if (!Registry)
    {
        return nullptr;
    }
    ACharacter* NPC = Registry->GetNPCById(NPCId);
    return NPC ? NPC->FindComponentByClass<UInventoryComponent>() : nullptr;
}

int32 UContractPoolSubsystem::PostContract(int32 PosterID, FName OfferItem, int32 OfferAmount, EItemType WantType, int32 WantAmount)
{
    if (PosterID == 0 || OfferItem.IsNone() || OfferAmount <= 0 || WantType == EItemType::None || WantAmount <= 0)
    {
        return 0;
    }

    // Validate the poster actually holds what it offers (no phantom listings).
    UInventoryComponent* PosterInv = GetInventoryFor(PosterID);
    if (!PosterInv || !PosterInv->HasItem(OfferItem, OfferAmount))
    {
        return 0;
    }

    FContract NewContract;
    NewContract.ContractId  = NextContractId++;
    NewContract.PosterID    = PosterID;
    NewContract.OfferItem   = OfferItem;
    NewContract.OfferAmount = OfferAmount;
    NewContract.WantType    = WantType;
    NewContract.WantAmount  = WantAmount;
    NewContract.Status      = EContractStatus::Open;
    NewContract.AccepterID  = 0;

    Contracts.Add(NewContract.ContractId, NewContract);

    UE_LOG(LogP2P, Log, TEXT("[P2P] PostContract #%d: NPC %d offers %d×%s, wants %d×%s"),
        NewContract.ContractId, PosterID, OfferAmount, *OfferItem.ToString(),
        WantAmount, *UEnum::GetValueAsString(WantType));

    return NewContract.ContractId;
}

void UContractPoolSubsystem::CancelContract(int32 ContractId)
{
    if (FContract* C = Contracts.Find(ContractId))
    {
        if (C->Status == EContractStatus::Open)
        {
            C->Status = EContractStatus::Cancelled;
            UE_LOG(LogP2P, Log, TEXT("[P2P] CancelContract #%d (poster %d)"), ContractId, C->PosterID);
        }
    }
}

TArray<FContract> UContractPoolSubsystem::GetOpenContracts() const
{
    TArray<FContract> Open;
    for (const TPair<int32, FContract>& Pair : Contracts)
    {
        if (Pair.Value.Status == EContractStatus::Open)
        {
            Open.Add(Pair.Value);
        }
    }
    return Open;
}

bool UContractPoolSubsystem::HasOpenContractFrom(int32 PosterID) const
{
    for (const TPair<int32, FContract>& Pair : Contracts)
    {
        if (Pair.Value.Status == EContractStatus::Open && Pair.Value.PosterID == PosterID)
        {
            return true;
        }
    }
    return false;
}

bool UContractPoolSubsystem::AcceptAndFulfill(int32 ContractId, int32 AccepterID)
{
    FContract* C = Contracts.Find(ContractId);
    if (!C || C->Status != EContractStatus::Open)
    {
        return false;
    }
    if (AccepterID == 0 || AccepterID == C->PosterID)
    {
        return false;   // can't trade with yourself
    }

    UInventoryComponent* PosterInv   = GetInventoryFor(C->PosterID);
    UInventoryComponent* AccepterInv = GetInventoryFor(AccepterID);

    // Poster gone / no longer holds the goods → self-cancel a dead listing.
    if (!PosterInv || !PosterInv->HasItem(C->OfferItem, C->OfferAmount))
    {
        C->Status = EContractStatus::Cancelled;
        UE_LOG(LogP2P, Log, TEXT("[P2P] Contract #%d self-cancelled (poster %d goods gone)"), ContractId, C->PosterID);
        return false;
    }
    if (!AccepterInv)
    {
        return false;
    }

    // Accepter must hold SOMETHING of the wanted category to give in return.
    const FName CounterItem = AccepterInv->FindFirstHeldItemOfType(C->WantType, C->WantAmount);
    if (CounterItem.IsNone())
    {
        return false;
    }

    // --- Virtual swap: pull both sides, then deposit (no movement; Tier 1). ---
    const int32 PulledOffer   = PosterInv->RemoveItem(C->OfferItem, C->OfferAmount);
    const int32 PulledCounter = AccepterInv->RemoveItem(CounterItem, C->WantAmount);

    if (PulledOffer < C->OfferAmount || PulledCounter < C->WantAmount)
    {
        // Roll back whatever we pulled — partial removal must never destroy goods.
        if (PulledOffer > 0)   { PosterInv->AddItem(C->OfferItem, PulledOffer); }
        if (PulledCounter > 0) { AccepterInv->AddItem(CounterItem, PulledCounter); }
        UE_LOG(LogP2P, Warning, TEXT("[P2P] Contract #%d swap aborted+rolled back (offer %d/%d, counter %d/%d)"),
            ContractId, PulledOffer, C->OfferAmount, PulledCounter, C->WantAmount);
        return false;
    }

    AccepterInv->AddItem(C->OfferItem, PulledOffer);     // accepter receives the offer
    PosterInv->AddItem(CounterItem, PulledCounter);      // poster receives the counter-good

    C->Status     = EContractStatus::Fulfilled;
    C->AccepterID = AccepterID;

    UE_LOG(LogP2P, Log, TEXT("[P2P] FULFILLED #%d: NPC %d gave %d×%s → NPC %d gave %d×%s"),
        ContractId, C->PosterID, C->OfferAmount, *C->OfferItem.ToString(),
        AccepterID, C->WantAmount, *CounterItem.ToString());

    return true;
}
