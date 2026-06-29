// BTTask_AttachToSafeZone.h
// L0 Track A / Slice 1 — WANDER → attach (brief §4). Turns an EQS_FindSafeZone hit into a HOME: resolves
// the zone under the found shelter point, writes it to HomeZone, and registers the NPC as a member.
//
// EQS_FindSafeZone is the affordance generator (type Shelter); its best item is a world point. The zone is
// resolved from that point via ACaldrethZone::GetZoneAtLocation (the shelter point == zone centroid).

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AttachToSafeZone.generated.h"

UCLASS()
class STAN_PIERWOTNY_API UBTTask_AttachToSafeZone : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AttachToSafeZone();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	/** Blackboard Vector key holding the EQS_FindSafeZone result point (the shelter location). */
	UPROPERTY(EditAnywhere, Category = "Exploration")
	FName FoundSafeZoneKey = TEXT("FoundSafeZone");

	/** Blackboard Object key to write the attached ACaldrethZone into (the NPC's home memory). */
	UPROPERTY(EditAnywhere, Category = "Exploration")
	FName HomeZoneKey = TEXT("HomeZone");
};
