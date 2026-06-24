#include "NPCIdentityComponent.h"
#include "NPCRegistrySubsystem.h"
#include "InventoryComponent.h"   // L3-05: stamp ownership with the registry ID
#include "GameFramework/Character.h"

UNPCIdentityComponent::UNPCIdentityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;   // project rule: NO Event Tick
}

void UNPCIdentityComponent::BeginPlay()
{
    Super::BeginPlay();

    if (NPCId != 0) { return; }                  // EC: idempotent (double BeginPlay)

    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!IsValid(OwnerChar))
    {
        UE_LOG(LogNPCRegistry, Warning, TEXT("[Identity] %s: owner not a Character — not registered."),
               *GetNameSafe(GetOwner()));
        return;
    }

    UWorld* World = GetWorld();
    if (!IsValid(World)) { return; }             // EC: teardown

    UNPCRegistrySubsystem* Registry = World->GetSubsystem<UNPCRegistrySubsystem>();
    if (!IsValid(Registry))
    {
        UE_LOG(LogNPCRegistry, Warning, TEXT("[Identity] %s: registry unavailable — not registered."),
               *GetNameSafe(OwnerChar));
        return;
    }

    NPCId = Registry->RegisterNPC(OwnerChar);

    // L3-05 P2P: the inventory's OwnerID IS the registry ID — so the contract pool can resolve a
    // poster/accepter back to their inventory by NPC ID, and theft (L5) attributes to a real identity.
    // Order-safe: UInventoryComponent::BeginPlay never touches OwnerID, so this set persists regardless
    // of component BeginPlay order.
    if (UInventoryComponent* Inv = OwnerChar->FindComponentByClass<UInventoryComponent>())
    {
        Inv->OwnerID = NPCId;
    }
}

void UNPCIdentityComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (NPCId != 0)
    {
        if (UWorld* World = GetWorld())
        {
            if (UNPCRegistrySubsystem* Registry = World->GetSubsystem<UNPCRegistrySubsystem>())
            {
                Registry->UnregisterNPC(NPCId);
            }
        }
        NPCId = 0;
    }
    Super::EndPlay(Reason);
}
