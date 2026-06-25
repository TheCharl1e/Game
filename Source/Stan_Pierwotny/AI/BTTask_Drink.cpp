// BTTask_Drink.cpp
#include "BTTask_Drink.h"
#include "AffordanceType.h"
#include "WorldAffordanceSubsystem.h"
#include "MaslowBiologicalComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UBTTask_Drink::UBTTask_Drink()
{
	NodeName = TEXT("Drink (consume Hydration)");
}

EBTNodeResult::Type UBTTask_Drink::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AC = OwnerComp.GetAIOwner();
	if (!IsValid(AC)) { return EBTNodeResult::Failed; }
	APawn* Pawn = AC->GetPawn();
	if (!IsValid(Pawn)) { return EBTNodeResult::Failed; }

	UWorld* World = Pawn->GetWorld();
	UWorldAffordanceSubsystem* Affordances = World ? World->GetSubsystem<UWorldAffordanceSubsystem>() : nullptr;
	UMaslowBiologicalComponent* Maslow = Pawn->FindComponentByClass<UMaslowBiologicalComponent>();
	if (!Affordances || !IsValid(Maslow)) { return EBTNodeResult::Failed; }

	const FVector PawnLoc = Pawn->GetActorLocation();
	const int32 Id = Affordances->QueryNearest(EAffordanceType::Hydration, PawnLoc, ConsumeRadius);
	if (Id == INDEX_NONE) { return EBTNodeResult::Failed; }

	if (!Affordances->TryReserve(Id, Pawn)) { return EBTNodeResult::Failed; }

	float Granted = 0.f;
	const bool bConsumed = Affordances->Consume(Id, MaxConsumePerVisit, Granted);
	Affordances->Release(Id, Pawn);

	if (!bConsumed) { return EBTNodeResult::Failed; }

	// Feed the Thirst axis. CurrentHydration is the public 0..100 vital the metabolism judge reads.
	Maslow->CurrentHydration = FMath::Clamp(Maslow->CurrentHydration + Granted, 0.f, 100.f);
	UE_LOG(LogWorldAffordance, Log, TEXT("[Affordance] %s drank id=%d granted=%.1f (Hydration now %.1f)."),
		*Pawn->GetName(), Id, Granted, Maslow->CurrentHydration);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_Drink::GetStaticDescription() const
{
	return FString::Printf(TEXT("Drink nearest Hydration within %.0f uu (take ≤ %.0f -> hydration)"),
		ConsumeRadius, MaxConsumePerVisit);
}
