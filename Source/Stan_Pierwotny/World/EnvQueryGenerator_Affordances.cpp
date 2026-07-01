// EnvQueryGenerator_Affordances.cpp
#include "EnvQueryGenerator_Affordances.h"
#include "WorldAffordanceSubsystem.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UEnvQueryGenerator_Affordances::UEnvQueryGenerator_Affordances()
{
	// Items are world points; EQS contexts/tests (distance, pathfinding) operate on them.
	ItemType = UEnvQueryItemType_Point::StaticClass();
}

void UEnvQueryGenerator_Affordances::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	// Derive world + center from the querier owner (robust across engine versions).
	UObject* OwnerOb = QueryInstance.Owner.Get();
	const AActor* Querier = Cast<AActor>(OwnerOb);
	UWorld* World = Querier ? Querier->GetWorld() : (OwnerOb ? OwnerOb->GetWorld() : nullptr);
	if (!World) { return; }

	const UWorldAffordanceSubsystem* Affordances = World->GetSubsystem<UWorldAffordanceSubsystem>();
	if (!Affordances) { return; }

	const FVector Center = Querier ? Querier->GetActorLocation() : FVector::ZeroVector;

	TArray<FVector> Points;
	Affordances->GatherAffordanceLocations(AffordanceType, Center, SearchRadius, Points);

	for (const FVector& P : Points)
	{
		QueryInstance.AddItemData<UEnvQueryItemType_Point>(P);
	}
}

FText UEnvQueryGenerator_Affordances::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Affordances (Caldreth)"));
}

FText UEnvQueryGenerator_Affordances::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("Type %d within %.0f uu of querier"),
		(int32)AffordanceType, SearchRadius));
}
