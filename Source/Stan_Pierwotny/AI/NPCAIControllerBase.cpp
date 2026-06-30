// NPCAIControllerBase.cpp
#include "NPCAIControllerBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "MaslowBiologicalComponent.h"          // LogMaslow
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

void ANPCAIControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Run the designer-assigned tree (BT_Exploration on the NPC BP). Guarded: a controller without a tree
	// assigned simply runs nothing here — the Blueprint may still start its own logic.
	if (DefaultBehaviorTree)
	{
		RunBehaviorTree(DefaultBehaviorTree);
	}
}

UAnimInstance* ANPCAIControllerBase::GetPawnAnimInstance() const
{
	APawn* PossessedPawn = GetPawn();
	if (!IsValid(PossessedPawn)) { return nullptr; }

	// Prefer the Character mesh (BP_NPC_Character derives from ACharacter); fall back to any skeletal mesh.
	USkeletalMeshComponent* Mesh = nullptr;
	if (const ACharacter* Char = Cast<ACharacter>(PossessedPawn)) { Mesh = Char->GetMesh(); }
	if (!IsValid(Mesh)) { Mesh = PossessedPawn->FindComponentByClass<USkeletalMeshComponent>(); }
	return IsValid(Mesh) ? Mesh->GetAnimInstance() : nullptr;
}

void ANPCAIControllerBase::PlayEatMontage(AActor* FoodActor)
{
	if (!IsValid(EatMontage))
	{
		// Data gap, not a crash: the BTTask still drives bites via Maslow only if the montage exists. Without a
		// montage there are no AnimNotify_EatBite ticks, so the meal would close on satiety alone — flag it once.
		UE_LOG(LogMaslow, Warning, TEXT("[Eat] %s — EatMontage unset on controller; no bite clock will play."),
			*GetNameSafe(GetPawn()));
		return;
	}

	UAnimInstance* AnimInst = GetPawnAnimInstance();
	if (!AnimInst)
	{
		UE_LOG(LogMaslow, Warning, TEXT("[Eat] %s — no AnimInstance on pawn; cannot play EatMontage."),
			*GetNameSafe(GetPawn()));
		return;
	}

	const float Len = AnimInst->Montage_Play(EatMontage);
	UE_LOG(LogMaslow, Log, TEXT("[Eat] %s playing EatMontage=%s (len=%.2f) on food=%s."),
		*GetNameSafe(GetPawn()), *GetNameSafe(EatMontage), Len, *GetNameSafe(FoodActor));
}

void ANPCAIControllerBase::StopEatMontage()
{
	if (!IsValid(EatMontage)) { return; }
	if (UAnimInstance* AnimInst = GetPawnAnimInstance())
	{
		// Only stops if this montage is the one playing (idempotent no-op otherwise).
		AnimInst->Montage_Stop(EatMontageBlendOut, EatMontage);
		UE_LOG(LogMaslow, Log, TEXT("[Eat] %s stopped EatMontage."), *GetNameSafe(GetPawn()));
	}
}
