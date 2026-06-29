// BTTask_RollWanderDirection.cpp
#include "BTTask_RollWanderDirection.h"
#include "AffordanceType.h"            // LogExploration
#include "CaldrethZone.h"              // ACaldrethZone::GetZoneTypeAtLocation, EZoneType
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"

UBTTask_RollWanderDirection::UBTTask_RollWanderDirection()
{
	NodeName = TEXT("Roll Wander Direction (avoid sea)");
}

EBTNodeResult::Type UBTTask_RollWanderDirection::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!IsValid(AC) || !IsValid(BB)) { return EBTNodeResult::Failed; }
	APawn* Pawn = AC->GetPawn();
	if (!IsValid(Pawn)) { return EBTNodeResult::Failed; }

	const FVector PawnLoc = Pawn->GetActorLocation();
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());

	for (int32 Roll = 0; Roll < FMath::Max(1, MaxRolls); ++Roll)
	{
		const float Ang = FMath::FRandRange(0.f, 2.f * PI);
		const FVector Dir(FMath::Cos(Ang), FMath::Sin(Ang), 0.f);
		const FVector Desired = PawnLoc + Dir * WanderDistance;

		// Sea barrier (layer 2): reject a heading that lands in the Ocean biome.
		if (ACaldrethZone::GetZoneTypeAtLocation(Pawn, Desired) == EZoneType::Ocean)
		{
			continue;
		}

		// Nav-project so MoveTo gets a reachable goal (layer 1: no navmesh over water anyway).
		if (Nav)
		{
			FNavLocation Proj;
			if (Nav->ProjectPointToNavigation(Desired, Proj, FVector(600.f, 600.f, 600.f)))
			{
				BB->SetValueAsVector(MoveTargetKey, Proj.Location);
				UE_LOG(LogExploration, Verbose, TEXT("[Explore] %s wander roll %d -> (%.0f,%.0f) accepted."),
					*Pawn->GetName(), Roll, Proj.Location.X, Proj.Location.Y);
				return EBTNodeResult::Succeeded;
			}
		}
		else
		{
			// No nav system (test world) — accept the raw point.
			BB->SetValueAsVector(MoveTargetKey, Desired);
			return EBTNodeResult::Succeeded;
		}
	}

	// Every roll hit sea / off-nav — fall back to any reachable point so the NPC still moves.
	if (Nav)
	{
		FNavLocation Proj;
		if (Nav->GetRandomReachablePointInRadius(PawnLoc, WanderDistance, Proj))
		{
			BB->SetValueAsVector(MoveTargetKey, Proj.Location);
			UE_LOG(LogExploration, Verbose, TEXT("[Explore] %s wander fallback -> reachable point."), *Pawn->GetName());
			return EBTNodeResult::Succeeded;
		}
	}

	UE_LOG(LogExploration, Warning, TEXT("[Explore] %s wander failed (no sea-free reachable heading)."), *Pawn->GetName());
	return EBTNodeResult::Failed;
}

FString UBTTask_RollWanderDirection::GetStaticDescription() const
{
	return FString::Printf(TEXT("Roll wander: '%s' = pawn + random * %.0f (reject Ocean, nav-projected)"),
		*MoveTargetKey.ToString(), WanderDistance);
}
