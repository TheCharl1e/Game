// BTTask_MoveToKnownZone.cpp
#include "BTTask_MoveToKnownZone.h"
#include "AffordanceType.h"            // LogExploration
#include "CaldrethZone.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

UBTTask_MoveToKnownZone::UBTTask_MoveToKnownZone()
{
	NodeName = TEXT("Move To Known Safe Zone");
}

EBTNodeResult::Type UBTTask_MoveToKnownZone::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BB)) { return EBTNodeResult::Failed; }

	// HomeZone is a weak memory: a destroyed zone reads back as null here -> fall through to WANDER.
	ACaldrethZone* Zone = Cast<ACaldrethZone>(BB->GetValueAsObject(HomeZoneKey));
	if (!IsValid(Zone))
	{
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsVector(MoveTargetKey, Zone->GetActorLocation());
	UE_LOG(LogExploration, Verbose, TEXT("[Explore] Move to known zone '%s'."), *Zone->GetName());
	return EBTNodeResult::Succeeded;
}

FString UBTTask_MoveToKnownZone::GetStaticDescription() const
{
	return FString::Printf(TEXT("MoveTarget '%s' = location of remembered '%s'"),
		*MoveTargetKey.ToString(), *HomeZoneKey.ToString());
}
