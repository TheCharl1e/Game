#include "CorpseBase.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

ACorpseBase::ACorpseBase()
{
    // ZERO TIKANIA - Klucz do 500 ciał na mapie bez spadku FPS
    PrimaryActorTick.bCanEverTick = false;

    // Tworzymy fizyczny model ciała
    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Optymalizacja: domyślnie wyłączamy fizykę. Blueprint włączy ją tylko na moment upadku.
    MeshComponent->SetSimulatePhysics(false);
    MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

    // Pełnowymiarowy dorosły NPC to sporo kalorii
    AvailableKcal = 1500.0f;

    // Np. 20 minut czasu rzeczywistego (1 dzień w grze), zanim ciało zgnije
    RotsInSeconds = 1200.0f;
}

void ACorpseBase::BeginPlay()
{
    Super::BeginPlay();

    // Uruchamiamy jednorazowy timer rozkładu. Silnik sam to odliczy w tle.
    GetWorld()->GetTimerManager().SetTimer(
        RotTimerHandle,
        this,
        &ACorpseBase::ProcessRotting,
        RotsInSeconds,
        false
    );
}

float ACorpseBase::ExtractMeat(float RequestedKcal)
{
    if (AvailableKcal <= 0.0f)
    {
        return 0.0f;
    }

    // Pobieramy tyle mięsa, ile chcemy, chyba że w ciele jest już mniej
    float ExtractedAmount = FMath::Min(RequestedKcal, AvailableKcal);
    AvailableKcal -= ExtractedAmount;

    // Jeśli zjedzono wszystko, wysyłamy sygnał do Blueprinta (żeby np. zniszczyć obiekt i zostawić sam szkielet/krew)
    if (AvailableKcal <= 0.0f)
    {
        OnCorpseDepleted();
    }

    return ExtractedAmount;
}

void ACorpseBase::ProcessRotting()
{
    // Mięso znika, ciało staje się bezwartościowe
    AvailableKcal = 0.0f;

    // Sygnał do BP: można tu podpiąć Particle much lub zmianę tekstury na zgniłą
    OnCorpseRotten();
}