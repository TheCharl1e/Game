// AffordanceSourceActor.cpp
#include "AffordanceSourceActor.h"
#include "WorldAffordanceSubsystem.h"
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
