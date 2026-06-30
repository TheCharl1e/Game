// BTTask_Eat.cpp
#include "BTTask_Eat.h"
#include "AffordanceType.h"
#include "WorldAffordanceSubsystem.h"
#include "MaslowBiologicalComponent.h"
#include "ConsumableComponent.h"
#include "IConsumable.h"
#include "NPCAIControllerBase.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UBTTask_Eat::UBTTask_Eat()
{
	NodeName = TEXT("Eat (consume Nourishment)");
	bCreateNodeInstance = true;   // per-NPC claim + delegate binding state (latent task)
}

EBTNodeResult::Type UBTTask_Eat::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	ANPCAIControllerBase* AC = Cast<ANPCAIControllerBase>(OwnerComp.GetAIOwner());
	if (!IsValid(AC)) { return EBTNodeResult::Failed; }
	APawn* Pawn = AC->GetPawn();
	if (!IsValid(Pawn)) { return EBTNodeResult::Failed; }

	UWorld* World = Pawn->GetWorld();
	UWorldAffordanceSubsystem* Affordances = World ? World->GetSubsystem<UWorldAffordanceSubsystem>() : nullptr;
	UMaslowBiologicalComponent* Maslow = Pawn->FindComponentByClass<UMaslowBiologicalComponent>();
	if (!Affordances || !IsValid(Maslow)) { return EBTNodeResult::Failed; }

	// ── Claim the nearest Nourishment (revalidated at the feet after MoveTo) ──────────────────────────
	const FVector PawnLoc = Pawn->GetActorLocation();
	const int32 Id = Affordances->QueryNearest(EAffordanceType::Nourishment, PawnLoc, ConsumeRadius);
	if (Id == INDEX_NONE) { return EBTNodeResult::Failed; }       // gone / claimed while walking — re-forage
	if (!Affordances->TryReserve(Id, Pawn)) { return EBTNodeResult::Failed; } // someone else claimed it

	// Resolve the consumable carrier on the reserved owner (BP_Food + UConsumableComponent). Claim already held.
	FAffordanceHandle Handle;
	AActor* FoodActor = Affordances->GetAffordance(Id, Handle) ? Handle.Owner.Get() : nullptr;
	UConsumableComponent* Cons = IsValid(FoodActor) ? FoodActor->FindComponentByClass<UConsumableComponent>() : nullptr;
	if (!Cons)
	{
		UE_LOG(LogWorldAffordance, Warning, TEXT("[Eat] %s — reserved id=%d owner has no UConsumableComponent; releasing."),
			*Pawn->GetName(), Id);
		Affordances->Release(Id, Pawn);
		return EBTNodeResult::Failed;
	}
	TScriptInterface<IConsumable> Consumable(Cons);

	// Cache per-NPC latent state (safe: bCreateNodeInstance).
	CachedOwnerComp = &OwnerComp;
	CachedPawn = Pawn;
	CachedAffordances = Affordances;
	CachedMaslow = Maslow;
	ClaimedId = Id;

	// Claim = single source of truth: feed BB NearestFood from the reserved slot owner (never the reverse).
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (NearestFoodObjectKey.IsSet()) { BB->SetValueAsObject(NearestFoodObjectKey.SelectedKeyName, FoodActor); }
	}

	UE_LOG(LogWorldAffordance, Log, TEXT("[Eat] %s claimed id=%d food=%s sig=%s."),
		*Pawn->GetName(), Id, *GetNameSafe(FoodActor), bIsHighSignificance ? TEXT("high") : TEXT("low"));

	// ── LOD-far (low significance): settle the whole meal in one pass, no montage ─────────────────────
	if (!bIsHighSignificance)
	{
		Maslow->SettleMealInstant(Consumable, FoodTable, BiteCount);
		ReleaseClaimAndCleanup();
		return EBTNodeResult::Succeeded;
	}

	// ── Near (high significance): open the meal, bind the close, let the BP body play the montage ─────
	if (!Maslow->StartEatingItem(Consumable, FoodTable, BiteCount))
	{
		ReleaseClaimAndCleanup();   // row unresolved / invalid — StartEatingItem already logged why
		return EBTNodeResult::Failed;
	}
	EatStoppedHandle = Maslow->OnEatingStopped.AddUObject(this, &UBTTask_Eat::HandleEatingStopped);
	AC->PlayEatMontage(FoodActor);   // BP body: montage carries AnimNotify_EatBite -> ConsumeBite (bite clock)
	return EBTNodeResult::InProgress;
}

void UBTTask_Eat::HandleEatingStopped(EEatStopReason /*Reason*/)
{
	UBehaviorTreeComponent* BTComp = CachedOwnerComp.Get();
	ReleaseClaimAndCleanup();
	if (BTComp) { FinishLatentTask(*BTComp, EBTNodeResult::Succeeded); }
}

EBTNodeResult::Type UBTTask_Eat::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	// Unbind FIRST so StopEating's OnEatingStopped broadcast can't re-enter HandleEatingStopped (double-finish).
	if (UMaslowBiologicalComponent* M = CachedMaslow.Get())
	{
		if (EatStoppedHandle.IsValid()) { M->OnEatingStopped.Remove(EatStoppedHandle); EatStoppedHandle.Reset(); }
		M->StopEating(EEatStopReason::Interrupted);   // idempotent (no-op if not eating)
	}
	if (ANPCAIControllerBase* AC = Cast<ANPCAIControllerBase>(OwnerComp.GetAIOwner())) { AC->StopEatMontage(); }
	ReleaseClaimAndCleanup();
	return EBTNodeResult::Aborted;
}

void UBTTask_Eat::ReleaseClaimAndCleanup()
{
	if (UMaslowBiologicalComponent* M = CachedMaslow.Get())
	{
		if (EatStoppedHandle.IsValid()) { M->OnEatingStopped.Remove(EatStoppedHandle); }
	}
	EatStoppedHandle.Reset();

	if (UWorldAffordanceSubsystem* Aff = CachedAffordances.Get())
	{
		if (ClaimedId != INDEX_NONE && CachedPawn.IsValid())
		{
			Aff->Release(ClaimedId, CachedPawn.Get());
			UE_LOG(LogWorldAffordance, Log, TEXT("[Eat] %s released id=%d."), *GetNameSafe(CachedPawn.Get()), ClaimedId);
		}
	}

	if (UBehaviorTreeComponent* BT = CachedOwnerComp.Get())
	{
		if (UBlackboardComponent* BB = BT->GetBlackboardComponent())
		{
			if (NearestFoodObjectKey.IsSet()) { BB->ClearValue(NearestFoodObjectKey.SelectedKeyName); }
		}
	}
	ClaimedId = INDEX_NONE;
}

void UBTTask_Eat::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		NearestFoodObjectKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTTask_Eat::GetStaticDescription() const
{
	return FString::Printf(TEXT("Claim + eat nearest Nourishment within %.0f uu (%d bites; far-LOD = instant)"),
		ConsumeRadius, BiteCount);
}
