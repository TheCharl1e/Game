// BTTask_Eat.cpp
#include "BTTask_Eat.h"
#include "AffordanceType.h"
#include "WorldAffordanceSubsystem.h"
#include "MaslowBiologicalComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UBTTask_Eat::UBTTask_Eat()
{
	NodeName = TEXT("Eat (consume Nourishment)");
}

EBTNodeResult::Type UBTTask_Eat::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
	const int32 Id = Affordances->QueryNearest(EAffordanceType::Nourishment, PawnLoc, ConsumeRadius);
	if (Id == INDEX_NONE) { return EBTNodeResult::Failed; }     // gone / claimed while walking — re-forage

	if (!Affordances->TryReserve(Id, Pawn)) { return EBTNodeResult::Failed; } // someone else claimed it

	float Granted = 0.f;
	const bool bConsumed = Affordances->Consume(Id, MaxConsumePerVisit, Granted); // revalidates on arrival (§2)
	Affordances->Release(Id, Pawn);                                              // always release

	if (!bConsumed) { return EBTNodeResult::Failed; }

	// Berry = carbohydrate: route the granted yield as sugars -> Glucose, clearing the Hunger need.
	Maslow->ConsumeFood(/*Proteins*/ 0.f, /*Fats*/ 0.f, /*Sugars*/ Granted);
	UE_LOG(LogWorldAffordance, Log, TEXT("[Affordance] %s ate id=%d granted=%.1f (Glucose now %.1f)."),
		*Pawn->GetName(), Id, Granted, Maslow->Glucose);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_Eat::GetStaticDescription() const
{
	return FString::Printf(TEXT("Eat nearest Nourishment within %.0f uu (take ≤ %.0f -> sugars)"),
		ConsumeRadius, MaxConsumePerVisit);
}
