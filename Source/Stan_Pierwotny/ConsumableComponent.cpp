// ConsumableComponent.cpp
#include "ConsumableComponent.h"

UConsumableComponent::UConsumableComponent()
{
	// No tick: state changes only on a bite (ConsumePortion), driven by the eat loop / AnimNotify.
	PrimaryComponentTick.bCanEverTick = false;
}

float UConsumableComponent::ConsumePortion_Implementation(float RequestedPortion)
{
	// Take <= requested, <= remaining. Mirror AItemBase::ConsumePortion (bitten food stays until depleted).
	const float Taken = FMath::Clamp(RequestedPortion, 0.0f, RemainingPortion);
	RemainingPortion -= Taken;

	if (RemainingPortion <= KINDA_SMALL_NUMBER)
	{
		RemainingPortion = 0.0f;
		OnDepleted.Broadcast();   // BP: destroy / leave scrap. Guarded by KINDA_SMALL_NUMBER so it fires once.
	}
	return Taken;
}
