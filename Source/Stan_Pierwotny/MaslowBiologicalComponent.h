#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "MaslowBiologicalComponent.generated.h"

// Dedicated log category for the Maslow biology / sleep engine.
DECLARE_LOG_CATEGORY_EXTERN(LogMaslow, Log, All);

class ACaldrethZone;   // AmbientTemp: cache strefy (perf #1)
class AItemBase;       // APPETITE slice 1b: StartEatingItem resolves food item → FFoodItemRow

// Definicja struktury dla DT_ActionCosts (Row Struct).
// Nazwa wiersza = identyfikator akcji (np. Action.Idle, Action.Work.Woodcutting),
// dlatego nie dublujemy go osobną kolumną ActionName.
USTRUCT(BlueprintType)
struct FActionCostRow : public FTableRowBase
{
    GENERATED_BODY()

    // Mnożnik spalania kalorii podczas akcji (Idle=1.0, Combat=5.0). Mnoży BaseBurnRate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    float KcalMultiplier = 1.0f;

    // Mnożnik spalania wody podczas akcji (Idle=1.0, Combat=5.0). Mnoży HydrationBurnRatePerTick.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    float WaterMultiplier = 1.0f;

    // Dodatkowy koszt kaloryczny przy zimnie (doliczany do mnożnika temperatury).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    float TempModifier = 0.0f;

    // Koszt staminy na sekundę podczas akcji (z modelu ST_ActionCost). 0 = brak.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    float StaminaCostPerSecond = 0.0f;

    // Czy koszt akcji skaluje się z dystansem (np. ruch). Metadana dla warstwy ruchu/AI.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    bool bIsDistanceBased = false;

    // Czy akcja jest natychmiastowa (jednorazowy koszt, nie ciągły). Metadana dla AI.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    bool bIsInstant = false;
};

// Definicja struktury dla DT_FoodStats - wartości pożywienia i ryzyko (spec architekta).
USTRUCT(BlueprintType)
struct FFoodItemRow : public FTableRowBase
{
    GENERATED_BODY()

    // Energia jedzenia (kcal) -> szybka energia (Glucose) i Glikogen. KANAŁ WĘGLOWODANÓW (carb) dla routingu makr.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Macros")
    float NutritionKcal = 0.0f;

    // Objętość/masa pokarmu -> napędza StomachFill (sytość). NIEZALEŻNE od kcal (D-VOLUME-KCAL):
    // gęste tłuste = mały Volume + duże FatG (tuczy, mało syci); bujne nisko-kal = duży Volume, małe makra.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Macros")
    float Volume = 1.0f;

    // Białko (g) -> odbudowa mięśni (MaxHP) po autofagii.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Macros")
    float ProteinG = 0.0f;

    // Tłuszcz (g) -> uzupełnienie BodyFat.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Macros")
    float FatG = 0.0f;

    // Nawodnienie przy jedzeniu (ml) -> dodawane do CurrentHydration.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
    float WaterMl = 0.0f;

    // Szansa na zatrucie (0.0 - 1.0). Zatrucie = Woda spada 3x szybciej. Dotyczy surowego jedzenia.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Risks")
    float PoisonChance = 0.0f;

    // Szansa na halucynacje (0.0 - 1.0). AI zacznie biegać losowo zamiast pracować.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Risks")
    float HallucinationChance = 0.0f;

    // Surowe = ryzyko (PoisonChance aktywne); ugotowane = zero. Metadana dla systemu gotowania/AI.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Cooking")
    bool bRequiresCooking = false;

    // Bonus morale (L3/L4) za ulubione jedzenie. > 0 nadaje status Wysokiego Morale.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Benefits")
    float MoraleBonus = 0.0f;

    // Czy psuje się po N godzinach.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Spoilage")
    bool bIsPerishable = false;
};

// APPETITE/GRUBAS slice 1: powód zamknięcia sesji jedzenia (StopEating). BP gra reakcję (beknięcie/odłożenie).
UENUM(BlueprintType)
enum class EEatStopReason : uint8
{
    Full           UMETA(DisplayName = "Full (satiety reached)"),        // StomachFill >= SatietySetpoint
    Finished       UMETA(DisplayName = "Finished (food gone)"),          // RemainingBites == 0
    Interrupted    UMETA(DisplayName = "Interrupted (forced)"),          // puść guzik / panika L0 / wróg
    SourceGone     UMETA(DisplayName = "Source gone (P2P stole/destroyed)"), // EatTargetFood stale
    Incapacitated  UMETA(DisplayName = "Incapacitated (collapse/microsleep)")
};

// Zdefiniowane Fazy Głodowania dla Drzewa Zachowań i UI (Zaktualizowane o nowy system)
UENUM(BlueprintType)
enum class EHungerPhase : uint8
{
    Phase_0_Glucose   UMETA(DisplayName = "Glucose Burn (Phase 0)"),
    Phase_1_Glycogen  UMETA(DisplayName = "Glycogen Burn (Phase 1)"),
    Phase_2_FatBurn   UMETA(DisplayName = "Fat Burn (Phase 2)"),
    Phase_3_Autophagy UMETA(DisplayName = "Autophagy/Desperation (Phase 3)"),
    Phase_4_Death     UMETA(DisplayName = "Death (Phase 4)")
};

// Lista rozwijana priorytetów (Będzie widoczna w Blueprintach i Blackboardzie)
UENUM(BlueprintType)
enum class EMaslowPriority : uint8
{
    Level_0_FightOrFlight UMETA(DisplayName = "Fight or Flight (Panic)"),
    Level_1_Temperature   UMETA(DisplayName = "Physiological: Temperature"),
    Level_1_Hydration     UMETA(DisplayName = "Physiological: Hydration"),
    Level_1_Rest          UMETA(DisplayName = "Physiological: Rest/Sleep"),
    Level_1_Nutrition     UMETA(DisplayName = "Physiological: Hunger"),
    Level_2_Safety        UMETA(DisplayName = "Safety (Shelter/Weapons)"),
    Satisfied             UMETA(DisplayName = "All Needs Met (Idle/Social)")
};

// TASK 3 (flee, plaster #4): formy zagrożenia, OD którego NPC ucieka. Generyczne — damage-hook ustawia Damage;
// percepcja predatora/wroga (L0-04) ustawi Predator/HostileHuman PRZEZ TO SAMO SetThreat. Flee = ten sam ruch
// (od GetThreatLocation()); typ jest pod przyszłe różnicowanie reakcji (np. człowiek → walcz-lub-uciekaj wg OCEAN).
UENUM(BlueprintType)
enum class EThreatType : uint8
{
    None          UMETA(DisplayName = "None"),
    Damage        UMETA(DisplayName = "Damage (got hit)"),
    Predator      UMETA(DisplayName = "Predator (animal)"),
    HostileHuman  UMETA(DisplayName = "Hostile human")
};

// Drabina zmęczenia z HoursAwake (progi 16/20/24h). Ortogonalna do bIsRested (buff po śnie).
UENUM(BlueprintType)
enum class EFatigueState : uint8
{
    Awake        UMETA(DisplayName = "Awake (normal, <16h)"),
    MentalFog    UMETA(DisplayName = "Mental Fog (>=16h)"),
    Microsleeps  UMETA(DisplayName = "Microsleeps (>=20h)"),
    Collapsed    UMETA(DisplayName = "Collapsed (>=24h)")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class STAN_PIERWOTNY_API UMaslowBiologicalComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMaslowBiologicalComponent();

protected:
    virtual void BeginPlay() override;
    // EC-1 fail-safe: martwy NPC nie zostawia wiszącego timera ani flag blokady (krytyczne przy 500 NPC).
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Pętla spalania (wywoływana asynchronicznie przez Timer np. co 10s)
    UFUNCTION()
    void ProcessMetabolism();

    FTimerHandle MetabolismTimerHandle;

    /** Interwał timera metabolizmu (sekundy, czas rzeczywisty). Narastanie HoursAwake i wyliczenie
     *  AwakeRatePerTick z zegara świata używają tej samej wartości, by były spójne z timerem. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Metabolism")
    float MetabolismInterval = 10.0f;

    /** Dolicza HoursAwake (× ActivityMultiplier) i przelicza FatigueState. Wołane co tick. */
    void UpdateFatigue();

    /** Opcja A: jednorazowo wylicza AwakeRatePerTick z TimeSpeed zegara świata (BP_DayNightCycle).
     *  Fallback na wartość UPROPERTY, gdy brak zegara / property. */
    void ResolveAwakeRateFromWorldClock();

    /** Gating: ResolveAwakeRateFromWorldClock ma się wykonać raz (na 1. ticku, po wszystkich BeginPlay). */
    bool bAwakeRateResolved = false;

    // --- ETAP 2: helpery skutków zmęczenia/snu ---
    /** Reaguje na zmianę FatigueState: eventy mgły (start/end), omdlenie (stop BT + ragdoll BP). */
    void HandleFatigueTransition(EFatigueState PrevState, EFatigueState NewState);
    /** Wyzwala mikrosen (event BP + timer trwania). */
    void TriggerMicrosleep();
    /** Koniec mikrosnu (callback timera). */
    void EndMicrosleep();
    FTimerHandle MicrosleepTimer;

    // --- AmbientTemp: cache strefy (perf #1, CaldrethZone.cpp:180 PERF TODO) ---
    /** Cache strefy, w której stoi NPC. Odświeżany RZADKO (próg dystansu), NIE co tick. */
    TWeakObjectPtr<ACaldrethZone> CurrentZone;
    /** Pozycja ostatniego sprawdzenia strefy — refresh dopiero po ruchu > ZoneRecheckDistance. */
    FVector LastZoneCheckPos = FVector::ZeroVector;
    /** Czy cache był już raz zainicjowany (pierwszy tick zawsze odpytuje). */
    bool bZoneCacheInitialized = false;
    /** Gating logu hipotermii — żeby [Temp] HIPOTERMIA logował raz na wejście poniżej progu (histereza). */
    bool bHypothermiaLogged = false;
    /** Odśwież CurrentZone, jeśli NPC ruszył > próg lub cache nieważny. Woła GetZoneAtLocation tylko wtedy. */
    void RefreshZoneCache();

    // --- L0 Track A / Slice 1: read-side shelter cache (decision #3) ---
    /** Strongest Shelter cold-dampen (0..1) covering the NPC, refreshed on the metabolism cadence.
     *  Consumed ONLY by GetFeltTemperature(). GUARDRAIL: never feeds the ambient→body coupling. */
    float CurrentShelterColdDampen = 0.f;
    /** Pull CurrentShelterColdDampen from UWorldAffordanceSubsystem at the owner's location (cadence-rate). */
    void RefreshShelterCache();

    /** Cache wskaźnika zegara świata (BP_DayNightCycle) — ustawiany raz w ResolveAwakeRateFromWorldClock. */
    TWeakObjectPtr<AActor> WorldClock;
    /** Liczy DayNightTempOffset z nasłonecznienia zegara (SunFactor = SunIntensity/MaxSunIntensity). */
    void UpdateDayNightTempOffset();

public:
    // ==== BIOLOGIA I METABOLIZM (Matryca Spalania) ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Metabolism")
    float Glucose; // Cukier we krwi (szybka energia)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Metabolism")
    float MaxGlucose;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Metabolism")
    float GlycogenReserves; // Zapas na kilka godzin

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Metabolism")
    float MaxGlycogen;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Metabolism")
    float BodyFat; // Warstwa tłuszczu (Zapasowy bank)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Metabolism")
    float MaxBodyFat;

    // --- Sleep / Adenosine model ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float HoursAwake = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float MaxHoursBeforeCollapse = 24.0f;

    // ==== SLEEP ENGINE (L1-06/07) — narastanie zmęczenia ====

    /** Godziny gry doliczane do HoursAwake na 1 tick metabolizmu przy Idle (ActivityMultiplier 1.0).
     *  FALLBACK: przy BeginPlay silnik auto-wylicza to z zegara świata (BP_DayNightCycle TimeSpeed ×
     *  interwał timera), więc zegar snu NIE rozjeżdża się z zegarem dobowym ("jeden zegar"). Wartość tu
     *  jest używana tylko, gdy brak zegara w świecie. Default 1.6667 = (1/6) × 10s dla obecnego tempa. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float AwakeRatePerTick = 1.6667f;

    /** HoursAwake, przy którym zaczyna się mgła umysłowa (−wydajność pracy). [approved 16h] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float MentalFogThreshold = 16.0f;

    /** HoursAwake, przy którym zaczynają się losowe mikrosny. [approved 20h] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float MicrosleepThreshold = 20.0f;

    /** HoursAwake, przy którym NPC omdlewa (ragdoll). [approved 24h] Lustro MaxHoursBeforeCollapse. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float CollapseThreshold = 24.0f;

    /** Bieżący stan zmęczenia, przeliczany co tick z HoursAwake. Czytany przez AI/UI. */
    UPROPERTY(BlueprintReadOnly, Category = "Biology|State")
    EFatigueState FatigueState = EFatigueState::Awake;

    /** Buff „Rested" (+praca/Eureka) po dobrym śnie. ORTOGONALNY do FatigueState; ustawiany przez
     *  ścieżkę resetu snu (kolejny ETAP). Tu zadeklarowany, by drabina i buff były rozdzielone. */
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Sleep")
    bool bIsRested = false;

    /** Bieżący stan zmęczenia (tani odczyt dla BT/EQS/HUD). */
    UFUNCTION(BlueprintPure, Category = "AI|Maslow|Sleep")
    EFatigueState GetFatigueState() const { return FatigueState; }

    // ==== SLEEP ENGINE ETAP 2 — skutki drabiny + temperatura + sen/reset ====

    /** Mnożnik wydajności pracy: 1.0 do MentalFog (16h), liniowo do 0.7 przy Collapse (24h);
     *  + RestedWorkBonus gdy bIsRested. Crafting/akcje mają to czytać. */
    UFUNCTION(BlueprintPure, Category = "AI|Maslow|Sleep")
    float GetWorkEfficiencyMultiplier() const;

    /** NPC unieruchomiony omdleniem (≥24h) — BT zatrzymany, ragdoll. Czyszczone przez StopSleep. */
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Sleep")
    bool bIncapacitated = false;

    /** Trwa mikrosen (NPC „zamarł" na ~1-2s). Transient. */
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Sleep")
    bool bMicrosleeping = false;

    /** Szansa na mikrosen na 1 tick, gdy HoursAwake ≥ MicrosleepThreshold. [tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float MicrosleepChancePerTick = 0.15f;

    /** Czas trwania pojedynczego mikrosnu (sekundy). [approved ~1-2s] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float MicrosleepDuration = 1.5f;

    /** Czy NPC śpi (ustawiane przez BT via StartSleep/StopSleep). Gdy true: HoursAwake MALEJE,
     *  a termoregulacja niemal wyłączona (most temperatury, ColdSleepMultiplier). */
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Sleep")
    bool bIsSleeping = false;

    /** Ile „godzin zmęczenia" regeneruje 1 tick snu w komforcie (× TempQualityMultiplier). [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float SleepRecoveryPerTick = 4.0f;

    /** Dolny próg strefy komfortu cieplnego (°C). W [Min,Max] jakość snu = 1.0. [approved 15] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float ComfortTempMin = 15.0f;

    /** Górny próg strefy komfortu cieplnego (°C). [approved 24] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float ComfortTempMax = 24.0f;

    /** Bonus do GetWorkEfficiencyMultiplier z buffa Rested. [approved +0.2] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float RestedWorkBonus = 0.2f;

    /** Mnożnik spalania zimna WE ŚNIE (zamiast jawnego ×2 — brak drżenia/termoregulacji). [approved 3.5] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Sleep")
    float ColdSleepMultiplier = 3.5f;

    /** API dla BT: NPC kładzie się spać. HOOK — decyzja KIEDY to BT/L3 (Safe Zone), nie ten silnik. */
    UFUNCTION(BlueprintCallable, Category = "AI|Maslow|Sleep")
    void StartSleep();

    /** API dla BT: przebudzenie. Pełny sen w komforcie (HoursAwake→0) → buff Rested; znosi omdlenie + wznawia BT. */
    UFUNCTION(BlueprintCallable, Category = "AI|Maslow|Sleep")
    void StopSleep();

    /** Jakość snu z CurrentTemp: 1.0 w komforcie [Min,Max], <1.0 poza (sen płytki, podłoga 0.25). */
    UFUNCTION(BlueprintPure, Category = "AI|Maslow|Sleep")
    float GetTempQualityMultiplier() const;

    // --- Eventy BP (C++ liczy, BP rysuje wizual) ---
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events|Sleep")
    void OnMentalFogStart();
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events|Sleep")
    void OnMentalFogEnd();
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events|Sleep")
    void OnMicrosleep();
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events|Sleep")
    void OnMicrosleepEnd();
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events|Sleep")
    void OnCollapse();
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events|Sleep")
    void OnWakeUp();
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events|Sleep")
    void OnRested();

    /** Poza snu DOBROWOLNEGO start (sen napędzany przez BT via StartSleep). Omdlenie/ragdoll używa OnCollapse. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events|Sleep")
    void OnSleepStart();

    // ==== FLEE / ZAGROŻENIE (TASK 3, plaster #4) ====
    // Generyczne zagrożenie, OD którego NPC ucieka. Ustawiane przez damage-hook (OnTakeAnyDamage → causer)
    // ORAZ (L0-04) przez percepcję predatora/wroga — to samo SetThreat. Threat jest WŁASNYM wyzwalaczem
    // paniki: EvaluateCurrentNeed czyta IsThreatActive() → Level_0_FightOrFlight, niezależnie od HP/Neurotyczności.

    /** Aktor-zagrożenie (słaby ptr — auto-null gdy zniszczony). Żywe źródło kierunku ucieczki. */
    UPROPERTY(BlueprintReadOnly, Category = "AI|Maslow|Threat")
    TWeakObjectPtr<AActor> ThreatActor;

    /** Ostatnia znana pozycja zagrożenia (cache — gdy ThreatActor zniknie, wciąż uciekamy od miejsca). */
    UPROPERTY(BlueprintReadOnly, Category = "AI|Maslow|Threat")
    FVector ThreatLocation = FVector::ZeroVector;

    /** Typ bieżącego zagrożenia (pod przyszłe różnicowanie reakcji). */
    UPROPERTY(BlueprintReadOnly, Category = "AI|Maslow|Threat")
    EThreatType ThreatType = EThreatType::None;

    /** Ile sekund NPC ucieka po OSTATNIM bodźcu zagrożenia, zanim się uspokoi. [tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Maslow|Threat")
    float ThreatMemoryDuration = 8.0f;

    /** Czas świata, do którego zagrożenie jest „aktywne" (przedłużane każdym SetThreat). */
    UPROPERTY(BlueprintReadOnly, Category = "AI|Maslow|Threat")
    float ThreatExpiryTime = 0.0f;

    /** Zgłoś zagrożenie (damage-hook / percepcja L0-04). Zapisuje aktora+pozycję+typ, przedłuża okno ucieczki. */
    UFUNCTION(BlueprintCallable, Category = "AI|Maslow|Threat")
    void SetThreat(AActor* InThreatActor, EThreatType Type);

    /** Wyczyść zagrożenie (np. dotarcie do Safe Zone / threat zniknął). */
    UFUNCTION(BlueprintCallable, Category = "AI|Maslow|Threat")
    void ClearThreat();

    /** Czy jest aktywne zagrożenie (w oknie ThreatMemoryDuration). Czytane przez EvaluateCurrentNeed. */
    UFUNCTION(BlueprintPure, Category = "AI|Maslow|Threat")
    bool IsThreatActive() const;

    /** Pozycja, OD której uciekać (żywy ThreatActor jeśli ważny, inaczej cache). Czyta BTTask_Flee. */
    UFUNCTION(BlueprintPure, Category = "AI|Maslow|Threat")
    FVector GetThreatLocation() const;

    /** Wizual paniki/ucieczki (BP rysuje krzyk/sprint). Odpalane przez BTTask_Flee na wejściu. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events|Threat")
    void OnPanicFlee();

    /** Bound do GetOwner()->OnTakeAnyDamage w BeginPlay: KAŻDE ApplyDamage → SetThreat(causer, Damage). */
    UFUNCTION()
    void HandleAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
        class AController* InstigatedBy, AActor* DamageCauser);

    // ==== AMBIENT TEMP (rdzeń strefowy + sprzężenie otoczenie→ciało) ====

    /** Temperatura OTOCZENIA NPC (°C) = baza strefy (cache) + offset doby. Liczona na timerze metabolizmu. */
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Climate")
    float AmbientTemp = 20.0f;

    /** Offset doby (°C) — HOOK, 0 do czasu warstwy doby (następny etap AmbientTemp-Doba). */
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Climate")
    float DayNightTempOffset = 0.0f;

    /** Fallback temp otoczenia gdy NPC poza wszystkimi strefami. [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float DefaultAmbientTemp = 20.0f;

    /** Próg temp OTOCZENIA, poniżej którego ciało pali tłuszcz ×2 by się ogrzać (NIE temp ciała — cold-burn split). [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float ColdAmbientThreshold = 11.0f;

    /** Tempo stygnięcia ciała ku otoczeniu (część delty/tick). [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float BodyCoolingRate = 0.05f;

    /** Tempo ogrzewania ciała ku otoczeniu/ognisku (zwykle szybsze — aktywna termoregulacja). [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float BodyWarmingRate = 0.08f;

    /** Izolacja: 1.0 = brak (pełne stygnięcie), <1.0 = ubranie/ogień spowalnia. HOOK ekwipunek/Eureka. */
    UPROPERTY(BlueprintReadWrite, Category = "Biology|Climate")
    float InsulationFactor = 1.0f;

    /** Dolny klamp temp ciała (°C) — poniżej = hipotermia. [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float MinBodyTemp = 25.0f;

    /** Górny klamp temp ciała (°C) — powyżej = hipertermia. [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float MaxBodyTemp = 42.0f;

    /** Homeostatyczny setpoint temp ciała (°C) — termoregulacja dąży tu na jawie / w komforcie. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float BodySetpointTemp = 36.6f;

    /** Próg temp OTOCZENIA, poniżej którego termoregulacja PRZEGRYWA nawet na jawie (powolna hipotermia).
     *  Domyślnie 0°C — żadna strefa bazowo nie wymusza hipotermii na jawie; ekstremum przyjdzie z warstwą doby. [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float ExtremeColdThreshold = 0.0f;

    /** Dystans (UU), po którym NPC od-świeża cache strefy (perf: stojący NPC nie odpytuje GetZoneAtLocation). [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float ZoneRecheckDistance = 500.0f;

    /** Amplituda dobowego offsetu temp (°C): noc = −Amp, południe = +Amp. SunFactor (z SunIntensity zegara)
     *  niesie też SEZON za darmo (MaxSunIntensity=stała → niższe SunIntensity zimą w południe). [TBD→tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Climate")
    float DayNightAmplitude = 8.0f;

    /** Baza temp z CACHE'owanej strefy (NIE GetZoneAtLocation co tick). Fallback DefaultAmbientTemp poza strefą. */
    UFUNCTION(BlueprintPure, Category = "AI|Maslow|Climate")
    float GetZoneBaseTemp() const;

    /** L0 Track A / Slice 1 (decision #3): FELT ambient temperature for EQS/BT navigation + reporting.
     *  Read-side wrapper over the SAME AmbientTemp (single source of truth) — a covering Shelter dampens the
     *  COLD DEFICIT only (ADDITIVE form; Dampen 0 = no shelter = raw ambient, 1 = full shelter = ComfortTempMin):
     *    Felt = (AmbientTemp >= ComfortTempMin) ? AmbientTemp
     *         : AmbientTemp + (ComfortTempMin - AmbientTemp) * CurrentShelterColdDampen.
     *  Warm/neutral ambient returns raw (shelter never cools a warm NPC). GUARDRAIL: NOT wired into the
     *  ambient→body coupling, GetTempQualityMultiplier(), or the hypothermia/death path — those keep reading
     *  AmbientTemp. Body-coupling feed is deferred to a future thermo slice. */
    UFUNCTION(BlueprintPure, Category = "AI|Maslow|Climate")
    float GetFeltTemperature() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Metabolism")
    float BaseBurnRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Metabolism")
    float HydrationBurnRatePerTick;

    // Aktualny mnożnik spalania wyciągnięty z DT_ActionCost (np. x3 podczas ucieczki)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|DynamicMetabolism")
    float CurrentActionKcalMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|DynamicMetabolism")
    float CurrentActionHydrationMultiplier;

    // Mnożnik temperatury (NPC marznie -> spala więcej energii, aby się ogrzać)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|DynamicMetabolism")
    float TemperatureKcalMultiplier;

    // Tabela kosztów akcji (DT_ActionCosts, Row Struct = FActionCostRow). Ustaw w edytorze.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|DynamicMetabolism")
    TObjectPtr<UDataTable> ActionCostTable = nullptr;

    // Dodatkowy modyfikator temperatury z aktualnej akcji (z DT_ActionCosts).
    UPROPERTY(BlueprintReadOnly, Category = "Biology|DynamicMetabolism")
    float CurrentActionTempModifier = 0.0f;

    // Koszt staminy/s z aktualnej akcji (z DT_ActionCosts). Do wykorzystania przez system staminy.
    UPROPERTY(BlueprintReadOnly, Category = "Biology|DynamicMetabolism")
    float CurrentActionStaminaCost = 0.0f;

    // ==== STATUSY I RYZYKO (EUREKA) ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|StatusEffects")
    bool bIsPoisoned;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|StatusEffects")
    bool bIsHallucinating;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|StatusEffects")
    bool bHasHighMorale;

    // Aktualna faza do odczytu przez AI i UI
    UPROPERTY(BlueprintReadOnly, Category = "Biology|State")
    EHungerPhase CurrentHungerPhase;

    // ==== ZDROWIE I KONSEKWENCJE ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Health")
    float CurrentHP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Health")
    float CurrentMaxHP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Health")
    float AbsoluteMaxHP;

    // ==== INNE PARAMETRY ====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Vitals")
    float CurrentTemp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Vitals")
    float CurrentHydration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Vitals")
    float CurrentStamina;

    // ==== STAMINA = GATE PRĘDKOŚCI (TASK 2, D1 follow-up) ====
    // Stamina NIE routuje już do potrzeb (wycięta z drabiny — sen jest na HoursAwake). Nowa rola:
    // wyczerpanie fizyczne akcją → wolniejszy ruch / brak sprintu. Drenowana CurrentActionStaminaCost
    // na kadencji metabolizmu, regenerowana w spoczynku. Ruch (BP) czyta GetStaminaSpeedMultiplier()
    // (wzorzec jak GetWorkEfficiencyMultiplier — C++ liczy, BP konsumuje). Próg krytyczny niżej (legacy).

    /** Regeneracja Staminy na 1 tick metabolizmu, gdy NPC NIE wykonuje męczącej akcji (CurrentActionStaminaCost<=0). [tune] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Vitals")
    float StaminaRegenPerTick = 8.0f;

    /** Dolny mnożnik prędkości przy Staminie=0 (wyczerpany NPC nadal idzie, ale wolno). 1.0 przy pełnej. [approved 0.5] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Vitals")
    float StaminaMinSpeedMultiplier = 0.5f;

    /** Mnożnik prędkości ruchu z bieżącej Staminy: Lerp(MinSpeedMult..1.0 po Stamina/100). */
    UFUNCTION(BlueprintPure, Category = "AI|Maslow|Vitals")
    float GetStaminaSpeedMultiplier() const;

    /** Bazowa MaxWalkSpeed (cache z CharacterMovement w BeginPlay). Ruch skalowany od niej przez Staminę (L1-08). */
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Vitals")
    float BaseWalkSpeed = 0.0f;

private:
    /** Cache komponentu ruchu właściciela (skalowanie MaxWalkSpeed bez TActorIterator). */
    UPROPERTY()
    TObjectPtr<class UCharacterMovementComponent> CachedMovement = nullptr;

    /** Cache InventoryComponent właściciela (L1-09a: Σ izolacji ubrania co kadencję, bez FindComponentByClass per tick). */
    UPROPERTY()
    TObjectPtr<class UInventoryComponent> CachedInventory = nullptr;

    /** MaxWalkSpeed = BaseWalkSpeed×Stamina; 0 gdy śpi/mikrosen/omdlały (L1-08). Woła kadencja + przejścia snu. */
    void ApplyMovementSpeedForState();

public:

    // Funkcje
    UFUNCTION(BlueprintCallable, Category = "AI|Maslow")
    void ConsumeFood(float Proteins, float Fats, float Sugars);

    // Zmiana akcji NPC (podpinamy pod Behavior Tree)
    UFUNCTION(BlueprintCallable, Category = "AI|Maslow|DynamicMetabolism")
    void SetCurrentActionMultiplier(float NewKcalMultiplier, float NewHydrationMultiplier);

    // Ustawia mnożniki metabolizmu na podstawie wiersza z DT_ActionCosts (data-driven).
    // ActionRowName = nazwa wiersza (np. "Action.Idle"). Zwraca false, gdy brak tabeli/wiersza.
    UFUNCTION(BlueprintCallable, Category = "AI|Maslow|DynamicMetabolism")
    bool SetActionByRow(FName ActionRowName);

    // Zjedzenie przedmiotu ze zdefiniowanym ryzykiem (Połączenie z DT_FoodStats)
    UFUNCTION(BlueprintCallable, Category = "AI|Maslow|Food")
    void ConsumeFoodItem(const FFoodItemRow& FoodData);

    // Wydarzenie w Blueprint (odpala np. zielone bąbelki trucizny albo serduszka morale)
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events")
    void OnFoodConsumed(bool bGotPoisoned, bool bGotHallucinations);

    // Dynamiczny kalkulator całkowitego zapotrzebowania Kcal w danym momencie
    UFUNCTION(BlueprintPure, Category = "AI|Maslow|DynamicMetabolism")
    float CalculateTotalKcalBurnRate() const;

    // Funkcje ułatwiające pracę z UI (BlueprintPure znaczy, że węzeł nie potrzebuje pinu "Exec" białej linii)
    UFUNCTION(BlueprintPure, Category = "Biology|UI")
    float GetHPPercent() const;

    UFUNCTION(BlueprintPure, Category = "Biology|UI")
    float GetHydrationPercent() const;

    UFUNCTION(BlueprintPure, Category = "Biology|UI")
    float GetGlucosePercent() const;
    
    UFUNCTION(BlueprintPure, Category = "Biology|UI")
    float GetFatPercent() const;

    // Alias for HUD naming consistency (GetFatPercent already exists).
    UFUNCTION(BlueprintPure, Category = "Biology|UI")
    float GetBodyFatPercent() const { return GetFatPercent(); }

    UFUNCTION(BlueprintPure, Category = "Biology|UI")
    float GetGlycogenPercent() const;

    UFUNCTION(BlueprintPure, Category = "Biology|UI")
    float GetHoursAwakePercent() const;

    UFUNCTION(BlueprintPure, Category = "Biology|UI")
    float GetHoursAwake() const { return HoursAwake; }

    // Magiczny nód zdarzenia (Pojawi się w BP na czerwono!)
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Events")
    void OnStarvedToDeath();

    // Progi alarmowe (Kiedy NPC zaczyna uważać, że ma problem)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Thresholds")
    float CriticalHPThreshold;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Thresholds")
    float CriticalTempThreshold;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Thresholds")
    float CriticalHydrationThreshold;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Thresholds")
    float CriticalStaminaThreshold;

    // DEPRECATED 2026-06-21 → zastąpione przez StableKcalThr (Hunger warstwa 1). Rung nutrition czyta teraz
    // EffectiveKcalThreshold; ten próg NIE jest już czytany w C++ (grep: 0 odczytów funkcjonalnych). Zostaje
    // dla ew. referencji BP (BlueprintReadWrite) — do kasacji w sprzątaniu mostu plaster #5.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biology|Thresholds")
    float CriticalKcalThreshold;

    // ==== Hunger warstwa 1 (OCEAN urgency): Neuroticism shifts the FELT-hunger kcal threshold. ====
    // Computed on the metabolism cadence into EffectiveKcalThreshold (BESIDE the judge, like bIsInPanic);
    // EvaluateCurrentNeed only READS EffectiveKcalThreshold — no personality math inside the judge.
    // INVARIANT: NervousKcalThr >= StableKcalThr (nervous eats EARLIER = at higher Glucose; never tolerates
    // a DEEPER deficit than the calm baseline). Glucose units (proxy true-state; ghrelin oscillator later). [tune]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Hunger")
    float StableKcalThr = 500.0f;   // calm baseline (== CriticalKcalThreshold)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Hunger")
    float NervousKcalThr = 650.0f;  // most-neurotic: eats earlier, at higher Glucose

    // Felt-hunger threshold: set by UpdateEffectiveKcalThreshold on the metabolism cadence, READ by
    // EvaluateCurrentNeed (mirror of bIsInPanic — beside the judge, no personality math inside it).
    // Init to StableKcalThr in BeginPlay so the judge has a valid baseline before the first cadence.
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Hunger")
    float EffectiveKcalThreshold = 500.0f;

    // ==== APPETITE / GRUBAS slice 1 (rozpychanie żołądka + odkładanie tłuszczu; runaway, bez leptyny) ====
    // --- Stan sesji jedzenia (transient) ---
    // Objętość pokarmu w żołądku TERAZ (drenuje przez trawienie na ticku). NAPĘDZANA Volume, nie kcal.
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Appetite")
    float StomachFill = 0.0f;

    // Czy NPC aktualnie je (sesja otwarta).
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Appetite")
    bool bIsEating = false;

    // Pojemność żołądka (stan PERSISTENT, dryfuje). Stretchowana posiłkami (event), kurczona głodówką (tick).
    // Init = BaseGastricCapacity w BeginPlay.
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Appetite")
    float GastricCapacity = 0.0f;

    // --- Tunables (data-driven, [TBD→tune]) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float BaseGastricCapacity = 100.0f;   // punkt zero (rozmiar "normalnego" posiłku)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float MaxGastricCapacity = 250.0f;    // ~2.5× Base — sufit skrajnego przejadacza

    // Satiety-stop POWYŻEJ pojemności (rozpychanie): bez tego MealSize nigdy nie przekracza GastricCapacity
    // i stretch-EMA jest matematycznie zamrożony. Patrz FLAGA w raporcie. [tune]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"), Category = "Biology|Appetite")
    float SatietyOverfillFactor = 1.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Appetite")
    float StretchRate = 0.30f;            // EMA na zamknięciu posiłku (szybki, event-driven)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Appetite")
    float ShrinkRate = 0.02f;             // EMA na ticku głodówki (wolny, << StretchRate) — asymetria

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float DigestionRatePerTick = 5.0f;    // ile StomachFill drenuje na tick (trawienie opróżnia żołądek)

    // StretchBonus: rozpchany żołądek → wyższy trigger → je wcześniej (dodatnie sprzężenie, część E).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float StretchBonusScale = 0.5f;

    // --- Routing makr → BodyFat (różne efektywności depozytu; Atwater-grounded) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Appetite")
    float CarbToFatEfficiency = 0.75f;    // nadwyżka cukru ponad cap Glucose → fat (DNL droga, SŁABO)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Appetite")
    float FatToFatEfficiency = 0.95f;     // dietary fat → adipocyt ~1:1 (makro grubasa, MOCNO)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Appetite")
    float ProteinToFatEfficiency = 0.5f;  // surplus białka (po odbudowie MaxHP) → fat (NAJSŁABIEJ)

    // Most fat→izolacja (część D): BodyFat 0..Max → InsulationFactor 1.0..MinInsulationFromFat.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Appetite")
    float MinInsulationFromFat = 0.6f;    // maks otyłość = mocna izolacja (wolniej stygnie)

    // L1-09a clothing: najcieplejszy możliwy InsulationFactor (clamp od dołu) gdy ubranie schodzi poniżej bazy-tłuszczu.
    // 0.1 (nie 0.2): inaczej gruby+średnie i gruby+ciężkie obie dobijają do floora = identyczne (kurtka bezsensowna
    // przy skórach). 0.1 → ciężkie wciąż bije średnie u grubego, a ≠0 (nie cold-immune). [tuning dyrektora]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Temperature")
    float FloorInsulation = 0.1f;

    // Tłuszcz na spawnie — ROZDZIELONY od MaxBodyFat (=5000). Default 1500 (~30% maxa): start ≠ max,
    // autofagia osiągalna, izolacja spawnu ≈ Lerp(1.0, 0.6, 0.3) = 0.88. EditDefaultsOnly → przyszła
    // wariacja per-archetyp (chudy/tłusty NPC z DataAssetu). BeginPlay: BodyFat = StartingBodyFat.
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float StartingBodyFat = 1500.0f;

    // --- APPETITE slice 2: LEPTYNA (hamulec — ujemne sprzężenie, lipostat setpoint-anchored) ---
    // dev = BodyFat − LeptinSetpointFat. Powyżej setpointu leptyna TŁUMI (trigger niżej + posiłek mniejszy);
    // poniżej trigger sam podbija głód. BodyFat osiada na LeptinSetpointFat → runaway (slice 1) → plateau.
    // "Naturalna waga" per-archetyp (EditDefaultsOnly jak StartingBodyFat).
    UPROPERTY(EditDefaultsOnly, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float LeptinSetpointFat = 1750.0f;

    // Lewar 1 (trigger, DWUSTRONNY): ile kcal-progu na jednostkę dev BodyFat. [TBD→tune w B1b]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float LeptinTriggerGain = 0.1f;

    // Lewar 2 (sytość, JEDNOSTRONNY max(0,dev)): ile objętości-sytości ucinane na dev powyżej setpointu. [TBD→tune]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float LeptinSatietyGain = 0.03f;

    // Podłoga triggera — gruby NPC je RZADKO, nie NIGDY (bez tego EffKcalThr<0 → cichy runaway w autofagię = anty-cel).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float LeptinTriggerFloor = 250.0f;

    // Sufit triggera — SEMANTYKA: trigger ≥ MaxGlucose → Glucose<=trigger zawsze prawda → "permanentnie głodny"
    // (wysycenie sygnału głodu; chudość nie robi się "bardziej niż zawsze głodna"). Default = MaxGlucose.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float LeptinTriggerCeiling = 1000.0f;

    // Podłoga sytości — posiłek zawsze ≥ minimum (bez tego leptyna może zbić Setpoint ≤0 → NPC nie weźmie kęsa).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"), Category = "Biology|Appetite")
    float SatietyFloor = 20.0f;

    // ==== L3-02 OCEAN → PANIC (Neuroticism shifts panic CHANCE; rolled on metabolism cadence) ====
    // HP% (CurrentHP/CurrentMaxHP) at which the CALMEST NPC's panic chance becomes >0.
    // INVARIANT: must stay >= CriticalHPThreshold/AbsoluteMaxHP so the hard floor sits below the band. [tune]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Panic")
    float StableThr = 0.25f;

    // HP% at which the MOST-neurotic NPC's panic chance becomes >0. [tune]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Panic")
    float NervousThr = 0.55f;

    // HP% width over which panic chance climbs from 0 to MaxPanicChancePerTick. [tune]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01", ClampMax="1.0"), Category = "Biology|Panic")
    float PanicBand = 0.15f;

    // Per-metabolism-tick ceiling on panic chance (sibling of MicrosleepChancePerTick). [tune]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"), Category = "Biology|Panic")
    float MaxPanicChancePerTick = 0.25f;

    // Stochastic-panic latch: set by the metabolism-cadence roll, READ by EvaluateCurrentNeed. Transient state.
    UPROPERTY(BlueprintReadOnly, Category = "Biology|Panic")
    bool bIsInPanic = false;

    // Magia: To pojawi się w Blueprintach jako gotowy Nód do wyciągnięcia
    // bActionableOnly (F2 fix, TASK 2 follow-up): gdy true, POMIŃ poziomy bez BT-akcji (Temperature dziś)
    // i schodź do następnej ACTIONABLE potrzeby. GetActionableNeed woła z true → zmarznięty NPC nie zwraca
    // już need=0 (bezczynne marznięcie), lecz realizuje jedzenie/picie/sen. Domyślnie false = pełna drabina (HUD/przyszłość).
    UFUNCTION(BlueprintCallable, Category = "AI|Maslow")
    EMaslowPriority EvaluateCurrentNeed(bool bActionableOnly = false);

    // ---- Maslow -> BT bridge (slice #1, pragnienie): CONCRETE actionable need for the BT ----
    // Returns the FIRST need hit while walking the Maslow pyramid TOP-DOWN
    // (panic -> temperature -> hydration -> rest/sleep -> nutrition/hunger -> none).
    // This is the Maslow hierarchy as an if-ladder: a higher need short-circuits the lower ones
    // (NOT "any need under threshold"). The ladder + order live in EvaluateCurrentNeed() (EMaslowPriority
    // 0->6); we delegate to it (single source of priority truth, no drift) and only MAP the LEVEL to a
    // CONCRETE need. Return is a raw uint8 CONTRACT-LOCKED to the BP enum E_NeedState:
    //   0 = None, 1 = Hunger, 2 = Thirst, 3 = Sleep, 4 = Flee.
    // Raw uint8 (not a 2nd C++ enum) keeps E_NeedState the single source -> nothing to keep in lockstep.
    // The BT service writes this via SetValueAsEnum(CurrentNeed). EvaluateCurrentNeed() returns the LEVEL
    // and stays untouched (HUD/other consumers).
    UFUNCTION(BlueprintPure, Category = "AI|Maslow")
    uint8 GetActionableNeed();

    // ==== APPETITE / GRUBAS slice 1 — proces jedzenia (BP rządzi rytmem, C++ liczy skutek) ====
    // Otwiera sesję jedzenia. Meal = makra+Volume CAŁEGO posiłku (BP rozwiązuje z DT), BiteCount = na ile
    // kęsów rozłożyć (rytm gryzień gra AnimNotify). Food = item w świecie (weak; może zniknąć — P2P) lub null
    // (posiłek abstrakcyjny/test). C++ NIE dotyka animacji.
    UFUNCTION(BlueprintCallable, Category = "Biology|Appetite")
    void StartEating(AActor* Food, const FFoodItemRow& Meal, int32 BiteCount);

    // APPETITE slice 1b — wiring helper: resolves Food's FoodTableRowName from FoodTable → FFoodItemRow,
    // then StartEating. Keeps the struct/data resolution in C++ (brain), so the BT/BP eat task calls ONE
    // clean node (no wildcard struct/data-table pins in Blueprint). Returns false AND does NOT start when
    // Food is invalid, FoodTable is null, or the row is not found — BP gates the eat montage on this bool
    // (false → no PlaySlotAnimationAsDynamicMontage → no silent hang over an empty meal). Data-driven:
    // the row id is read from the item (Food->FoodTableRowName), never hardcoded.
    UFUNCTION(BlueprintCallable, Category = "Biology|Appetite")
    bool StartEatingItem(AItemBase* Food, UDataTable* FoodTable, int32 BiteCount);

    // UTILITY — NOT biology. Lives here only as a pragmatic home (no UBlueprintFunctionLibrary in the
    // module yet, and a dedicated file for one 3-liner is disproportionate). MOVE to a BP function library
    // if more such utils accrue. Removes null/!IsValid entries from an actor array (by ref) and returns the
    // new count. Used to prune BP_NPC_AI's perception-built Food array (perception only Adds, never removes
    // → HowMuchFood would lie + the array would leak nulls; see ROADMAP TECH-11). Typed array pin (not
    // wildcard) so it is Blueprint-authorable via tooling. Static: no Maslow state touched.
    UFUNCTION(BlueprintCallable, Category = "Utility")
    static int32 CompactNullActors(UPARAM(ref) TArray<AActor*>& Actors);

    // Jedno ugryzienie — wołane z AnimNotify (naturalny zegar gryzień, zero nowego timera). Deponuje makra
    // jednego kęsa, rośnie StomachFill, dekrementuje porcję itemu. Auto-stop przy sytości / wyczerpaniu.
    // Early-return przy !bIsEating / bIncapacitated / stale Food (EC-EAT-2/3).
    UFUNCTION(BlueprintCallable, Category = "Biology|Appetite")
    void ConsumeBite();

    // Zamyka sesję: rozmiar posiłku → stretch-EMA pojemności. Nadgryzione jedzenie zostaje w świecie.
    UFUNCTION(BlueprintCallable, Category = "Biology|Appetite")
    void StopEating(EEatStopReason Reason);

    // Próg sytości (kiedy przestać jeść) = pojemność × overfill (rozpychanie ponad pojemność → stretch).
    UFUNCTION(BlueprintPure, Category = "Biology|Appetite")
    float GetSatietySetpoint() const
    {
        // APPETITE slice 2: leptyna tnie wielkość posiłku TYLKO powyżej setpointu (jednostronnie); floor = min posiłek.
        const float Base = GastricCapacity * SatietyOverfillFactor;
        const float LeptinCut = LeptinSatietyGain * FMath::Max(0.0f, BodyFat - LeptinSetpointFat);
        return FMath::Max(SatietyFloor, Base - LeptinCut);
    }

    UFUNCTION(BlueprintPure, Category = "Biology|Appetite")
    float GetGastricCapacity() const { return GastricCapacity; }

    UFUNCTION(BlueprintPure, Category = "Biology|Appetite")
    float GetBodyFatRatio() const { return GetFatPercent(); }   // alias gate'owy; reużywa GetFatPercent

    // BP rysuje reakcję na koniec posiłku (beknięcie / animacja sytości / odłożenie nadgryzionego). C++ liczy CZY.
    UFUNCTION(BlueprintImplementableEvent, Category = "Biology|Appetite")
    void OnMealEnd(float MealSize, EEatStopReason Reason);

private:
    // L3-02: rolls Neuroticism-driven panic on the metabolism cadence (mirror of TriggerMicrosleep).
    // Sets bIsInPanic; EvaluateCurrentNeed only reads it. NO RNG in the judge. Called from ProcessMetabolism.
    void EvaluatePanicRoll();

    // Hunger warstwa 1: computes EffectiveKcalThreshold from Neuroticism on the metabolism cadence
    // (sibling of EvaluatePanicRoll). EvaluateCurrentNeed only reads the field. NO personality in the judge.
    // Slice 1 appetite: + StretchBonus(GastricCapacity) (część E; LeptinBrake = 0 do slice 2).
    void UpdateEffectiveKcalThreshold();

    // APPETITE slice 1: routing makr jednego kęsa → Glucose/Glycogen/BodyFat/MaxHP (różne efektywności).
    void DepositBiteMacros(float CarbG, float FatG, float ProteinG);

    // APPETITE slice 1: drenaż StomachFill (trawienie) + shrink-driver pojemności przy głodówce. Na istniejącym ticku.
    void UpdateAppetiteTick();

    // Lazy-cached sibling identity (perf: no FindComponentByClass per tick across 500 NPCs).
    TWeakObjectPtr<class UNPCIdentityComponent> CachedIdentity;

    // APPETITE slice 1 — stan sesji (nie-UPROPERTY: runtime, jak CachedIdentity).
    TWeakObjectPtr<AActor> EatTargetFood;   // item jedzenia tej sesji (weak — może zniknąć: P2P/śmierć)
    FFoodItemRow CurrentMeal;               // makra+Volume CAŁEGO posiłku (kęs = część 1/TotalBites)
    int32 TotalBites = 0;                   // na ile kęsów rozłożony posiłek
    int32 RemainingBites = 0;               // ile kęsów zostało
    float CurrentMealSize = 0.0f;           // suma Volume zjedzonych kęsów (→ stretch-EMA przy StopEating)
    bool bHasFoodActor = false;             // czy sesja ma realny item (do guardu EC-EAT-2 vs posiłek-test)
    bool bWasFasting = false;               // rising-edge log shrink-tick
};