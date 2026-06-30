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

UCLASS()
class STAN_PIERWOTNY_API ANPCAIControllerBase : public AAIController
{
	GENERATED_BODY()

public:
	/** Behavior tree to run when this controller possesses a pawn. Set per Blueprint (e.g. BT_Exploration). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTree = nullptr;

	// BTTASK-EAT-WIRING-01 (F1): C++ eat task fires the eat animation through the BP body. BP_NPC_AI plays
	// the eat montage on the possessed pawn (montage carries AnimNotify_EatBite → ConsumeBite). C++ = brain
	// (decides WHEN/claim), BP = body (plays montage). FoodActor = the affordance owner (BP_Food) being eaten.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|Appetite")
	void PlayEatMontage(AActor* FoodActor);

	/** Stop/interrupt the eat montage (eat aborted before the session finished). */
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|Appetite")
	void StopEatMontage();

protected:
	virtual void OnPossess(APawn* InPawn) override;
};
