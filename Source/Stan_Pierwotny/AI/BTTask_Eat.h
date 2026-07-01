// BTTask_Eat.h
// L0 Track A / Slice 1 — consume a Nourishment affordance (brief §1.6). Runs after MoveTo has brought the
// NPC to the EQS-chosen resource. Atomic claim → revalidate-on-arrival → consume → feed Hunger → release.
//
// Fail-safes (§2): the affordance is re-found and reserved HERE (not at selection), so a bush emptied or
// claimed while the NPC walked over simply fails the task (NPC re-forages); the claim is always released,
// including when feeding fails.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Eat.generated.h"

UCLASS()
class STAN_PIERWOTNY_API UBTTask_Eat : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Eat();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	/** Radius (uu) to look for the bush at the NPC's feet (it should be standing on it after MoveTo). */
	UPROPERTY(EditAnywhere, Category = "Forage")
	float ConsumeRadius = 300.f;

	/** Max yield to take this visit. OutGranted (≤ remaining) is fed to the metabolism as sugars. [tune] */
	UPROPERTY(EditAnywhere, Category = "Forage")
	float MaxConsumePerVisit = 50.f;
};
