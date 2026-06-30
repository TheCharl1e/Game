// BTTask_Eat.h
// BTTASK-EAT-WIRING-01 — THE canonical eat task (one C++ class; the BP BTTask_Eat.uasset is deprecated).
// Wires the rich appetite model (APPETITE_GRUBAS) onto the affordance claim:
//   claim Nourishment  ->  feed BB NearestFood from the reserved slot (claim = single source of truth)
//   near/high-sig  ->  StartEatingItem + PlayEatMontage(BP body)  ->  AnimNotify_EatBite drives ConsumeBite
//                      ->  Maslow auto-StopEating at satiety/finished  ->  OnEatingStopped -> finish + release
//   far/low-sig    ->  SettleMealInstant (one synchronous pass, no montage)  ->  release  (perf LOD)
//
// Invariants (§8): claim/release is OWNED HERE (C++), never a BP abort. Released in HandleEatingStopped,
// AbortTask, and on any early-out; the affordance subsystem additionally auto-frees on reserver death.
// bCreateNodeInstance = true so each NPC owns its claim/binding state (latent, per-pawn).

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"     // FBlackboardKeySelector
#include "MaslowBiologicalComponent.h"          // EEatStopReason (HandleEatingStopped signature)
#include "BTTask_Eat.generated.h"

class UWorldAffordanceSubsystem;
class UMaslowBiologicalComponent;
class UDataTable;
class APawn;
class UBehaviorTreeComponent;

UCLASS()
class STAN_PIERWOTNY_API UBTTask_Eat : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Eat();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

	/** Radius (uu) to claim the Nourishment affordance at the NPC's feet (standing on it after MoveTo). */
	UPROPERTY(EditAnywhere, Category = "Forage")
	float ConsumeRadius = 300.f;

	/** How many bites to split the meal into (rytm wybija AnimNotify_EatBite). Also caps SettleMealInstant. */
	UPROPERTY(EditAnywhere, Category = "Forage")
	int32 BiteCount = 4;

	/** DT_FoodStats — the brain resolves the food row from the consumable's FoodTableRowName. [set in asset] */
	UPROPERTY(EditAnywhere, Category = "Forage")
	TObjectPtr<UDataTable> FoodTable = nullptr;

	/** BB Object key (BB_NPC: NearestFood) written from the reserved affordance owner. Claim = SoT. */
	UPROPERTY(EditAnywhere, Category = "Forage")
	FBlackboardKeySelector NearestFoodObjectKey;

	/** F5 STUB: significance/LOD signal is absent in 5.8. TODO: hook a real LOD/significance source.
	 *  true = high-sig (full montage loop); false = far (SettleMealInstant). */
	UPROPERTY(EditAnywhere, Category = "Forage")
	bool bIsHighSignificance = true;

private:
	/** Native delegate handler (bound via AddUObject): meal session closed -> finish node + release claim. */
	void HandleEatingStopped(EEatStopReason Reason);

	/** Idempotent: unbind OnEatingStopped, Release the affordance claim, clear the BB key. */
	void ReleaseClaimAndCleanup();

	// --- per-NPC latent state (valid because bCreateNodeInstance = true) ---
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	TWeakObjectPtr<APawn> CachedPawn;
	TWeakObjectPtr<UWorldAffordanceSubsystem> CachedAffordances;
	TWeakObjectPtr<UMaslowBiologicalComponent> CachedMaslow;
	int32 ClaimedId = INDEX_NONE;
	FDelegateHandle EatStoppedHandle;
};
