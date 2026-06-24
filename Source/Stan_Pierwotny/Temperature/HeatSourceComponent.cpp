#include "HeatSourceComponent.h"
#include "HeatSourceRegistrySubsystem.h"
#include "Engine/World.h"

UHeatSourceComponent::UHeatSourceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;   // zero tick — rejestr odpytywany przez NPC
}

void UHeatSourceComponent::BeginPlay()
{
    Super::BeginPlay();
    if (const UWorld* World = GetWorld())
    {
        if (UHeatSourceRegistrySubsystem* Registry = World->GetSubsystem<UHeatSourceRegistrySubsystem>())
        {
            Registry->RegisterSource(this);
        }
    }
}

void UHeatSourceComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (const UWorld* World = GetWorld())
    {
        if (UHeatSourceRegistrySubsystem* Registry = World->GetSubsystem<UHeatSourceRegistrySubsystem>())
        {
            Registry->UnregisterSource(this);
        }
    }
    Super::EndPlay(Reason);
}
