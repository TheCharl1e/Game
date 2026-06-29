#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemBase.generated.h"

// Enum zdefiniowany według ustaleń z GDD (Opcje własności dla Drzewa Zachowań)
UENUM(BlueprintType)
enum class EItemOwnership : uint8
{
    Public      UMETA(DisplayName = "Public (Pantry/Ground)"),
    Private     UMETA(DisplayName = "Private (NPC Pocket)"),
    PlayerOwned UMETA(DisplayName = "Player Owned (Do Not Touch!)")
};

UCLASS()
class STAN_PIERWOTNY_API AItemBase : public AActor
{
    GENERATED_BODY()
    
public:	
    AItemBase();

protected:
    virtual void BeginPlay() override;

public:
    // Wizualna reprezentacja przedmiotu (jabłko, kawałek mięsa, jagody)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* MeshComponent;

    // ==== WŁASNOŚĆ I DYSTRYBUCJA ====
    // Identity of the owner. -1 = public domain (ground / storehouse).
    // Intentionally an int32, NOT AActor* — owners die; IDs don't.
    // Mirrors PUBLIC_OWNER_ID in InventoryComponent.h (keep in sync).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ownership")
    int32 OwnerID = -1;

    // Ownership state used by the Behavior Tree for quick branching.
    // Derived from OwnerID — set via SetOwnership(), never independently.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ownership")
    EItemOwnership OwnershipState = EItemOwnership::Public;

    // ==== DANE PRZEDMIOTU ====

    // Nazwa wiersza z naszej nowej tabeli DT_FoodStats (łączy dany item na mapie ze statystykami EUREKA)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    FName FoodTableRowName;

    // APPETITE/GRUBAS slice 1 — mechanika nadgryzienia. 1.0 = całe, 0.0 = zjedzone. Każdy kęs zdejmuje
    // część (ConsumePortion). Nadgryzione jedzenie ZOSTAJE w świecie (NPC przerwał posiłek). Mirror ACorpseBase.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    float RemainingPortion = 1.0f;

    // ==== FUNKCJE DLA DRZEWA ZACHOWAŃ ====

    // Zdejmuje część porcji (jeden kęs). Zwraca FAKTYCZNIE pobraną część (≤ requested, ≤ remaining).
    // Mirror ACorpseBase::ExtractMeat: przy wyczerpaniu odpala OnItemDepleted (BP może zniszczyć aktora).
    UFUNCTION(BlueprintCallable, Category = "Item Data")
    float ConsumePortion(float RequestedPortion);

    // BP reaguje na zjedzenie do zera (np. zniszcz aktora / zostaw ogryzek).
    UFUNCTION(BlueprintImplementableEvent, Category = "Item Data")
    void OnItemDepleted();

    // Sets ownership. Pass -1 as InOwnerID for public items (dropped on ground,
    // storehouse goods). NPC identity comes from the NPC Registry (int32 ID).
    UFUNCTION(BlueprintCallable, Category = "Ownership")
    void SetOwnership(EItemOwnership NewState, int32 InOwnerID = -1);

    // Returns true if RequesterID can eat this without committing theft.
    // True when: item is Public OR the requester IS the owner.
    UFUNCTION(BlueprintPure, Category = "Ownership")
    bool CanBeEatenBy(int32 RequesterID) const;

    // Returns true if taking this item would count as theft.
    // Feeds the L5 detective / alibi system.
    UFUNCTION(BlueprintPure, Category = "Ownership")
    bool IsStolenBy(int32 RequesterID) const;
};