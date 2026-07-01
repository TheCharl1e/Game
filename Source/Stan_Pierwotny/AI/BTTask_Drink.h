// BTTask_Drink.h
// L0 Track A / Slice 1 — consume a Hydration affordance (brief §1.6). Same shape as BTTask_Eat: atomic
// claim → revalidate → consume → feed Thirst (CurrentHydration) → release. River points are renewable.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Drink.generated.h"

UCLASS()
class STAN_PIERWOTNY_API UBTTask_Drink : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Drink();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	/** Radius (uu) to look for the water point at the NPC's feet after MoveTo. */
	UPROPERTY(EditAnywhere, Category = "Forage")
	float ConsumeRadius = 300.f;

	/** Max yield to take this visit. OutGranted (≤ remaining) is added to CurrentHydration (0..100). [tune] */
	UPROPERTY(EditAnywhere, Category = "Forage")
	float MaxConsumePerVisit = 40.f;
};
