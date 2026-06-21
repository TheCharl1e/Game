// CaldrethPOIMarker.cpp

#include "CaldrethPOIMarker.h"
#include "Components/SceneComponent.h"
#include "CaldrethZone.h"   // reuse the map layer's LogCaldreth category

ACaldrethPOIMarker::ACaldrethPOIMarker()
{
	// Pure data marker: no Tick, no movement. Cheap landmark the import tool places once.
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void ACaldrethPOIMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Keep Tags in sync if Role was edited in the details panel; the importer also calls this
	// explicitly after setting Role (OnConstruction during SpawnActor runs before Role is set).
	ApplyRoleTag();
}

void ACaldrethPOIMarker::ApplyRoleTag()
{
	if (PoiRole.IsNone())
	{
		return;
	}
	if (!Tags.Contains(PoiRole))
	{
		Tags.Add(PoiRole);
		UE_LOG(LogCaldreth, Verbose, TEXT("ACaldrethPOIMarker '%s': tagged with role '%s'."),
			*GetName(), *PoiRole.ToString());
	}
}
