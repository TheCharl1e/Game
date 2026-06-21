#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CorpseBase.generated.h"

UCLASS()
class STAN_PIERWOTNY_API ACorpseBase : public AActor
{
    GENERATED_BODY()

public:
    ACorpseBase();

protected:
    virtual void BeginPlay() override;

public:
    // Główny model ciała
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Corpse|Components")
    class USkeletalMeshComponent* MeshComponent;

    // Ile kalorii (mięsa) zostało w ciele
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corpse|Resources")
    float AvailableKcal;

    // Czas do całkowitego zgnicia (w sekundach)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corpse|Resources")
    float RotsInSeconds;

    // Funkcja dla drapieżników/NPC do wyciągania pożywienia
    UFUNCTION(BlueprintCallable, Category = "Corpse|Interaction")
    float ExtractMeat(float RequestedKcal);

    // Zdarzenia do obsłużenia w Blueprincie
    UFUNCTION(BlueprintImplementableEvent, Category = "Corpse|Events")
    void OnCorpseDepleted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Corpse|Events")
    void OnCorpseRotten();

private:
    FTimerHandle RotTimerHandle;

    UFUNCTION()
    void ProcessRotting();
};