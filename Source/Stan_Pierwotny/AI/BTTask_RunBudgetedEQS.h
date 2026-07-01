// BTTask_RunBudgetedEQS.h
// L0 Track A / Slice 1 — runs an EQS query THROUGH UEQSBudgetManager instead of firing it directly, so
// brief §3 ("500 NPC never fire EQS directly — they go through the budget") holds for every query in the
// exploration machine. This is the BT node that drives EQS_FindSafeZone and EQS_FindResource.
//
// Latent: returns InProgress, then writes the best result to a Blackboard key and finishes when the budget
// manager completes (or fails) the query. Instanced (bCreateNodeInstance) so each running NPC keeps its own
// in-flight handle. Cancel-on-abort releases the budget slot (NPC interrupted / dies while waiting).
//
// Result routing: set ResultObjectKey to capture an actor item (Actors-of-Class queries) OR ResultVectorKey
// to capture a location item (point queries, incl. the affordance generator). One of the two should be set.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BTTask_RunBudgetedEQS.generated.h"

class UEnvQuery;

UCLASS()
class STAN_PIERWOTNY_API UBTTask_RunBudgetedEQS : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RunBudgetedEQS();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	/** The EQS asset to run (e.g. EQS_FindSafeZone, EQS_FindResource). */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> QueryTemplate = nullptr;

	/** EQS run mode (SingleResult is the usual pick-the-best). */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	/** If set, write the best item's location into this Blackboard Vector key. */
	UPROPERTY(EditAnywhere, Category = "EQS")
	FName ResultVectorKey = NAME_None;

	/** If set, write the best item's actor into this Blackboard Object key. */
	UPROPERTY(EditAnywhere, Category = "EQS")
	FName ResultObjectKey = NAME_None;

private:
	/** Budget handle for the in-flight query (for AbortTask). INDEX_NONE when idle. */
	int32 BudgetHandle = INDEX_NONE;

	/** The owning BT component, cached so the async callback can finish the latent task. */
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	/** Budget-manager completion callback: write result to the blackboard and finish the task. */
	void HandleQueryResult(TSharedPtr<FEnvQueryResult> Result);
};
