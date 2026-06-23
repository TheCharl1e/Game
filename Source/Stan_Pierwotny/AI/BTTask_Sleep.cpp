#include "BTTask_Sleep.h"
#include "MaslowBiologicalComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

UBTTask_Sleep::UBTTask_Sleep()
{
    NodeName    = TEXT("Sleep (Maslow)");
    bNotifyTick = true;   // poll HoursAwake each tick until rested
}

UMaslowBiologicalComponent* UBTTask_Sleep::GetMaslow(UBehaviorTreeComponent& OwnerComp)
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

EBTNodeResult::Type UBTTask_Sleep::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UMaslowBiologicalComponent* Maslow = GetMaslow(OwnerComp);
    if (!IsValid(Maslow))
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_Sleep] No Maslow component on controlled pawn — failing."));
        return EBTNodeResult::Failed;
    }

    Maslow->StartSleep();                 // C++ owns the fatigue math; this only drives the hook
    return EBTNodeResult::InProgress;     // tick until rested
}

void UBTTask_Sleep::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    UMaslowBiologicalComponent* Maslow = GetMaslow(OwnerComp);
    if (!IsValid(Maslow))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Fully rested -> voluntary wake + finish. HoursAwake drains in UpdateFatigue (bIsSleeping branch).
    if (Maslow->HoursAwake <= WakeHoursAwake)
    {
        Maslow->StopSleep();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

EBTNodeResult::Type UBTTask_Sleep::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Branch interrupted mid-sleep — never leave the NPC stuck bIsSleeping.
    if (UMaslowBiologicalComponent* Maslow = GetMaslow(OwnerComp))
    {
        Maslow->StopSleep();
    }
    return EBTNodeResult::Aborted;
}

FString UBTTask_Sleep::GetStaticDescription() const
{
    return FString::Printf(TEXT("Voluntary sleep: StartSleep() -> drain HoursAwake -> StopSleep() at <= %.2f"),
        WakeHoursAwake);
}
