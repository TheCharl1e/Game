// EQSBudgetManager.h
// L0 Track A / Slice 1 — the religion of performance (brief §3).
//
// 500 NPCs NEVER fire EQS directly — they go through this budget. Load is bounded by MaxConcurrent
// (a fixed number of in-flight queries), NOT by the NPC count. Requests over budget queue and dispatch
// as slots free. Edge-triggered callers + a per-life HomeZone cache keep the request rate low to begin
// with; this manager is the hard ceiling on top of that.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnvironmentQuery/EnvQueryTypes.h"   // EEnvQueryRunMode, FQueryFinishedSignature, FEnvQueryResult
#include "EQSBudgetManager.generated.h"

class UEnvQuery;

// Brief §7 — EQS budget log category (queue depth / dispatch / cancel). Defined in EQSBudgetManager.cpp.
DECLARE_LOG_CATEGORY_EXTERN(LogEQSBudget, Log, All);

/**
 * World throttle for EQS. One per UWorld. BT tasks call RequestQuery instead of running EQS themselves.
 */
UCLASS()
class STAN_PIERWOTNY_API UEQSBudgetManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Queue an EQS run. Returns a stable handle (>0) for CancelQuery, or INDEX_NONE on bad input.
	 *  OnDone fires with the result when the query completes (skipped if Requester died meanwhile). */
	int32 RequestQuery(AActor* Requester, UEnvQuery* Query, EEnvQueryRunMode::Type Mode,
		FQueryFinishedSignature OnDone);

	/** Fail-safe: drop a queued query or abort an in-flight one (NPC dies while waiting). */
	void CancelQuery(int32 Handle);

	/** Max EQS runs in flight at once. THE knob that makes load scale with budget, not NPC count. */
	UPROPERTY(EditAnywhere, Category = "Budget")
	int32 MaxConcurrent = 12;

	/** Seconds between reclaim sweeps. Belt-and-suspenders: an in-flight slot whose requester died and whose
	 *  EQS never fired its finish delegate would leak permanently (no lazy recovery) and, after MaxConcurrent
	 *  deaths, starve exploration. The sweep aborts + frees such slots deterministically. [TBD→tune] */
	UPROPERTY(EditAnywhere, Category = "Budget")
	float ReclaimInterval = 5.f;

private:
	struct FPendingQuery
	{
		int32 Handle = INDEX_NONE;
		TWeakObjectPtr<AActor> Requester;
		TWeakObjectPtr<UEnvQuery> Query;
		EEnvQueryRunMode::Type Mode = EEnvQueryRunMode::SingleResult;
		FQueryFinishedSignature Callback;
	};

	struct FActiveQuery
	{
		int32 Handle = INDEX_NONE;
		int32 EqsQueryId = INDEX_NONE;     // UEnvQueryManager request id (for abort + result correlation)
		TWeakObjectPtr<AActor> Requester;
		FQueryFinishedSignature Callback;
	};

	TArray<FPendingQuery> Queue;
	TArray<FActiveQuery>  Active;
	int32 NextHandle = 1;
	FTimerHandle ReclaimTimerHandle;

	/** Pull from the queue while slots are free; drops requests whose requester already died. */
	void DispatchNext();
	/** Launch one EQS run through UEnvQueryManager. Returns true if it went in flight. */
	bool DispatchOne(const FPendingQuery& Pending);
	/** EQS completion callback: correlate by QueryID, forward to the caller, free the slot, dispatch next. */
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result);
	/** Timer sweep: abort + free any in-flight slot whose requester died (deterministic leak recovery). */
	void ReclaimDeadSlots();
};
