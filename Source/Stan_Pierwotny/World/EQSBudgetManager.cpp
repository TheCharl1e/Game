// EQSBudgetManager.cpp
#include "EQSBudgetManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogEQSBudget);

void UEQSBudgetManager::Deinitialize()
{
	// Abort anything still in flight so no EQS completion fires into a torn-down manager.
	if (UWorld* World = GetWorld())
	{
		if (UEnvQueryManager* EQSMgr = UEnvQueryManager::GetCurrent(World))
		{
			for (const FActiveQuery& A : Active)
			{
				if (A.EqsQueryId != INDEX_NONE) { EQSMgr->AbortQuery(A.EqsQueryId); }
			}
		}
	}
	Active.Reset();
	Queue.Reset();
	Super::Deinitialize();
}

int32 UEQSBudgetManager::RequestQuery(AActor* Requester, UEnvQuery* Query, EEnvQueryRunMode::Type Mode,
	FQueryFinishedSignature OnDone)
{
	if (!IsValid(Requester) || !IsValid(Query))
	{
		UE_LOG(LogEQSBudget, Warning, TEXT("[EQSBudget] RequestQuery rejected (requester/query invalid)."));
		return INDEX_NONE;
	}

	FPendingQuery P;
	P.Handle = NextHandle++;
	P.Requester = Requester;
	P.Query = Query;
	P.Mode = Mode;
	P.Callback = OnDone;

	const int32 Handle = P.Handle;   // stable handle, whether it stays queued or dispatches immediately
	Queue.Add(MoveTemp(P));

	DispatchNext();   // launch immediately if a slot is free
	return Handle;
}

void UEQSBudgetManager::CancelQuery(int32 Handle)
{
	// Queued but not yet launched -> just drop it.
	const int32 Qi = Queue.IndexOfByPredicate([Handle](const FPendingQuery& P) { return P.Handle == Handle; });
	if (Qi != INDEX_NONE)
	{
		Queue.RemoveAt(Qi);
		UE_LOG(LogEQSBudget, Log, TEXT("[EQSBudget] Cancel queued handle=%d (queue depth=%d)."), Handle, Queue.Num());
		return;
	}

	// In flight -> abort the EQS run and free the slot.
	const int32 Ai = Active.IndexOfByPredicate([Handle](const FActiveQuery& A) { return A.Handle == Handle; });
	if (Ai != INDEX_NONE)
	{
		if (UWorld* World = GetWorld())
		{
			if (UEnvQueryManager* EQSMgr = UEnvQueryManager::GetCurrent(World))
			{
				EQSMgr->AbortQuery(Active[Ai].EqsQueryId);
			}
		}
		UE_LOG(LogEQSBudget, Warning, TEXT("[EQSBudget] Cancel in-flight handle=%d (query canceled by death/abort)."), Handle);
		Active.RemoveAtSwap(Ai, EAllowShrinking::No);
		DispatchNext();
	}
}

void UEQSBudgetManager::DispatchNext()
{
	while (Active.Num() < MaxConcurrent && Queue.Num() > 0)
	{
		FPendingQuery Pending = Queue[0];
		Queue.RemoveAt(0);

		// Cancel-on-death: requester gone while waiting -> drop silently, don't burn a slot.
		if (!Pending.Requester.IsValid() || !Pending.Query.IsValid())
		{
			UE_LOG(LogEQSBudget, Verbose, TEXT("[EQSBudget] Dropping handle=%d (requester/query gone before dispatch)."),
				Pending.Handle);
			continue;
		}

		if (!DispatchOne(Pending))
		{
			// Launch failed -> notify the caller of failure so its BT task doesn't hang.
			Pending.Callback.ExecuteIfBound(nullptr);
		}
	}
}

bool UEQSBudgetManager::DispatchOne(const FPendingQuery& Pending)
{
	UWorld* World = GetWorld();
	if (!World) { return false; }

	FEnvQueryRequest Request(Pending.Query.Get(), Pending.Requester.Get());
	const int32 EqsId = Request.Execute(Pending.Mode, this, &UEQSBudgetManager::OnQueryFinished);
	if (EqsId == INDEX_NONE) { return false; }

	FActiveQuery A;
	A.Handle = Pending.Handle;
	A.EqsQueryId = EqsId;
	A.Requester = Pending.Requester;
	A.Callback = Pending.Callback;
	Active.Add(MoveTemp(A));

	UE_LOG(LogEQSBudget, Log, TEXT("[EQSBudget] Dispatch handle=%d eqsId=%d (queue depth=%d, active=%d/%d)."),
		Pending.Handle, EqsId, Queue.Num(), Active.Num(), MaxConcurrent);
	return true;
}

void UEQSBudgetManager::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	const int32 EqsId = Result.IsValid() ? Result->QueryID : INDEX_NONE;
	const int32 Ai = Active.IndexOfByPredicate([EqsId](const FActiveQuery& A) { return A.EqsQueryId == EqsId; });
	if (Ai != INDEX_NONE)
	{
		FActiveQuery A = Active[Ai];
		Active.RemoveAtSwap(Ai, EAllowShrinking::No);

		// Skip callback if the requester died mid-flight (fail-safe — no write into a dead NPC's blackboard).
		if (A.Requester.IsValid())
		{
			A.Callback.ExecuteIfBound(Result);
		}
		UE_LOG(LogEQSBudget, Log, TEXT("[EQSBudget] Finished handle=%d (queue depth=%d, active=%d/%d)."),
			A.Handle, Queue.Num(), Active.Num(), MaxConcurrent);
	}
	DispatchNext();
}
