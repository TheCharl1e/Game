// ConsumableComponent.h
// ICONSUMABLE-EAT-01 (D2 = component) — lightweight carrier of food data + bite mechanic, attached to BP_Food
// ALONGSIDE its AAffordanceSourceActor parent (no shared actor base required). C++ = brain (data + math);
// BP = body: BP binds OnDepleted to play the destroy/scrap reaction. Mirrors AItemBase's food fields so the
// eat loop is identical whether the edible is a component (now) or an actor (AItemBase, later — D3).
//
// Perf x500: zero tick, zero per-NPC cost; ConsumePortion is O(1) and only runs on a bite (AnimNotify cadence).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IConsumable.h"
#include "ConsumableComponent.generated.h"

/** BP reacts to the consumable hitting zero (destroy actor / leave a scrap). C++ decides WHEN, BP draws HOW. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConsumableDepleted);

UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class STAN_PIERWOTNY_API UConsumableComponent : public UActorComponent, public IConsumable
{
	GENERATED_BODY()

public:
	UConsumableComponent();

	// Row id from DT_FoodStats (links this world item to its EUREKA stats). Mirror AItemBase::FoodTableRowName.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	FName FoodTableRowName;

	// 1.0 = whole, 0.0 = eaten. Each bite removes a fraction; bitten food stays in the world. Mirror AItemBase.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	float RemainingPortion = 1.0f;

	// Fires once when RemainingPortion reaches 0. BP_Food binds this to destroy the actor / leave an ogryzek.
	UPROPERTY(BlueprintAssignable, Category = "Consumable")
	FOnConsumableDepleted OnDepleted;

	//~ IConsumable
	virtual FName GetFoodTableRowName_Implementation() const override { return FoodTableRowName; }
	virtual float GetRemainingPortion_Implementation() const override { return RemainingPortion; }
	virtual float ConsumePortion_Implementation(float RequestedPortion) override;
};
