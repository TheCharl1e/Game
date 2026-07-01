// BTTask_RollWanderDirection.h
// L0 Track A / Slice 1 — WANDER step (brief §4). Rolls a random heading for a homeless NPC and writes a
// nav-projected wander destination to a Blackboard Vector key (a following MoveTo runs there).
//
// Sea = barrier, layer 2: a heading whose destination lands in the Ocean biome is REJECTED and re-rolled
// (navmesh not generating over water is layer 1). Falls back to any reachable point if every roll is bad.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RollWanderDirection.generated.h"

UCLASS()
class STAN_PIERWOTNY_API UBTTask_RollWanderDirection : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RollWanderDirection();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	/** How far (uu) to aim the wander destination from the pawn. [tune] */
	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderDistance = 3000.f;

	/** How many headings to roll before giving up on the biome/nav test and using a fallback. */
	UPROPERTY(EditAnywhere, Category = "Wander")
	int32 MaxRolls = 6;

	/** Blackboard Vector key to write the wander destination into (read by the following MoveTo). */
	UPROPERTY(EditAnywhere, Category = "Wander")
	FName MoveTargetKey = TEXT("MoveTarget");
};
