#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Sleep.generated.h"

class UMaslowBiologicalComponent;

/**
 * TASK 2 (sleep / plaster #3): VOLUNTARY sleep behavior. Runs inside Handle Sleep, selected when the
 * Blackboard decorator CurrentNeed==Sleep(3) passes (i.e. HoursAwake >= MentalFogThreshold, D1).
 *
 * ExecuteTask -> Maslow->StartSleep() (C++ owns the fatigue math). Ticks while HoursAwake drains
 * (UpdateFatigue, bIsSleeping branch); finishes Succeeded once fully rested (HoursAwake <= WakeHoursAwake)
 * -> Maslow->StopSleep() (Rested buff + OnWakeUp). Aborts cleanly (StopSleep) so the NPC is never left
 * stuck bIsSleeping if the branch is interrupted.
 *
 * INVOLUNTARY collapse recovery (HoursAwake >= CollapseThreshold while awake) is NOT this task — C++
 * owns it (UpdateFatigue auto-calls StopSleep once a collapsed NPC has slept it off). This task is the
 * VOLUNTARY path only; the BT is StopLogic'd during a collapse so it cannot run here.
 */
UCLASS()
class STAN_PIERWOTNY_API UBTTask_Sleep : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_Sleep();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual FString GetStaticDescription() const override;

    /** HoursAwake at/below which the NPC is fully rested and wakes (voluntary). [tune] */
    UPROPERTY(EditAnywhere, Category = "Sleep")
    float WakeHoursAwake = 0.01f;

private:
    /** Fetch the Maslow component from the controlled pawn (controller -> pawn -> component). */
    static UMaslowBiologicalComponent* GetMaslow(UBehaviorTreeComponent& OwnerComp);
};
