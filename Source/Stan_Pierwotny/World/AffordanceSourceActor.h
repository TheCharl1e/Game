// AffordanceSourceActor.h
// L0 Track A / Slice 1 — designer-placeable world resource. ONE data-driven actor (configure Type/Yield/
// Regen/ColdDampen per instance) that self-registers a record in UWorldAffordanceSubsystem on BeginPlay and
// unregisters on EndPlay (the §2 owner-EndPlay fail-safe). Place berry bushes (Nourishment), river points
// (Hydration) or standalone shelters (Shelter) with the same class — zero hardcoded values.
//
// This is the body for the affordance registry: visuals/mesh are added in the Blueprint child; C++ only
// owns the record's lifetime + numbers.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AffordanceType.h"
#include "AffordanceSourceActor.generated.h"

UCLASS()
class STAN_PIERWOTNY_API AAffordanceSourceActor : public AActor
{
	GENERATED_BODY()

public:
	AAffordanceSourceActor();

	/** What this resource offers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affordance")
	EAffordanceType AffordanceType = EAffordanceType::Nourishment;

	/** Starting (and max) consumable yield. Ignored for Shelter (no yield). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affordance", meta = (ClampMin = "0.0"))
	float Yield = 100.f;

	/** Per game-hour regrowth toward Yield. 0 = non-renewable (a depleted bush stays empty). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affordance", meta = (ClampMin = "0.0"))
	float RegenPerHour = 0.f;

	/** Shelter only: read-side cold-deficit dampen (0..1). See GetFeltTemperature (decision #3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affordance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ColdDampenFactor = 0.f;

	/** The registry Id while registered (INDEX_NONE otherwise). Read-only debug. */
	UFUNCTION(BlueprintPure, Category = "Affordance")
	int32 GetAffordanceId() const { return AffordanceId; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Registry Id for this instance's record. */
	int32 AffordanceId = INDEX_NONE;
};
