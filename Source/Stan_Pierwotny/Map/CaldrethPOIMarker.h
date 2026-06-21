// CaldrethPOIMarker.h
// A single point-of-interest landmark placed on the Caldreth island (Caldera, Great Tree,
// Obsidian Fields, The Grey Spring, River Source, ...).
//
// Data marker only (no mesh, no Tick): the C++ "brain" holds the manifest fields and pushes
// the gameplay "role" onto the actor Tags so systems can find a landmark by tag without a cast.
// Visuals are a Blueprint "body" concern — subclass this and add a mesh/icon if needed.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaldrethPOIMarker.generated.h"

/**
 * One points_of_interest entry from caldreth_manifest.json, placed in the world.
 * Fields are free-form (FName/FString), not an enum, so new POIs in the manifest need
 * no C++ change — stays data-driven like the zone layer.
 */
UCLASS()
class STAN_PIERWOTNY_API ACaldrethPOIMarker : public AActor
{
	GENERATED_BODY()

public:
	ACaldrethPOIMarker();

	/** Human-readable POI name from the manifest (e.g. "The Grey Spring"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caldreth|POI")
	FString PoiName;

	/** Manifest "kind" (caldera, tree, obsidian, grey, spring). Free-form for data-driven growth. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caldreth|POI")
	FName Kind;

	/**
	 * Gameplay role from the manifest (e.g. "FireGate_TrueName_L5"). Also pushed onto the actor Tags.
	 * Named PoiRole (not Role) to avoid shadowing the inherited AActor network-role member.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caldreth|POI")
	FName PoiRole;

	/** Push PoiRole onto the actor Tags array (idempotent, no-op if PoiRole is None or already present). */
	UFUNCTION(BlueprintCallable, Category = "Caldreth|POI")
	void ApplyRoleTag();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
