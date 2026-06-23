#include "NPCPerceptionComponent.h"
#include "ItemBase.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"

DEFINE_LOG_CATEGORY(LogNPCPerception);

const FName UNPCPerceptionComponent::FoodTag(TEXT("Food"));

UNPCPerceptionComponent::UNPCPerceptionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;   // project rule: NO Event Tick
}

void UNPCPerceptionComponent::BeginPlay()
{
    Super::BeginPlay();

    // We live on the AIController. Resolve its perception component (AIPerception_Sight).
    UAIPerceptionComponent* Perception = nullptr;
    if (AAIController* AICon = Cast<AAIController>(GetOwner()))
    {
        Perception = AICon->GetPerceptionComponent();
    }
    // Fallback: a BP-added AIPerceptionComponent may not be registered as the controller's
    // canonical PerceptionComponent — find it by class on the owner regardless.
    if (!IsValid(Perception) && IsValid(GetOwner()))
    {
        Perception = GetOwner()->FindComponentByClass<UAIPerceptionComponent>();
    }

    if (!IsValid(Perception))
    {
        UE_LOG(LogNPCPerception, Warning,
               TEXT("[Perception] %s: no AIPerceptionComponent on owner '%s' — food perception INERT."),
               *GetNameSafe(this), *GetNameSafe(GetOwner()));
        return;
    }

    CachedPerception = Perception;
    Perception->OnTargetPerceptionUpdated.AddDynamic(this, &UNPCPerceptionComponent::HandlePerceptionUpdated);

    UE_LOG(LogNPCPerception, Log,
           TEXT("[Perception] %s: bound to AIPerception on '%s' (food perception LIVE; MaxAge stays 0)."),
           *GetNameSafe(this), *GetNameSafe(GetOwner()));
}

void UNPCPerceptionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UAIPerceptionComponent* Perception = CachedPerception.Get())
    {
        Perception->OnTargetPerceptionUpdated.RemoveDynamic(this, &UNPCPerceptionComponent::HandlePerceptionUpdated);
    }
    CachedPerception = nullptr;
    PerceivedFood.Reset();
    Super::EndPlay(Reason);
}

void UNPCPerceptionComponent::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    // Opportunistic housekeeping: drop any food destroyed since the last event (no tick, no scan).
    CompactPerceivedFood();

    AItemBase* Food = Cast<AItemBase>(Actor);
    if (!IsValid(Food) || !Actor->ActorHasTag(FoodTag))
    {
        return;   // not food — Drink/Shelter handled by the BP handler (TECH-11b), out of scope here
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        // GAIN: now visible. AddUnique → no duplicates (replaces the BP monotonic counter).
        PerceivedFood.AddUnique(Food);
        UE_LOG(LogNPCPerception, Verbose, TEXT("[Perception] %s GAINED food '%s' (count=%d)."),
               *GetNameSafe(GetOwner()), *GetNameSafe(Food), GetPerceivedFoodCount());
    }
    else
    {
        // LOSS: living-but-out-of-range (LoseSightRadius). Remove + notify (hook for memory, krok 2).
        if (PerceivedFood.RemoveSingle(Food) > 0)
        {
            UE_LOG(LogNPCPerception, Verbose, TEXT("[Perception] %s LOST food '%s' (count=%d)."),
                   *GetNameSafe(GetOwner()), *GetNameSafe(Food), GetPerceivedFoodCount());
            OnFoodPerceptionLost();   // BlueprintImplementableEvent, empty for now
        }
    }
}

void UNPCPerceptionComponent::CompactPerceivedFood()
{
    PerceivedFood.RemoveAll([](const TWeakObjectPtr<AItemBase>& Ptr) { return !Ptr.IsValid(); });
}

int32 UNPCPerceptionComponent::GetPerceivedFoodCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AItemBase>& Ptr : PerceivedFood)
    {
        if (Ptr.IsValid()) { ++Count; }   // only valid → counter never lies structurally (TECH-11 fix)
    }
    return Count;
}

AItemBase* UNPCPerceptionComponent::GetNearestPerceivedFood(const FVector& FromLocation) const
{
    AItemBase* Nearest = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();
    for (const TWeakObjectPtr<AItemBase>& Ptr : PerceivedFood)
    {
        AItemBase* Food = Ptr.Get();
        if (!IsValid(Food)) { continue; }
        const float DistSq = static_cast<float>(FVector::DistSquared(FromLocation, Food->GetActorLocation()));
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Nearest = Food;
        }
    }
    return Nearest;
}
