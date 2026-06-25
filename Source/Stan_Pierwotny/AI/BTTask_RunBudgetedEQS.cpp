// BTTask_RunBudgetedEQS.cpp
#include "BTTask_RunBudgetedEQS.h"
#include "EQSBudgetManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UBTTask_RunBudgetedEQS::UBTTask_RunBudgetedEQS()
{
	NodeName = TEXT("Run Budgeted EQS");
	// Per-NPC in-flight handle -> each running instance needs its own node object.
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_RunBudgetedEQS::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AC = OwnerComp.GetAIOwner();
	if (!IsValid(AC) || !IsValid(QueryTemplate)) { return EBTNodeResult::Failed; }
	APawn* Pawn = AC->GetPawn();
	if (!IsValid(Pawn)) { return EBTNodeResult::Failed; }

	UWorld* World = Pawn->GetWorld();
	UEQSBudgetManager* Budget = World ? World->GetSubsystem<UEQSBudgetManager>() : nullptr;
	if (!Budget) { return EBTNodeResult::Failed; }

	CachedOwnerComp = &OwnerComp;

	FQueryFinishedSignature OnDone =
		FQueryFinishedSignature::CreateUObject(this, &UBTTask_RunBudgetedEQS::HandleQueryResult);
	BudgetHandle = Budget->RequestQuery(Pawn, QueryTemplate, RunMode, OnDone);
	if (BudgetHandle == INDEX_NONE) { return EBTNodeResult::Failed; }

	return EBTNodeResult::InProgress;   // budget manager calls HandleQueryResult when done
}

EBTNodeResult::Type UBTTask_RunBudgetedEQS::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (BudgetHandle != INDEX_NONE)
	{
		if (AAIController* AC = OwnerComp.GetAIOwner())
		{
			if (APawn* Pawn = AC->GetPawn())
			{
				if (UWorld* World = Pawn->GetWorld())
				{
					if (UEQSBudgetManager* Budget = World->GetSubsystem<UEQSBudgetManager>())
					{
						Budget->CancelQuery(BudgetHandle);   // free the slot
					}
				}
			}
		}
		BudgetHandle = INDEX_NONE;
	}
	CachedOwnerComp = nullptr;
	return EBTNodeResult::Aborted;
}

void UBTTask_RunBudgetedEQS::HandleQueryResult(TSharedPtr<FEnvQueryResult> Result)
{
	UBehaviorTreeComponent* Comp = CachedOwnerComp.Get();
	BudgetHandle = INDEX_NONE;
	if (!IsValid(Comp)) { return; }   // tree torn down / NPC gone

	bool bOk = Result.IsValid() && Result->IsSuccessful();
	if (bOk)
	{
		if (UBlackboardComponent* BB = Comp->GetBlackboardComponent())
		{
			if (ResultObjectKey != NAME_None)
			{
				AActor* BestActor = Result->GetItemAsActor(0);
				if (IsValid(BestActor)) { BB->SetValueAsObject(ResultObjectKey, BestActor); }
				else { bOk = false; }
			}
			else if (ResultVectorKey != NAME_None)
			{
				BB->SetValueAsVector(ResultVectorKey, Result->GetItemAsLocation(0));
			}
		}
		else { bOk = false; }
	}

	FinishLatentTask(*Comp, bOk ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}

FString UBTTask_RunBudgetedEQS::GetStaticDescription() const
{
	const FString QName = QueryTemplate ? QueryTemplate->GetName() : TEXT("<none>");
	const FString Dest = (ResultObjectKey != NAME_None)
		? FString::Printf(TEXT("object '%s'"), *ResultObjectKey.ToString())
		: FString::Printf(TEXT("vector '%s'"), *ResultVectorKey.ToString());
	return FString::Printf(TEXT("Budgeted EQS '%s' -> %s"), *QName, *Dest);
}
