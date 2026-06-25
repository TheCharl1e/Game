// EnvQueryGenerator_Affordances.h
// L0 Track A / Slice 1 — bridges the affordance REGISTRY into EQS.
//
// Affordances are lightweight records, not Actors, so the stock "Actors of Class" generator cannot see
// them. This generator pulls candidate points of a chosen EAffordanceType from UWorldAffordanceSubsystem
// (within SearchRadius of the querier), so EQS_FindResource can distance-score them AND apply a stock
// Pathfinding test for "reachable" (brief §4). Discovery stays EQS, all of it routed through the budget.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "AffordanceType.h"
#include "EnvQueryGenerator_Affordances.generated.h"

UCLASS(meta = (DisplayName = "Affordances (Caldreth)"))
class STAN_PIERWOTNY_API UEnvQueryGenerator_Affordances : public UEnvQueryGenerator
{
	GENERATED_BODY()

public:
	UEnvQueryGenerator_Affordances();

	/** Which affordance kind to gather (Nourishment / Hydration / Shelter). */
	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	EAffordanceType AffordanceType = EAffordanceType::Nourishment;

	/** Gather radius (uu) around the querier. Keep ~ the forage range; bounds the spatial-hash scan. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	float SearchRadius = 5000.f;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};
