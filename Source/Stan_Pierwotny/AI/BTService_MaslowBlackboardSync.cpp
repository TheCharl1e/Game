#include "BTService_MaslowBlackboardSync.h"
#include "MaslowBiologicalComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTService_MaslowBlackboardSync::UBTService_MaslowBlackboardSync()
{
	// Human-readable name shown in the BT editor node.
	NodeName = TEXT("Maslow Blackboard Sync");

	// Recommended default: re-evaluate every 1 second.
	// Designer can override per-NPC-archetype in the BT editor.
	// Do NOT set this below 0.5s for 500+ NPC builds.
	Interval		= 1.0f;
	RandomDeviation = 0.1f; // Stagger ticks across NPCs to flatten CPU spikes.
}

UMaslowBiologicalComponent* UBTService_MaslowBlackboardSync::GetMaslowComponent(
	UBehaviorTreeComponent& OwnerComp)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!IsValid(Controller))
	{
		return nullptr;
	}

	APawn* Pawn = Controller->GetPawn();
	if (!IsValid(Pawn))
	{
		return nullptr;
	}

	// Re-use cached component as long as the pawn hasn't changed.
	if (CachedPawn == Pawn && IsValid(CachedMaslowComp))
	{
		return CachedMaslowComp;
	}

	// Pawn changed (spawn / possession switch) — refresh cache.
	CachedPawn		 = Pawn;
	CachedMaslowComp = Pawn->FindComponentByClass<UMaslowBiologicalComponent>();

	if (!IsValid(CachedMaslowComp))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[MaslowSync] Pawn '%s' has no UMaslowBiologicalComponent — "
				 "service will produce no output."),
			*GetNameSafe(Pawn));
	}

	return CachedMaslowComp;
}

void UBTService_MaslowBlackboardSync::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UMaslowBiologicalComponent* Maslow = GetMaslowComponent(OwnerComp);
	if (!IsValid(Maslow))
	{
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BB))
	{
		return;
	}

	// --- Read concrete need from the simulation layer (Maslow->BT bridge, slice #1) ---
	const uint8 Need = Maslow->GetActionableNeed(); // E_NeedState byte: 0=None,1=Hunger,2=Thirst,3=Sleep,4=Flee

	// --- Write ONE authoritative, correctly-typed key to the AI decision layer ---
	// SetValueAsEnum matches the Enum key "CurrentNeed" (E_NeedState). C++ is now the sole writer.
	BB->SetValueAsEnum(CurrentNeedKey.SelectedKeyName, Need);

	// Verbose trace (off in shipping) -- enable LogTemp Verbose to watch the need each sync.
	UE_LOG(LogTemp, Verbose,
		TEXT("[MaslowSync:%s] CurrentNeed=%d"),
		*GetNameSafe(CachedPawn), Need);
}

FString UBTService_MaslowBlackboardSync::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("C++ Maslow is the sole writer of the NPC's concrete need. "
			 "GetActionableNeed() -> %s (Enum E_NeedState, via SetValueAsEnum)"),
		*CurrentNeedKey.SelectedKeyName.ToString());
}
