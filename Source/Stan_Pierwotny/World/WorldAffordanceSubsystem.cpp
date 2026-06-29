// WorldAffordanceSubsystem.cpp
#include "WorldAffordanceSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

// Single definition site for all three Slice 1 world/exploration log categories (declared in AffordanceType.h).
DEFINE_LOG_CATEGORY(LogWorldAffordance);
DEFINE_LOG_CATEGORY(LogExploration);

void UWorldAffordanceSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Regen on a TIMER (project rule: never Tick). Renewable affordances regrow off the metabolism cadence.
	if (RegenInterval > 0.f)
	{
		InWorld.GetTimerManager().SetTimer(
			RegenTimerHandle, this, &UWorldAffordanceSubsystem::RegenTick, RegenInterval, /*loop*/ true);
	}
	UE_LOG(LogWorldAffordance, Log, TEXT("[Affordance] Subsystem ready (CellSize=%.0f, RegenInterval=%.1fs)."),
		CellSize, RegenInterval);
}

void UWorldAffordanceSubsystem::Deinitialize()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RegenTimerHandle);
	}
	Super::Deinitialize();
}

FIntVector UWorldAffordanceSubsystem::ToCell(const FVector& Location) const
{
	const float Inv = (CellSize > KINDA_SMALL_NUMBER) ? (1.f / CellSize) : 0.f;
	return FIntVector(FMath::FloorToInt(Location.X * Inv), FMath::FloorToInt(Location.Y * Inv), 0);
}

bool UWorldAffordanceSubsystem::IsLive(int32 Id) const
{
	if (!Affordances.IsValidIndex(Id)) { return false; }
	const FAffordanceHandle& H = Affordances[Id];
	if (H.Type == EAffordanceType::None) { return false; }        // freed slot
	if (H.RemainingYield <= 0.f && H.Type != EAffordanceType::Shelter) { return false; } // shelter has no yield
	// Ownerless records (test fixtures) are allowed; owned records require a live owner.
	if (H.Owner.IsStale(/*bIncludingGarbage*/ true)) { return false; }
	return true;
}

int32 UWorldAffordanceSubsystem::RegisterAffordance(const FAffordanceHandle& Handle)
{
	int32 Id;
	if (FreeList.Num() > 0)
	{
		Id = FreeList.Pop(EAllowShrinking::No);
		Affordances[Id] = Handle;
	}
	else
	{
		Id = Affordances.Add(Handle);
	}

	FAffordanceHandle& H = Affordances[Id];
	// A freshly registered record is never born reserved, regardless of what the caller passed.
	H.ReservedBy = INDEX_NONE;
	H.ReservedByActor = nullptr;
	H.bExhaustLogged = false;
	if (H.MaxYield <= 0.f) { H.MaxYield = H.RemainingYield; } // default cap = starting yield

	H.HashCell = ToCell(H.Location);
	SpatialHash.FindOrAdd(H.HashCell).Add(Id);

	UE_LOG(LogWorldAffordance, Log,
		TEXT("[Affordance] Register id=%d type=%d yield=%.1f regen/h=%.2f dampen=%.2f owner=%s cell=(%d,%d)."),
		Id, (int32)H.Type, H.RemainingYield, H.RegenPerHour, H.ColdDampenFactor,
		H.Owner.IsValid() ? *H.Owner->GetName() : TEXT("<none>"), H.HashCell.X, H.HashCell.Y);
	return Id;
}

int32 UWorldAffordanceSubsystem::RegisterAffordanceSimple(EAffordanceType Type, FVector Location, float Yield,
	float RegenPerHour, AActor* Owner, float ColdDampenFactor)
{
	FAffordanceHandle H;
	H.Type = Type;
	H.Location = Location;
	H.RemainingYield = Yield;
	H.MaxYield = Yield;
	H.RegenPerHour = RegenPerHour;
	H.ColdDampenFactor = ColdDampenFactor;
	H.Owner = Owner;
	return RegisterAffordance(H);
}

void UWorldAffordanceSubsystem::UnregisterAffordance(int32 Id)
{
	if (!Affordances.IsValidIndex(Id) || Affordances[Id].Type == EAffordanceType::None) { return; }

	FAffordanceHandle& H = Affordances[Id];
	if (TArray<int32>* Cell = SpatialHash.Find(H.HashCell))
	{
		Cell->RemoveSingleSwap(Id, EAllowShrinking::No);
		if (Cell->Num() == 0) { SpatialHash.Remove(H.HashCell); }
	}

	UE_LOG(LogWorldAffordance, Log, TEXT("[Affordance] Unregister id=%d type=%d."), Id, (int32)H.Type);

	H = FAffordanceHandle();        // wipe -> Type=None (freed)
	FreeList.Add(Id);
}

int32 UWorldAffordanceSubsystem::QueryNearest(EAffordanceType Type, const FVector& From, float Radius) const
{
	const float RadiusSq = Radius * Radius;
	const int32 CellReach = (CellSize > KINDA_SMALL_NUMBER) ? FMath::CeilToInt(Radius / CellSize) : 0;
	const FIntVector Base = ToCell(From);

	int32 BestId = INDEX_NONE;
	float BestDistSq = RadiusSq;

	for (int32 dx = -CellReach; dx <= CellReach; ++dx)
	{
		for (int32 dy = -CellReach; dy <= CellReach; ++dy)
		{
			const FIntVector Cell(Base.X + dx, Base.Y + dy, 0);
			const TArray<int32>* Ids = SpatialHash.Find(Cell);
			if (!Ids) { continue; }

			for (int32 Id : *Ids)
			{
				const FAffordanceHandle& H = Affordances[Id];
				if (H.Type != Type || !IsLive(Id)) { continue; }
				// Reserved? A reserver that died without releasing is treated as free (death fail-safe).
				if (H.ReservedBy != INDEX_NONE && H.ReservedByActor.IsValid()) { continue; }

				const float DistSq = FVector::DistSquared(From, H.Location);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestId = Id;
				}
			}
		}
	}
	return BestId;
}

bool UWorldAffordanceSubsystem::TryReserve(int32 Id, AActor* By)
{
	if (!IsLive(Id) || !IsValid(By)) { return false; }
	FAffordanceHandle& H = Affordances[Id];

	// Stale reserver (died without Release) auto-frees the claim before we test it.
	if (H.ReservedBy != INDEX_NONE && !H.ReservedByActor.IsValid())
	{
		H.ReservedBy = INDEX_NONE;
		H.ReservedByActor = nullptr;
	}
	if (H.ReservedBy != INDEX_NONE) { return false; } // genuinely claimed by someone alive

	H.ReservedBy = (int32)By->GetUniqueID();
	H.ReservedByActor = By;
	UE_LOG(LogWorldAffordance, Verbose, TEXT("[Affordance] Reserve id=%d by=%s."), Id, *By->GetName());
	return true;
}

void UWorldAffordanceSubsystem::Release(int32 Id, AActor* By)
{
	if (!Affordances.IsValidIndex(Id)) { return; }
	FAffordanceHandle& H = Affordances[Id];
	// Only the holder may release; but if By is null (NPC destructor lost identity) and the actor ptr is
	// already stale, clear it anyway so the slot never sticks.
	const bool bMatches = IsValid(By) && H.ReservedBy == (int32)By->GetUniqueID();
	if (bMatches || !H.ReservedByActor.IsValid())
	{
		H.ReservedBy = INDEX_NONE;
		H.ReservedByActor = nullptr;
	}
}

bool UWorldAffordanceSubsystem::Consume(int32 Id, float Amount, float& OutGranted)
{
	OutGranted = 0.f;
	if (!IsLive(Id) || Amount <= 0.f) { return false; }   // revalidate on arrival (§2)

	FAffordanceHandle& H = Affordances[Id];
	OutGranted = FMath::Min(Amount, H.RemainingYield);
	H.RemainingYield -= OutGranted;

	if (H.RemainingYield <= KINDA_SMALL_NUMBER && !H.bExhaustLogged)
	{
		H.bExhaustLogged = true;
		UE_LOG(LogWorldAffordance, Warning, TEXT("[Affordance] id=%d type=%d EXHAUSTED (regen/h=%.2f)."),
			Id, (int32)H.Type, H.RegenPerHour);
	}
	return OutGranted > 0.f;
}

bool UWorldAffordanceSubsystem::GetAffordance(int32 Id, FAffordanceHandle& OutHandle) const
{
	if (!Affordances.IsValidIndex(Id) || Affordances[Id].Type == EAffordanceType::None) { return false; }
	OutHandle = Affordances[Id];
	return true;
}

void UWorldAffordanceSubsystem::GatherAffordanceLocations(EAffordanceType Type, const FVector& Center,
	float Radius, TArray<FVector>& OutLocations) const
{
	OutLocations.Reset();
	const float RadiusSq = Radius * Radius;
	const int32 CellReach = (CellSize > KINDA_SMALL_NUMBER) ? FMath::CeilToInt(Radius / CellSize) : 0;
	const FIntVector Base = ToCell(Center);

	for (int32 dx = -CellReach; dx <= CellReach; ++dx)
	{
		for (int32 dy = -CellReach; dy <= CellReach; ++dy)
		{
			const TArray<int32>* Ids = SpatialHash.Find(FIntVector(Base.X + dx, Base.Y + dy, 0));
			if (!Ids) { continue; }
			for (int32 Id : *Ids)
			{
				const FAffordanceHandle& H = Affordances[Id];
				if (H.Type != Type || !IsLive(Id)) { continue; }
				if (H.ReservedBy != INDEX_NONE && H.ReservedByActor.IsValid()) { continue; }
				if (FVector::DistSquared(Center, H.Location) <= RadiusSq)
				{
					OutLocations.Add(H.Location);
				}
			}
		}
	}
}

float UWorldAffordanceSubsystem::GetShelterColdDampenAt(const FVector& Location) const
{
	const float Radius = ShelterEffectRadius;
	const float RadiusSq = Radius * Radius;
	const int32 CellReach = (CellSize > KINDA_SMALL_NUMBER) ? FMath::CeilToInt(Radius / CellSize) : 0;
	const FIntVector Base = ToCell(Location);

	float Best = 0.f;   // strongest covering shelter wins
	for (int32 dx = -CellReach; dx <= CellReach; ++dx)
	{
		for (int32 dy = -CellReach; dy <= CellReach; ++dy)
		{
			const TArray<int32>* Ids = SpatialHash.Find(FIntVector(Base.X + dx, Base.Y + dy, 0));
			if (!Ids) { continue; }
			for (int32 Id : *Ids)
			{
				const FAffordanceHandle& H = Affordances[Id];
				if (H.Type != EAffordanceType::Shelter || H.Owner.IsStale(true)) { continue; }
				if (FVector::DistSquared(Location, H.Location) <= RadiusSq)
				{
					Best = FMath::Max(Best, H.ColdDampenFactor);
				}
			}
		}
	}
	return FMath::Clamp(Best, 0.f, 1.f);
}

void UWorldAffordanceSubsystem::RegenTick()
{
	// No world clock in Slice 1 (audit §6.1) -> interpret RegenPerHour against real seconds elapsed.
	const float GameHours = RegenInterval / 3600.f;
	for (FAffordanceHandle& H : Affordances)
	{
		if (H.Type == EAffordanceType::None) { continue; }

		// Dead-reserver claim release is LAZY in QueryNearest/TryReserve; ACTIVELY prune the stale field here
		// on the existing walk (no new per-NPC scan) so a reserver that died without Release frees the slot.
		if (H.ReservedBy != INDEX_NONE && !H.ReservedByActor.IsValid())
		{
			UE_LOG(LogWorldAffordance, Verbose, TEXT("[Affordance] Released claim id=%d — reserver dead/invalid"), static_cast<int32>(&H - Affordances.GetData()));
			H.ReservedBy = INDEX_NONE;
			H.ReservedByActor = nullptr;
		}

		if (H.RegenPerHour <= 0.f) { continue; }
		if (H.RemainingYield >= H.MaxYield) { continue; }
		H.RemainingYield = FMath::Min(H.MaxYield, H.RemainingYield + H.RegenPerHour * GameHours);
		if (H.RemainingYield > KINDA_SMALL_NUMBER) { H.bExhaustLogged = false; } // regrew -> allow exhaust log again
	}
}
