#include "NPCRegistrySubsystem.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY(LogNPCRegistry);

int32 UNPCRegistrySubsystem::RegisterNPC(ACharacter* NPC)
{
    if (!IsValid(NPC))
    {
        UE_LOG(LogNPCRegistry, Warning, TEXT("[Registry] RegisterNPC: invalid NPC — skipped."));
        return 0;   // INVALID
    }

    const int32 NewId = NextNPCId++;          // never rolls back — IDs retired forever
    RegisteredNPCs.Add(NewId, NPC);

    UE_LOG(LogNPCRegistry, Log, TEXT("[Registry] Registered %s as ID %d (live=%d)."),
           *GetNameSafe(NPC), NewId, RegisteredNPCs.Num());
    return NewId;
}

void UNPCRegistrySubsystem::UnregisterNPC(int32 NPCId)
{
    if (NPCId <= 0) { return; }               // 0/negative = never registered

    if (RegisteredNPCs.Remove(NPCId) > 0)
    {
        UE_LOG(LogNPCRegistry, Log, TEXT("[Registry] Unregistered ID %d (live=%d). ID retired."),
               NPCId, RegisteredNPCs.Num());
    }
    // not present → silent (idempotent)
}

ACharacter* UNPCRegistrySubsystem::GetNPCById(int32 NPCId)
{
    if (NPCId <= 0) { return nullptr; }

    TWeakObjectPtr<ACharacter>* Found = RegisteredNPCs.Find(NPCId);
    if (!Found) { return nullptr; }           // unknown ID

    ACharacter* NPC = Found->Get();
    if (IsValid(NPC)) { return NPC; }

    // Stale: died without unregister (should not happen — diagnostic + self-heal).
    UE_LOG(LogNPCRegistry, Warning,
           TEXT("[Registry] ID %d stale (destroyed without unregister) — self-healed."), NPCId);
    RegisteredNPCs.Remove(NPCId);
    return nullptr;
}
