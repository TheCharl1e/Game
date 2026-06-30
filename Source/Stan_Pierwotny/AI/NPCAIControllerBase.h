// NPCAIControllerBase.h
// L0 Track A — controller base that runs a designer-assigned BehaviorTree on possession.
// Exists so the NPC Blueprint controller can run BT_Exploration (the EQS forage/drink spine) via a
// C++-owned UBehaviorTree* reference (set per Blueprint), instead of a graph literal pin. C++ owns the
// "which tree" wiring; the Blueprint child just sets DefaultBehaviorTree.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NPCAIControllerBase.generated.h"

class UBehaviorTree;
class UAnimMontage;
class UAnimInstance;

UCLASS()
class STAN_PIERWOTNY_API ANPCAIControllerBase : public AAIController
{
	GENERATED_BODY()

public:
	/** Behavior tree to run when this controller possesses a pawn. Set per Blueprint (e.g. BT_Exploration). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTree = nullptr;

	// BTTASK-EAT-WIRING-01 (F1, REVISED 2026-07-01 — director-approved deviation):
	// The eat montage is played from C++ instead of a BP BlueprintImplementableEvent. Rationale: the BP-graph
	// montage-play node (object-ref pin) cannot be authored via MCP tooling without corruption, so the montage
	// is held as a DATA-DRIVEN designer-assigned asset (EatMontage, set per-Blueprint/CDO) and played here.
	// C++ still does NOT hardcode/load any animation — it only plays whatever montage the designer assigns.
	// Convention note: this bends "anim lives in BP" for this one node; approved because the reference stays
	// data-driven and the montage itself (with AnimNotify_EatBite → ConsumeBite) is authored as a BP asset.
	/** Eat montage played on the possessed pawn; MUST carry AnimNotify_EatBite ≥ BiteCount. [data, set per-BP] */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Appetite")
	TObjectPtr<UAnimMontage> EatMontage = nullptr;

	/** Blend-out time (s) when StopEatMontage interrupts the eat montage. */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Appetite")
	float EatMontageBlendOut = 0.2f;

	// Called by UBTTask_Eat. FoodActor = the affordance owner (BP_Food) being eaten (logging/anim context only).
	UFUNCTION(BlueprintCallable, Category = "AI|Appetite")
	void PlayEatMontage(AActor* FoodActor);

	/** Stop/interrupt the eat montage (eat aborted before the session finished). Idempotent. */
	UFUNCTION(BlueprintCallable, Category = "AI|Appetite")
	void StopEatMontage();

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	/** Resolve the possessed pawn's AnimInstance (ACharacter mesh first, else any USkeletalMeshComponent). */
	UAnimInstance* GetPawnAnimInstance() const;
};
