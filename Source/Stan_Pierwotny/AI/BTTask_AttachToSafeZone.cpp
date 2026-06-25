// BTTask_AttachToSafeZone.cpp
#include "BTTask_AttachToSafeZone.h"
#include "AffordanceType.h"            // LogExploration
#include "CaldrethZone.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

UBTTask_AttachToSafeZone::UBTTask_AttachToSafeZone()
{
	NodeName = TEXT("Attach To Safe Zone (set HomeZone)");
}

EBTNodeResult::Type UBTTask_AttachToSafeZone::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!IsValid(AC) || !IsValid(BB)) { return EBTNodeResult::Failed; }
	APawn* Pawn = AC->GetPawn();
	if (!IsValid(Pawn)) { return EBTNodeResult::Failed; }

	const FVector ShelterPoint = BB->GetValueAsVector(FoundSafeZoneKey);
	ACaldrethZone* Zone = ACaldrethZone::GetZoneAtLocation(Pawn, ShelterPoint);
	if (!IsValid(Zone) || !Zone->bIsSafeZone)
	{
		UE_LOG(LogExploration, Warning, TEXT("[Explore] %s attach failed (no safe zone under shelter point)."),
			*Pawn->GetName());
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsObject(HomeZoneKey, Zone);
	Zone->RegisterMember(Pawn);
	UE_LOG(LogExploration, Log, TEXT("[Explore] %s attach -> HomeZone=%s (members=%d)."),
		*Pawn->GetName(), *Zone->GetName(), Zone->AssignedMembers.Num());
	return EBTNodeResult::Succeeded;
}

FString UBTTask_AttachToSafeZone::GetStaticDescription() const
{
	return FString::Printf(TEXT("Attach: '%s' = zone under '%s'"),
		*HomeZoneKey.ToString(), *FoundSafeZoneKey.ToString());
}
