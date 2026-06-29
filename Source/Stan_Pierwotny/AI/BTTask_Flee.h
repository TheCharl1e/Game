#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Flee.generated.h"

class UMaslowBiologicalComponent;

/**
 * TASK 3 (flee / plaster #4): compute a "run AWAY from the threat" destination and write it to a Blackboard
 * Vector key, so a following BTTask_MoveTo can run there (same pattern as FindFood/FindWater → MoveTo).
 *
 * Destination = PawnLocation + normalize(PawnLocation - ThreatLocation) * FleeDistance, projected to the
 * navmesh (so MoveTo gets a reachable goal). ThreatLocation comes from UMaslowBiologicalComponent
 * (GetThreatLocation), which is set by the damage-hook (OnTakeAnyDamage → SetThreat) and, later, by
 * predator/hostile-human perception (L0-04) through the same SetThreat API.
 *
 * The Flee branch is gated by a Blackboard decorator CurrentNeed==Flee(4) with FlowAbortMode=LowerPriority,
 * so it INTERRUPTS a running lower-priority branch (eat/sleep/drink) the moment a threat appears. While the
 * threat is active the Selector keeps re-running this branch (new away-point each pass — flee a moving
 * threat); once the threat memory expires, CurrentNeed drops and the NPC resumes its previous need.
 */
UCLASS()
class STAN_PIERWOTNY_API UBTTask_Flee : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_Flee();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual FString GetStaticDescription() const override;

    /** How far (uu) past the pawn, directly away from the threat, to aim the flee point. [tune] */
    UPROPERTY(EditAnywhere, Category = "Flee")
    float FleeDistance = 1500.0f;

    /** Blackboard Vector key to write the flee destination into (read by the following MoveTo). */
    UPROPERTY(EditAnywhere, Category = "Flee")
    FName FleeLocationKey = TEXT("FleeLocation");

private:
    static UMaslowBiologicalComponent* GetMaslow(UBehaviorTreeComponent& OwnerComp);
};
