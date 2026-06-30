// IConsumable.h
// ICONSUMABLE-EAT-01 — decouple the rich eating model (StartEatingItem -> ConsumeBite -> StopEating) from a
// single actor base. In 5.8 BP_Food is reparented to AAffordanceSourceActor (live forage), so the food layer
// cannot also force AItemBase. Anything edible (UConsumableComponent on BP_Food now; AItemBase / corpse later)
// implements this interface; the metabolism calls the interface, never a concrete base.
//
// Minimal slice (D1/D7): only the eat-loop API. Ownership / theft (CanBeEatenBy, IsStolenBy — L5) deferred.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IConsumable.generated.h"

UINTERFACE(BlueprintType)
class UConsumable : public UInterface
{
	GENERATED_BODY()
};

class STAN_PIERWOTNY_API IConsumable
{
	GENERATED_BODY()

public:
	/** Row id into DT_FoodStats (FFoodItemRow). Data-driven: lives on the consumable, never hardcoded. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Consumable")
	FName GetFoodTableRowName() const;

	/** Take one bite's fraction (0..1 of the whole). Returns the fraction ACTUALLY removed (<= requested,
	 *  <= remaining). Mirrors AItemBase::ConsumePortion: bitten food stays in the world until depleted. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Consumable")
	float ConsumePortion(float RequestedPortion);

	/** Remaining portion: 1.0 = whole, 0.0 = fully eaten. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Consumable")
	float GetRemainingPortion() const;
};
