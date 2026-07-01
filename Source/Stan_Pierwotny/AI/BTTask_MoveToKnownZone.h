// BTTask_MoveToKnownZone.h
// L0 Track A / Slice 1 — EXPLORATION step (brief §4). If the NPC already remembers a Safe Zone
// (HomeZone object key valid), aim MoveTarget at it instead of wandering blind. Fails (so the tree falls
// through to WANDER) when there is no remembered zone or it was destroyed.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToKnownZone.generated.h"

UCLASS()
class STAN_PIERWOTNY_API UBTTask_MoveToKnownZone : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToKnownZone();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	/** Blackboard Object key holding the remembered ACaldrethZone (null = homeless). */
	UPROPERTY(EditAnywhere, Category = "Exploration")
	FName HomeZoneKey = TEXT("HomeZone");

	/** Blackboard Vector key to write the zone location into (read by the following MoveTo). */
	UPROPERTY(EditAnywhere, Category = "Exploration")
	FName MoveTargetKey = TEXT("MoveTarget");
};
