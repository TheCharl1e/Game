#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BTService_MaslowBlackboardSync.generated.h"

/**
 * THE BRIDGE between the C++ metabolic simulation and the Blueprint Behavior Tree.
 *
 * Reads UMaslowBiologicalComponent from the controlled pawn and writes typed,
 * clean values to BB_NPC. This is the ONLY place where C++ Maslow data enters
 * the AI decision layer. Nothing else should duplicate this state.
 *
 * Tick interval is configured in the BT editor (recommended: 1.0–2.0s).
 * For 500 NPCs at 1s interval: 500 cheap float reads + BB writes per second.
 * That is not a performance problem.
 *
 * HOW TO USE IN BT EDITOR:
 * - Add this service to the ROOT node of BT_NPC (runs for the whole tree).
 * - Bind each Key selector to the matching key in BB_NPC.
 * - Remove (or stop reading) DaysOfHunger / DaysOfThirst BP keys — they are
 *   now superseded by the values this service writes.
 */
UCLASS()
class STAN_PIERWOTNY_API UBTService_MaslowBlackboardSync : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_MaslowBlackboardSync();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory, float DeltaSeconds) override;

	virtual FString GetStaticDescription() const override;

public:
	// --- Blackboard key binding (bind in BT editor) -----------------------
	// ONE authoritative key, ONE writer (Maslow->BT bridge, slice #1). C++ is now the sole writer
	// of the NPC's concrete need. Bind this to the Enum key "CurrentNeed" (E_NeedState) in BT_NPC.
	// The service writes it via SetValueAsEnum(GetActionableNeed()) — correctly typed (was: 6 selectors
	// all mis-bound to CurrentNeed + SetValueAsInt/Float/Bool, which zeroed the key every tick).

	/** The NPC's concrete actionable need (E_NeedState: 0=None,1=Hunger,2=Thirst,3=Sleep,4=Flee).
	 *  Bind to "CurrentNeed" in BB_NPC. Decorators branch on this Enum. */
	UPROPERTY(EditAnywhere, Category = "Maslow|Keys")
	FBlackboardKeySelector CurrentNeedKey;

private:
	/** Cache the component reference per owner pawn to avoid FindComponentByClass
	 *  every tick. Invalidated automatically when pawn changes. */
	UPROPERTY()
	TObjectPtr<class UMaslowBiologicalComponent> CachedMaslowComp = nullptr;

	/** The pawn we last cached from. Used to detect pawn switches. */
	UPROPERTY()
	TObjectPtr<APawn> CachedPawn = nullptr;

	/** Safely fetch (and re-cache when needed) the Maslow component. */
	UMaslowBiologicalComponent* GetMaslowComponent(UBehaviorTreeComponent& OwnerComp);
};
