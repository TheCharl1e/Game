// CaldrethZone.cpp

#include "CaldrethZone.h"
#include "Components/SceneComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"   // TActorIterator
#include "WorldAffordanceSubsystem.h"   // L0-TA-S1: self-register Shelter affordance for Safe Zones

DEFINE_LOG_CATEGORY(LogCaldreth);

namespace
{
	// Even-odd ray-cast point-in-polygon. Works for the current axis-aligned bbox outline and for a
	// true (possibly concave) boundary trace later — no change to callers when outlines get sharper.
	bool PointInPolygon(const TArray<FVector2D>& Poly, const FVector2D& P)
	{
		const int32 N = Poly.Num();
		if (N < 3)
		{
			return false;
		}
		bool bInside = false;
		for (int32 i = 0, j = N - 1; i < N; j = i++)
		{
			const FVector2D& A = Poly[i];
			const FVector2D& B = Poly[j];
			if (((A.Y > P.Y) != (B.Y > P.Y)) &&
				(P.X < (B.X - A.X) * (P.Y - A.Y) / (B.Y - A.Y) + A.X))
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}
}

ACaldrethZone::ACaldrethZone()
{
	// Pure data marker: no Tick, no movement. Cheap to have hundreds of these.
	PrimaryActorTick.bCanEverTick = false;

	// A bare scene root so the zone has a placeable transform (its centroid in the world).
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void ACaldrethZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Resolve in the editor too, so IsSpawnable/IsHabitable answer correctly before PIE
	// and the import tool / debug draw can read DebugColor straight after placement.
	ResolveZoneDef();
}

void ACaldrethZone::BeginPlay()
{
	Super::BeginPlay();

	// Re-resolve at runtime in case the ZoneTable reference was set after construction.
	ResolveZoneDef();

	// L0-TA-S1: a Safe Zone is an emergent Shelter affordance (decision #3). Self-register one record at
	// the zone centroid so EQS_FindSafeZone (affordance generator, type Shelter) can find it. The authored
	// ShelterColdDampenFactor seeds the record's read-side cold-deficit dampen.
	if (bIsSafeZone)
	{
		if (UWorld* World = GetWorld())
		{
			if (UWorldAffordanceSubsystem* Affordances = World->GetSubsystem<UWorldAffordanceSubsystem>())
			{
				ShelterAffordanceId = Affordances->RegisterAffordanceSimple(
					EAffordanceType::Shelter, GetActorLocation(), /*Yield*/ 0.f, /*RegenPerHour*/ 0.f,
					/*Owner*/ this, /*ColdDampenFactor*/ ShelterColdDampenFactor);
			}
		}
	}
}

void ACaldrethZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Fail-safe: a destroyed Safe Zone leaves no ghost Shelter affordance.
	if (ShelterAffordanceId != INDEX_NONE)
	{
		if (UWorld* World = GetWorld())
		{
			if (UWorldAffordanceSubsystem* Affordances = World->GetSubsystem<UWorldAffordanceSubsystem>())
			{
				Affordances->UnregisterAffordance(ShelterAffordanceId);
			}
		}
		ShelterAffordanceId = INDEX_NONE;
	}

	Super::EndPlay(EndPlayReason);
}

void ACaldrethZone::RegisterMember(AActor* Member)
{
	if (!IsValid(Member)) { return; }

	// Prune stale weak refs, then add (deduped).
	AssignedMembers.RemoveAll([](const TWeakObjectPtr<AActor>& M) { return !M.IsValid(); });
	if (!AssignedMembers.ContainsByPredicate([Member](const TWeakObjectPtr<AActor>& M) { return M.Get() == Member; }))
	{
		AssignedMembers.Add(Member);
	}
}

void ACaldrethZone::ResolveZoneDef()
{
	bZoneDefResolved = false;
	CachedDef = FZoneDef();

	if (!IsValid(ZoneTable))
	{
		UE_LOG(LogCaldreth, Warning,
			TEXT("ACaldrethZone '%s': ZoneTable not set — IsSpawnable/IsHabitable default to false."),
			*GetName());
		return;
	}

	const FName RowName = ZoneTypeToRowName(ZoneType);
	static const FString Context(TEXT("ACaldrethZone::ResolveZoneDef"));
	const FZoneDef* Row = ZoneTable->FindRow<FZoneDef>(RowName, Context, /*bWarnIfRowMissing*/ false);
	if (!Row)
	{
		UE_LOG(LogCaldreth, Warning,
			TEXT("ACaldrethZone '%s': no FZoneDef row '%s' in DataTable '%s' — flags default to false."),
			*GetName(), *RowName.ToString(), *ZoneTable->GetName());
		return;
	}

	CachedDef = *Row;
	bZoneDefResolved = true;

	UE_LOG(LogCaldreth, Verbose,
		TEXT("ACaldrethZone '%s': resolved zone '%s' (Spawnable=%d, Habitable=%d)."),
		*GetName(), *RowName.ToString(), CachedDef.bSpawnable ? 1 : 0, CachedDef.bHabitable ? 1 : 0);
}

bool ACaldrethZone::IsSpawnable() const
{
	// Fast path: read the cached row. Unresolved (missing table/row) is treated as "no".
	return bZoneDefResolved && CachedDef.bSpawnable;
}

bool ACaldrethZone::IsHabitable() const
{
	return bZoneDefResolved && CachedDef.bHabitable;
}

bool ACaldrethZone::GetZoneDef(FZoneDef& OutDef) const
{
	OutDef = CachedDef;
	return bZoneDefResolved;
}

FName ACaldrethZone::ZoneTypeToRowName(EZoneType InZoneType)
{
	const UEnum* Enum = StaticEnum<EZoneType>();
	if (!Enum)
	{
		// Should never happen for a UENUM, but never trust a raw pointer.
		UE_LOG(LogCaldreth, Error, TEXT("ZoneTypeToRowName: StaticEnum<EZoneType>() is null."));
		return NAME_None;
	}

	// GetNameStringByValue yields "EZoneType::Ocean"; the DataTable row is the short name.
	FString Name = Enum->GetNameStringByValue(static_cast<int64>(InZoneType));
	int32 ColonIdx = INDEX_NONE;
	if (Name.FindLastChar(TEXT(':'), ColonIdx))
	{
		Name = Name.RightChop(ColonIdx + 1);
	}
	return FName(*Name);
}

bool ACaldrethZone::ContainsWorldLocation(const FVector& WorldLocation) const
{
	if (NormalizedOutline.Num() < 3 || WorldSizeUU <= 0.f)
	{
		return false;
	}

	// Inverse of the importer's placement: world = (n - 0.5) * WorldSizeUU  ->  n = world/WorldSizeUU + 0.5.
	const FVector2D P(
		WorldLocation.X / WorldSizeUU + 0.5f,
		WorldLocation.Y / WorldSizeUU + 0.5f);

	return PointInPolygon(NormalizedOutline, P);
}

float ACaldrethZone::GetNormalizedArea() const
{
	const int32 N = NormalizedOutline.Num();
	if (N < 3)
	{
		return 0.f;
	}

	// Shoelace formula (absolute), accumulated in double to keep tiny normalized regions precise.
	double Area = 0.0;
	for (int32 i = 0, j = N - 1; i < N; j = i++)
	{
		Area += (double)(NormalizedOutline[j].X + NormalizedOutline[i].X) *
				(double)(NormalizedOutline[j].Y - NormalizedOutline[i].Y);
	}
	return (float)(FMath::Abs(Area) * 0.5);
}

ACaldrethZone* ACaldrethZone::GetZoneAtLocation(const UObject* WorldContextObject, const FVector& Location)
{
	UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
		: nullptr;
	if (!IsValid(World))
	{
		UE_LOG(LogCaldreth, Warning, TEXT("GetZoneAtLocation: no valid world from context object."));
		return nullptr;
	}

	// Linear scan over placed zones. Most-specific (smallest area) containing zone wins, so the giant
	// Ocean bbox never shadows a Beach/Oasis sitting inside it. Call on demand, never per-frame.
	// PERF TODO: replace TActorIterator with spatial index + per-NPC cache when NPC consumer exists (see plan).
	// TActorIterator scans ALL level actors per call — fine at ~18 zones with no NPC consumer, but it is the
	// O(total actors) hot path that the "no per-query full scan" rule forbids at 500 NPC. Plan: build-once
	// UCaldrethZoneWorldSubsystem grid-bucket index (O(1)+~3 tests) + cache "my zone" on each NPC (~99% hits).
	// Full reasoning in DESIGN_how_it_works.md → "Strefy koncentryczne" → query-perf plan.
	ACaldrethZone* Best = nullptr;
	float BestArea = TNumericLimits<float>::Max();
	for (TActorIterator<ACaldrethZone> It(World); It; ++It)
	{
		ACaldrethZone* Zone = *It;
		if (!IsValid(Zone) || !Zone->ContainsWorldLocation(Location))
		{
			continue;
		}
		const float Area = Zone->GetNormalizedArea();
		if (Area < BestArea)
		{
			BestArea = Area;
			Best = Zone;
		}
	}

	UE_LOG(LogCaldreth, Verbose, TEXT("GetZoneAtLocation (%.0f,%.0f): %s"),
		Location.X, Location.Y,
		Best ? *ZoneTypeToRowName(Best->ZoneType).ToString() : TEXT("none"));

	return Best;
}

EZoneType ACaldrethZone::GetZoneTypeAtLocation(const UObject* WorldContextObject, const FVector& Location)
{
	const ACaldrethZone* Zone = GetZoneAtLocation(WorldContextObject, Location);
	return Zone ? Zone->ZoneType : EZoneType::Ocean;
}
