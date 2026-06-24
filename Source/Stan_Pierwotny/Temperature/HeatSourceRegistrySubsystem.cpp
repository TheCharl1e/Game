#include "HeatSourceRegistrySubsystem.h"
#include "HeatSourceComponent.h"
#include "GameFramework/Actor.h"

void UHeatSourceRegistrySubsystem::RegisterSource(UHeatSourceComponent* Source)
{
    if (IsValid(Source))
    {
        Sources.AddUnique(Source);
    }
}

void UHeatSourceRegistrySubsystem::UnregisterSource(UHeatSourceComponent* Source)
{
    Sources.RemoveAll([Source](const TWeakObjectPtr<UHeatSourceComponent>& P)
    {
        return !P.IsValid() || P.Get() == Source;
    });
}

float UHeatSourceRegistrySubsystem::GetRadiantHeatAt(const FVector& Location) const
{
    float Total = 0.0f;
    for (const TWeakObjectPtr<UHeatSourceComponent>& Ptr : Sources)
    {
        const UHeatSourceComponent* Src = Ptr.Get();
        if (!Src || !Src->bIsLit)
        {
            continue;   // zniszczone (weak null) / zgaszone → nie grzeje
        }
        const AActor* Owner = Src->GetOwner();
        if (!IsValid(Owner) || Src->Radius <= 0.0f)
        {
            continue;
        }
        const float Dist = FVector::Dist(Location, Owner->GetActorLocation());
        if (Dist >= Src->Radius)
        {
            continue;   // poza zasięgiem
        }
        // Huddle falloff: (1 − d/r)² — ciepło skupione przy ogniu, stromy spadek ku krawędzi.
        const float Falloff = FMath::Square(1.0f - Dist / Src->Radius);
        Total += Src->Warmth * Falloff;
    }
    // Clamp: suma wielu ognisk nie ugotuje NPC.
    return FMath::Min(Total, MaxRadiantHeat);
}
