#include "ItemBase.h"
#include "Components/StaticMeshComponent.h"

AItemBase::AItemBase()
{
    // Optymalizacja! Jedzenie leżące na mapie nie potrzebuje aktualizować się co klatkę.
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Domyślnie każdy nowy przedmiot na ziemi lub w krzaku należy do ogółu/nikogo
    OwnerID = -1;
    OwnershipState = EItemOwnership::Public;
}

void AItemBase::BeginPlay()
{
    Super::BeginPlay();
}

void AItemBase::SetOwnership(EItemOwnership NewState, int32 InOwnerID)
{
    OwnershipState = NewState;
    OwnerID        = InOwnerID;
}

bool AItemBase::CanBeEatenBy(int32 RequesterID) const
{
    return (OwnershipState == EItemOwnership::Public) || (OwnerID == RequesterID);
}

bool AItemBase::IsStolenBy(int32 RequesterID) const
{
    // Theft = private item + requester is NOT the owner.
    // PlayerOwned items are always theft for any NPC.
    return (OwnershipState != EItemOwnership::Public) && (OwnerID != RequesterID);
}

float AItemBase::ConsumePortion(float RequestedPortion)
{
    // Mirror ACorpseBase::ExtractMeat — bierzemy tyle, ile zostało, jeśli prosimy o więcej.
    if (RemainingPortion <= 0.0f || RequestedPortion <= 0.0f)
    {
        return 0.0f;
    }

    const float Taken = FMath::Min(RequestedPortion, RemainingPortion);
    RemainingPortion -= Taken;

    // Zjedzone do zera → sygnał do Blueprinta (np. zniszcz aktora / zostaw ogryzek).
    if (RemainingPortion <= 0.0f)
    {
        RemainingPortion = 0.0f;
        OnItemDepleted();
    }

    return Taken;
}