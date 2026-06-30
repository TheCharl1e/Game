// AffordanceSourceActor.cpp
#include "AffordanceSourceActor.h"
#include "WorldAffordanceSubsystem.h"
#include "ConsumableComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AAffordanceSourceActor::AAffordanceSourceActor()
{
	// Data marker: no Tick. A bare scene root gives it a placeable transform.
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AAffordanceSourceActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UWorldAffordanceSubsystem* Affordances = World->GetSubsystem<UWorldAffordanceSubsystem>())
		{
			AffordanceId = Affordances->RegisterAffordanceSimple(
				AffordanceType, GetActorLocation(), Yield, RegenPerHour, /*Owner*/ this, ColdDampenFactor);
		}
	}

	// DEPLETION-GATING-01 (D1=A): if this source also carries edible data, bridge its depletion to the
	// affordance so a fully-eaten source leaves the query pool (Yield = single source of truth for queries).
	// Opt-in: only binds when a UConsumableComponent is present (water/shelter sources have none).
	if (UConsumableComponent* Cons = FindComponentByClass<UConsumableComponent>())
	{
		Cons->OnDepleted.AddDynamic(this, &AAffordanceSourceActor::HandleConsumableDepleted);
	}
}

void AAffordanceSourceActor::HandleConsumableDepleted()
{
	if (RegenPerHour > 0.f)
	{
		// Renewable: drop from the offer pool until the regen timer refills the yield. NOTE: the consumable's
		// RemainingPortion is NOT reset here — renewable consumable refill is a separate follow-up; the path
		// exercised today is non-renewable (RegenPerHour=0).
		if (UWorld* World = GetWorld())
		{
			if (UWorldAffordanceSubsystem* Affordances = World->GetSubsystem<UWorldAffordanceSubsystem>())
			{
				Affordances->DepleteAffordance(AffordanceId);
			}
		}
	}
	else
	{
		// Non-renewable: gone for good. Destroy -> EndPlay unregisters the record and the claim auto-frees.
		Destroy();
	}
}

void AAffordanceSourceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// §2 fail-safe: owner EndPlay unregisters the record so a destroyed source leaves no ghost.
	if (AffordanceId != INDEX_NONE)
	{
		if (UWorld* World = GetWorld())
		{
			if (UWorldAffordanceSubsystem* Affordances = World->GetSubsystem<UWorldAffordanceSubsystem>())
			{
				Affordances->UnregisterAffordance(AffordanceId);
			}
		}
		AffordanceId = INDEX_NONE;
	}

	Super::EndPlay(EndPlayReason);
}
