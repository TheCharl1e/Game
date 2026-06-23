#include "BTTask_Flee.h"
#include "MaslowBiologicalComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"

UBTTask_Flee::UBTTask_Flee()
{
    NodeName = TEXT("Flee (away from threat)");
}

UMaslowBiologicalComponent* UBTTask_Flee::GetMaslow(UBehaviorTreeComponent& OwnerComp)
{
    AAIController* AC = OwnerComp.GetAIOwner();
    if (!IsValid(AC))
    {
        return nullptr;
    }
    APawn* Pawn = AC->GetPawn();
    if (!IsValid(Pawn))
    {
        return nullptr;
    }
    return Pawn->FindComponentByClass<UMaslowBiologicalComponent>();
}

EBTNodeResult::Type UBTTask_Flee::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UMaslowBiologicalComponent* Maslow = GetMaslow(OwnerComp);
    AAIController*              AC     = OwnerComp.GetAIOwner();
    UBlackboardComponent*      BB     = OwnerComp.GetBlackboardComponent();
    if (!IsValid(Maslow) || !IsValid(AC) || !IsValid(BB))
    {
        return EBTNodeResult::Failed;
    }
    APawn* Pawn = AC->GetPawn();
    if (!IsValid(Pawn))
    {
        return EBTNodeResult::Failed;
    }

    const FVector PawnLoc   = Pawn->GetActorLocation();
    const FVector ThreatLoc = Maslow->GetThreatLocation();

    // Kierunek "precz" = od zagrożenia do nas, w płaszczyźnie poziomej.
    FVector AwayDir = PawnLoc - ThreatLoc;
    AwayDir.Z = 0.0f;
    if (!AwayDir.Normalize())
    {
        // Zagrożenie dokładnie pod nami / brak pozycji → losowy poziomy kierunek (i tak uciekamy).
        const float Ang = FMath::FRandRange(0.0f, 2.0f * PI);
        AwayDir = FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.0f);
    }
    const FVector Desired = PawnLoc + AwayDir * FleeDistance;

    // Projekcja na navmesh — MoveTo potrzebuje osiągalnego celu.
    FVector Dest = Desired;
    if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(Pawn->GetWorld()))
    {
        FNavLocation Proj;
        if (Nav->ProjectPointToNavigation(Desired, Proj, FVector(600.0f, 600.0f, 600.0f)))
        {
            Dest = Proj.Location;
        }
        else if (Nav->GetRandomReachablePointInRadius(PawnLoc, FleeDistance, Proj))
        {
            Dest = Proj.Location;   // fallback: jakikolwiek osiągalny punkt "dalej"
        }
    }

    BB->SetValueAsVector(FleeLocationKey, Dest);
    Maslow->OnPanicFlee();          // wizual paniki/ucieczki (BP)
    return EBTNodeResult::Succeeded; // następujący MoveTo(FleeLocation) wykona ruch
}

FString UBTTask_Flee::GetStaticDescription() const
{
    return FString::Printf(TEXT("Flee: '%s' = pawn + away-from-threat * %.0f (nav-projected)"),
        *FleeLocationKey.ToString(), FleeDistance);
}
