#include "MaslowBiologicalComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"            // TActorIterator (Option A: find the world clock)
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"     // FProperty / FDoubleProperty / FFloatProperty (reflection read)
#include "GameFramework/Pawn.h"     // ETAP 2: omdlenie — dostęp do kontrolera pawna
#include "AIController.h"            // ETAP 2: zatrzymanie/wznowienie Behavior Tree
#include "BrainComponent.h"         // ETAP 2: StopLogic / RestartLogic
#include "Map/CaldrethZone.h"       // AmbientTemp: GetZoneAtLocation + FZoneDef.BaseTemp
#include "NPC/NPCIdentityComponent.h" // L3-02: read Neuroticism for the panic roll
#include "ItemBase.h"               // APPETITE slice 1: nadgryzienie itemu (Cast<AItemBase> w ConsumeBite)

DEFINE_LOG_CATEGORY(LogMaslow);

UMaslowBiologicalComponent::UMaslowBiologicalComponent()
{
    // Wyłączamy Tickowanie! Dbamy o optymalizację pod 500 NPC
    PrimaryComponentTick.bCanEverTick = false;

    // Ustawienie parametrów zgodnie z GDD
    MaxGlucose = 1000.0f;
    Glucose = 1000.0f;

    MaxGlycogen = 1000.0f;
    GlycogenReserves = 1000.0f;

    MaxBodyFat = 5000.0f;
    // BodyFat START rozdzielony od Maxa → ustawiany w BeginPlay z UPROPERTY StartingBodyFat (default 1500).
    // (Konstruktorowe 5000 dawało spawn = max fat → InsulationFactor=0.6 u wszystkich + autofagia poza zasięgiem.)

    AbsoluteMaxHP = 100.0f;
    CurrentMaxHP = 100.0f;
    CurrentHP = 100.0f; // CurrentHP podąża za CurrentMaxHP w limicie

    BaseBurnRate = 10.0f; // Traci bazowo 10 energii co wywołanie timera
    CurrentHungerPhase = EHungerPhase::Phase_0_Glucose;

    // Inicjalizacja mnożników
    CurrentActionKcalMultiplier = 1.0f;
    CurrentActionHydrationMultiplier = 1.0f;
    TemperatureKcalMultiplier = 1.0f;
    CurrentActionTempModifier = 0.0f;
    CurrentActionStaminaCost = 0.0f;

    // Inicjalizacja statusów
    bIsPoisoned = false;
    bIsHallucinating = false;
    bHasHighMorale = false;

    // Domyślne wartości startowe dla reszty
    CurrentTemp = 36.6f;
    CurrentHydration = 100.0f;
    CurrentStamina = 100.0f;
    HydrationBurnRatePerTick = 2.0f; 

    // Domyślne progi alarmowe
    CriticalHPThreshold = 25.0f;
    CriticalTempThreshold = 34.0f; // Hipotermia
    CriticalHydrationThreshold = 20.0f;
    CriticalStaminaThreshold = 15.0f;
    CriticalKcalThreshold = 500.0f;
}

void UMaslowBiologicalComponent::BeginPlay()
{
    Super::BeginPlay();

    // Hunger warstwa 1: seed the felt-hunger threshold to the (designer-edited) calm baseline so the judge
    // has a valid value before the first metabolism cadence runs UpdateEffectiveKcalThreshold().
    EffectiveKcalThreshold = StableKcalThr;

    // APPETITE slice 1: pojemność żołądka startuje na podłodze (rozpychana posiłkami, kurczona głodówką).
    GastricCapacity = BaseGastricCapacity;

    // APPETITE slice 1: tłuszcz na spawnie z tunable'a (rozdzielony od MaxBodyFat). ~30% maxa = start ≠ max,
    // autofagia osiągalna, InsulationFactor spawnu ≈ Lerp(1.0, 0.6, 0.3) = 0.88 (nie 0.6 jak przy BodyFat=max).
    BodyFat = FMath::Clamp(StartingBodyFat, 0.0f, MaxBodyFat);

    // Timer uruchamia się po starcie i wywołuje ProcessMetabolism co MetabolismInterval sekund.
    // Ta sama wartość interwału jest używana do narastania HoursAwake i do wyliczenia AwakeRatePerTick.
    GetWorld()->GetTimerManager().SetTimer(
        MetabolismTimerHandle,
        this,
        &UMaslowBiologicalComponent::ProcessMetabolism,
        MetabolismInterval,
        true
    );
}

void UMaslowBiologicalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // EC-1 fail-safe (zasada projektu): martwy/zniszczony NPC NIE zostawia wiszącego timera ani flag
    // blokady. Bezwarunkowe sprzątanie — krytyczne przy 500 NPC ginących w trakcie mikrosnu/omdlenia.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MicrosleepTimer);
        World->GetTimerManager().ClearTimer(MetabolismTimerHandle);
    }
    bMicrosleeping = false;
    bIncapacitated = false;

    Super::EndPlay(EndPlayReason);
}

void UMaslowBiologicalComponent::ProcessMetabolism()
{
    // FAZA 4: Wyrok ostateczny.
    if (CurrentMaxHP <= 0.0f)
    {
        // Żeby zapobiec wywoływaniu śmierci w kółko co 10 sekund,
        // sprawdzamy, czy już wcześniej nie był martwy
        if (CurrentHungerPhase != EHungerPhase::Phase_4_Death)
        {
            CurrentHungerPhase = EHungerPhase::Phase_4_Death;
            CurrentHP = 0.0f;
            OnStarvedToDeath(); // <--- ODPALAMY EVENT DO BLUEPRINTA!
            GetWorld()->GetTimerManager().ClearTimer(MetabolismTimerHandle);
        }
        return;
    }

    // --- SILNIK SNU (L1-06/07): narastanie zmęczenia na tym ticku ---
    // Opcja A: dostrojenie do zegara świata robimy leniwie na 1. ticku (po wszystkich BeginPlay).
    if (!bAwakeRateResolved)
    {
        ResolveAwakeRateFromWorldClock();
    }
    UpdateFatigue();

    // STAMINA (TASK 2): gate PRĘDKOŚCI, nie potrzeba. Drenowana kosztem męczącej akcji (CurrentActionStaminaCost
    // z DT akcji, dotąd liczony lecz NIEaplikowany), regenerowana w spoczynku/śnie. Czytana przez BP-ruch via
    // GetStaminaSpeedMultiplier(). Wycięta z drabiny potrzeb (EvaluateCurrentNeed) — patrz D1.
    if (CurrentActionStaminaCost > 0.0f)
    {
        CurrentStamina = FMath::Clamp(CurrentStamina - CurrentActionStaminaCost, 0.0f, 100.0f);
    }
    else
    {
        CurrentStamina = FMath::Clamp(CurrentStamina + StaminaRegenPerTick, 0.0f, 100.0f);
    }

    // Spalanie wody (zachowane z poprzedniej iteracji + Mnożnik Aktywności)
    float TotalHydrationBurn = HydrationBurnRatePerTick * CurrentActionHydrationMultiplier;
    
    // Jeśli NPC jest zatruty (np. po surowym mięsie), woda spada 3x szybciej!
    if (bIsPoisoned)
    {
        TotalHydrationBurn *= 3.0f;
    }

    CurrentHydration = FMath::Clamp(CurrentHydration - TotalHydrationBurn, 0.0f, 100.0f);

    // KONSEKWENCJE ODWODNIENIA: Odbieramy HP (CurrentHP), nie mylić z max!
    if (CurrentHydration <= 0.0f)
    {
        // Przy braku wody błyskawicznie niszczymy też białko (MaxHP)
        CurrentMaxHP = FMath::Clamp(CurrentMaxHP - 10.0f, 0.0f, AbsoluteMaxHP);
        CurrentHP = FMath::Clamp(CurrentHP - 10.0f, 0.0f, CurrentMaxHP);

        if (CurrentHP <= 0.0f)
        {
            if (CurrentHungerPhase != EHungerPhase::Phase_4_Death)
            {
                CurrentHungerPhase = EHungerPhase::Phase_4_Death;
                GetWorld()->GetTimerManager().ClearTimer(MetabolismTimerHandle);
                OnStarvedToDeath();
            }
            return;
        }
    }

    // ==== AMBIENT TEMP — strefa niesie temperaturę otoczenia (rdzeń strefowy) ====
    // Cache strefy (perf #1): GetZoneAtLocation (TActorIterator) TYLKO po ruchu > próg, nie co tick.
    RefreshZoneCache();
    UpdateDayNightTempOffset();   // warstwa doby: DayNightTempOffset z nasłonecznienia zegara świata
    // AmbientTemp = baza strefy (cache) + offset doby (nasłonecznienie zegara — SunFactor).
    AmbientTemp = GetZoneBaseTemp() + DayNightTempOffset;

    // APPETITE slice 1 (część D) — most fat→izolacja. BodyFat 0..Max → InsulationFactor 1.0..MinInsulationFromFat.
    // Liczone TU (przed reżimami), by sprzężenie stygnięcia użyło świeżej izolacji w TYM samym ticku. Jedyny pisarz.
    InsulationFactor = FMath::Lerp(1.0f, MinInsulationFromFat, GetBodyFatRatio());

    // ==== SPRZĘŻENIE OTOCZENIE → CIAŁO (3 reżimy termoregulacji — zatwierdzone) ====
    // Hipotermia STOPNIOWA (przez wiele ticków) — fail-safe, NIE instant. CurrentTemp≤CriticalTempThreshold(34)
    // → priorytet Level_1_Temperature w EvaluateCurrentNeed.
    if (bIsSleeping && AmbientTemp < ComfortTempMin)
    {
        // 1) Sen w zimnie (<15°C otoczenia): termoregulacja WYŁĄCZONA → najszybsze stygnięcie (×ColdSleepMultiplier).
        CurrentTemp += (AmbientTemp - CurrentTemp) * BodyCoolingRate * ColdSleepMultiplier * InsulationFactor;
    }
    else if (!bIsSleeping && AmbientTemp < ExtremeColdThreshold)
    {
        // 2) Jawa, EKSTREMALNE zimno (<próg, domyślnie 0°C): termoregulacja przegrywa → powolny dryf
        //    (bez ×3.5 — NPC się rusza/drży). Ekstremum przyjdzie z warstwą doby (noc na górze: 4-8=-4°C).
        CurrentTemp += (AmbientTemp - CurrentTemp) * BodyCoolingRate * InsulationFactor;
    }
    else
    {
        // 3) Jawa (każde znośne otoczenie) ALBO sen w komforcie → termoregulacja utrzymuje setpoint 36.6.
        CurrentTemp += (BodySetpointTemp - CurrentTemp) * BodyWarmingRate;
    }
    CurrentTemp = FMath::Clamp(CurrentTemp, MinBodyTemp, MaxBodyTemp);

    // Log hipotermii — raz na wejście poniżej progu krytycznego (histereza +1°C), do verify bloku 3.
    if (CurrentTemp <= CriticalTempThreshold && !bHypothermiaLogged)
    {
        bHypothermiaLogged = true;
        UE_LOG(LogMaslow, Warning,
            TEXT("[Temp] %s: HIPOTERMIA (CurrentTemp=%.2f <= %.1f, ambient=%.1f, śpi=%d) — Level_1_Temperature."),
            GetOwner() ? *GetOwner()->GetName() : TEXT("?"), CurrentTemp, CriticalTempThreshold, AmbientTemp,
            bIsSleeping ? 1 : 0);
    }
    else if (CurrentTemp > CriticalTempThreshold + 1.0f && bHypothermiaLogged)
    {
        bHypothermiaLogged = false;   // odzyskał ciepło — pozwól zalogować ponownie przy kolejnym wychłodzeniu
    }

    // Spalanie na zimnie — COLD-BURN SPLIT (decyzja usera): zależy od OTOCZENIA (zimno = organizm
    // grzeje się = pali tłuszcz ×2), NIE od temp ciała. Hipotermia (CurrentTemp≤CriticalTempThreshold=34)
    // to OSOBNY skutek — priorytet Level_1_Temperature w EvaluateCurrentNeed.
    if (AmbientTemp < ColdAmbientThreshold)
    {
        TemperatureKcalMultiplier = 2.0f + CurrentActionTempModifier;
    }
    else
    {
        TemperatureKcalMultiplier = 1.0f;
    }

    // MATRYCA SPALANIA (Kolejność Kataboliczna)
    float TotalBurnRate = CalculateTotalKcalBurnRate();

    if (Glucose > 0.0f)
    {
        CurrentHungerPhase = EHungerPhase::Phase_0_Glucose;
        Glucose = FMath::Clamp(Glucose - TotalBurnRate, 0.0f, MaxGlucose);
    }
    else if (GlycogenReserves > 0.0f)
    {
        CurrentHungerPhase = EHungerPhase::Phase_1_Glycogen;
        GlycogenReserves = FMath::Clamp(GlycogenReserves - TotalBurnRate, 0.0f, MaxGlycogen);
    }
    else if (BodyFat > 0.0f)
    {
        CurrentHungerPhase = EHungerPhase::Phase_2_FatBurn;
        // Tłuszcz jest gęstszy kalorycznie, spala się wolniej (podzielimy BurnRate przez np. 2)
        BodyFat = FMath::Clamp(BodyFat - (TotalBurnRate * 0.5f), 0.0f, MaxBodyFat);
    }
    else
    {
        CurrentHungerPhase = EHungerPhase::Phase_3_Autophagy;
        // Brak wszystkiego, organizm zjada mięśnie (białko)
        CurrentMaxHP = FMath::Clamp(CurrentMaxHP - (TotalBurnRate * 0.25f), 0.0f, AbsoluteMaxHP);

        if (CurrentHP > CurrentMaxHP)
        {
            CurrentHP = CurrentMaxHP;
        }
    }

    // L3-02: Neuroticism-driven panic roll on the freshest HP (after all this tick's HP changes).
    EvaluatePanicRoll();

    // Hunger warstwa 1: refresh the Neuroticism-modulated felt-hunger threshold (beside the judge).
    UpdateEffectiveKcalThreshold();

    // APPETITE slice 1: drenaż żołądka (trawienie) + shrink pojemności przy głodówce (po ustaleniu fazy głodu).
    UpdateAppetiteTick();
}

void UMaslowBiologicalComponent::SetCurrentActionMultiplier(float NewKcalMultiplier, float NewHydrationMultiplier)
{
    CurrentActionKcalMultiplier = NewKcalMultiplier;
    CurrentActionHydrationMultiplier = NewHydrationMultiplier;
}

bool UMaslowBiologicalComponent::SetActionByRow(FName ActionRowName)
{
    // Data-driven: pobierz koszty akcji z DT_ActionCosts (Row Struct = FActionCostRow).
    if (!IsValid(ActionCostTable) || ActionRowName.IsNone())
    {
        return false;
    }

    static const FString Context(TEXT("UMaslowBiologicalComponent::SetActionByRow"));
    const FActionCostRow* Row = ActionCostTable->FindRow<FActionCostRow>(
        ActionRowName, Context, /*bWarnIfRowMissing*/ false);
    if (!Row)
    {
        return false;
    }

    CurrentActionKcalMultiplier      = Row->KcalMultiplier;
    CurrentActionHydrationMultiplier = Row->WaterMultiplier;
    CurrentActionTempModifier        = Row->TempModifier;
    CurrentActionStaminaCost         = Row->StaminaCostPerSecond;
    return true;
}

float UMaslowBiologicalComponent::GetGlycogenPercent() const
{
    if (MaxGlycogen <= 0.0f) return 0.0f;
    return FMath::Clamp(GlycogenReserves / MaxGlycogen, 0.0f, 1.0f);
}

float UMaslowBiologicalComponent::GetHoursAwakePercent() const
{
    if (MaxHoursBeforeCollapse <= 0.0f) return 0.0f;
    return FMath::Clamp(HoursAwake / MaxHoursBeforeCollapse, 0.0f, 1.0f);
}

void UMaslowBiologicalComponent::UpdateFatigue()
{
    const EFatigueState Prev = FatigueState;

    if (bIsSleeping)
    {
        // ETAP 2 — RESET: sen redukuje zmęczenie; tempo zależne od jakości cieplnej snu
        // (komfort = pełne tempo; zimno/upał = sen płytki = wolniejsza regeneracja).
        HoursAwake = FMath::Max(0.0f, HoursAwake - SleepRecoveryPerTick * GetTempQualityMultiplier());
    }
    else
    {
        // NARASTANIE: skalowane TYM SAMYM mnożnikiem aktywności, którego używa metabolizm
        // (Idle 1.0 / Work / Combat). Praca/walka męczą szybciej (emergentny realizm).
        HoursAwake += AwakeRatePerTick * CurrentActionKcalMultiplier;
    }

    // Przelicz drabinę z (nowego) HoursAwake.
    if (HoursAwake >= CollapseThreshold)        FatigueState = EFatigueState::Collapsed;
    else if (HoursAwake >= MicrosleepThreshold) FatigueState = EFatigueState::Microsleeps;
    else if (HoursAwake >= MentalFogThreshold)  FatigueState = EFatigueState::MentalFog;
    else                                        FatigueState = EFatigueState::Awake;

    // Loguj + odpal skutki TYLKO na przejściu progu.
    if (FatigueState != Prev)
    {
        const FString OwnerName = GetOwner() ? GetOwner()->GetName() : TEXT("?");
        UE_LOG(LogMaslow, Log,
            TEXT("[Sleep] %s: zmęczenie %d -> %d przy HoursAwake=%.2f (akt x%.1f%s)."),
            *OwnerName, static_cast<int32>(Prev), static_cast<int32>(FatigueState),
            HoursAwake, CurrentActionKcalMultiplier, bIsSleeping ? TEXT(", śpi") : TEXT(""));
        HandleFatigueTransition(Prev, FatigueState);
    }

    // D2 (TASK 2): WYBUDZENIE MIMOWOLNE z omdlenia. Collapse ustawił bIsSleeping (sen wymuszony) → HoursAwake
    // malało powyżej; gdy w pełni odespane, C++ SAM budzi (StopSleep: znosi bIncapacitated, RestartLogic wznawia
    // BT, OnWakeUp). To JEDYNE wyjście z pułapki omdlenia — BT był StopLogic'owany, więc sen dobrowolny (BT) nie zadziała.
    if (bIncapacitated && HoursAwake <= KINDA_SMALL_NUMBER)
    {
        StopSleep();
    }

    // Mikrosen: losowo, tylko na jawie, ≥ próg mikrosnów, i NIE gdy omdlały (Collapsed nadpisuje).
    if (!bIsSleeping && !bMicrosleeping && !bIncapacitated && HoursAwake >= MicrosleepThreshold
        && FMath::FRand() < MicrosleepChancePerTick)
    {
        TriggerMicrosleep();
    }
}

void UMaslowBiologicalComponent::HandleFatigueTransition(EFatigueState PrevState, EFatigueState NewState)
{
    // Mgła umysłowa: start/koniec (MentalFog i wyżej).
    const bool bWasFog = (PrevState >= EFatigueState::MentalFog);
    const bool bNowFog = (NewState  >= EFatigueState::MentalFog);
    if (bNowFog && !bWasFog)      { OnMentalFogStart(); }
    else if (!bNowFog && bWasFog) { OnMentalFogEnd(); }

    // Omdlenie: wejście w Collapsed → unieruchom, zatrzymaj BT + ruch, ragdoll (BP).
    if (NewState == EFatigueState::Collapsed && PrevState != EFatigueState::Collapsed)
    {
        bIncapacitated = true;
        // D2 (TASK 2): omdlenie = SEN WYMUSZONY. Bez tego UpdateFatigue dalej NARASTAŁOby HoursAwake
        // (gałąź !bIsSleeping) → NPC nigdy się nie wybudza (StopLogic zabił BT, StopSleep nikt nie wołał).
        // Z bIsSleeping=true HoursAwake MALEJE co tick, a UpdateFatigue auto-woła StopSleep przy ~0
        // (wznawia BT, OnWakeUp). C++ posiada wybudzenie MIMOWOLNE; BT posiada tylko sen DOBROWOLNY.
        bIsSleeping = true;
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
        {
            if (AAIController* AC = Cast<AAIController>(OwnerPawn->GetController()))
            {
                if (UBrainComponent* Brain = AC->GetBrainComponent())
                {
                    Brain->StopLogic(TEXT("Collapsed"));   // zatrzymuje Behavior Tree
                }
                AC->StopMovement();
            }
        }
        UE_LOG(LogMaslow, Warning, TEXT("[Sleep] %s: OMDLENIE (HoursAwake=%.2f) — BT zatrzymany, ragdoll."),
            GetOwner() ? *GetOwner()->GetName() : TEXT("?"), HoursAwake);
        OnCollapse();   // BP rzuca ragdoll
    }
}

void UMaslowBiologicalComponent::TriggerMicrosleep()
{
    bMicrosleeping = true;
    OnMicrosleep();   // BP rysuje kiwnięcie głową / "zamarcie"
    UE_LOG(LogMaslow, Log, TEXT("[Sleep] %s: mikrosen (HoursAwake=%.2f, %.1fs)."),
        GetOwner() ? *GetOwner()->GetName() : TEXT("?"), HoursAwake, MicrosleepDuration);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            MicrosleepTimer, this, &UMaslowBiologicalComponent::EndMicrosleep, MicrosleepDuration, false);
    }
}

void UMaslowBiologicalComponent::EndMicrosleep()
{
    bMicrosleeping = false;
    OnMicrosleepEnd();
}

float UMaslowBiologicalComponent::GetWorkEfficiencyMultiplier() const
{
    // 1.0 do MentalFogThreshold (16h), liniowo do 0.7 przy CollapseThreshold (24h) — mgła umysłowa.
    const float Range = FMath::Max(CollapseThreshold - MentalFogThreshold, 1.0f);
    const float Alpha = FMath::Clamp((HoursAwake - MentalFogThreshold) / Range, 0.0f, 1.0f);
    float Eff = FMath::Lerp(1.0f, 0.7f, Alpha);
    if (bIsRested)
    {
        Eff += RestedWorkBonus;   // buff po dobrym śnie (+praca/Eureka)
    }
    return Eff;
}

float UMaslowBiologicalComponent::GetTempQualityMultiplier() const
{
    // AmbientTemp (NIE CurrentTemp ciała!): komfort cieplny OTOCZENIA → jakość snu. To była DOKŁADNIE
    // poprawka buga z ETAP 2 (liczył komfort z temp ciała 36.6 → 0.25). Teraz z AmbientTemp = poprawne.
    if (AmbientTemp >= ComfortTempMin && AmbientTemp <= ComfortTempMax)
    {
        return 1.0f;
    }
    const float Dist = (AmbientTemp < ComfortTempMin)
        ? (ComfortTempMin - AmbientTemp)
        : (AmbientTemp - ComfortTempMax);
    return FMath::Clamp(1.0f - Dist * 0.075f, 0.25f, 1.0f);   // ~10°C poza strefą → ~0.25
}

void UMaslowBiologicalComponent::RefreshZoneCache()
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }
    const FVector Loc = Owner->GetActorLocation();

    // Perf #1 (CaldrethZone.cpp:180 PERF TODO): wołaj GetZoneAtLocation (TActorIterator po WSZYSTKICH
    // aktorach) TYLKO przy pierwszym ticku, gdy cache nieważny, albo gdy NPC ruszył > ZoneRecheckDistance.
    // Stojący/pracujący/śpiący NPC = ZERO zapytań → brak spike'u O(NPC×aktorzy) co tick.
    const bool bMoved = FVector::DistSquared(Loc, LastZoneCheckPos) > (ZoneRecheckDistance * ZoneRecheckDistance);
    if (bZoneCacheInitialized && CurrentZone.IsValid() && !bMoved)
    {
        return;   // cache aktualny — żadnego iteratora w tym ticku
    }

    CurrentZone = ACaldrethZone::GetZoneAtLocation(this, Loc);
    LastZoneCheckPos = Loc;
    bZoneCacheInitialized = true;
}

float UMaslowBiologicalComponent::GetZoneBaseTemp() const
{
    // Realna sygnatura (flaga #1): GetZoneDef(FZoneDef&) zwraca bool + out-param; strefa trzyma CachedDef
    // (resolved raz w ResolveZoneDef) → tani odczyt, bez bicia w DataTable per tick.
    if (CurrentZone.IsValid())
    {
        FZoneDef Def;
        if (CurrentZone->GetZoneDef(Def))
        {
            return Def.BaseTemp;
        }
    }
    return DefaultAmbientTemp;   // poza wszystkimi strefami / nieresolved
}

namespace
{
    // Czyta property typu "real" (double LUB float w UE5) z aktora przez reflection. False gdy brak/niepasujące.
    bool ReadActorDouble(AActor* Actor, const TCHAR* PropName, double& Out)
    {
        if (!IsValid(Actor)) { return false; }
        FProperty* Prop = Actor->GetClass()->FindPropertyByName(FName(PropName));
        if (const FDoubleProperty* DP = CastField<FDoubleProperty>(Prop))
        {
            Out = DP->GetPropertyValue_InContainer(Actor);
            return true;
        }
        if (const FFloatProperty* FP = CastField<FFloatProperty>(Prop))
        {
            Out = static_cast<double>(FP->GetPropertyValue_InContainer(Actor));
            return true;
        }
        return false;
    }
}

void UMaslowBiologicalComponent::UpdateDayNightTempOffset()
{
    // Offset dobowy z NASŁONECZNIENIA zegara świata (SunFactor). MaxSunIntensity to stała (cap),
    // a SunIntensity idzie za realną elewacją słońca → SunFactor niesie też SEZON za darmo
    // (zima w południe = niższe SunIntensity = chłodniej). Brak zegara → offset 0 (mapa bez BP_DayNightCycle).
    AActor* Clock = WorldClock.Get();
    double SunIntensity = 0.0, MaxSunIntensity = 0.0;
    if (!IsValid(Clock)
        || !ReadActorDouble(Clock, TEXT("SunIntensity"), SunIntensity)
        || !ReadActorDouble(Clock, TEXT("MaxSunIntensity"), MaxSunIntensity)
        || MaxSunIntensity <= KINDA_SMALL_NUMBER)
    {
        DayNightTempOffset = 0.0f;
        return;
    }
    // SunFactor: 0 noc (SunIntensity→0), ~1 południe. Klamp na wypadek resztki/przekroczenia.
    const float SunFactor = FMath::Clamp(static_cast<float>(SunIntensity / MaxSunIntensity), 0.0f, 1.0f);
    DayNightTempOffset = DayNightAmplitude * (SunFactor - 0.5f) * 2.0f;   // noc −Amp, południe +Amp
}

void UMaslowBiologicalComponent::StartSleep()
{
    if (bIsSleeping)
    {
        return;
    }
    bIsSleeping = true;
    bIsRested = false;   // buff zużyty w chwili zaśnięcia; odzyskany dopiero po wyspaniu
    OnSleepStart();      // D6 (TASK 2): poza snu dobrowolnego (BP). Omdlenie ma osobny wizual (OnCollapse/ragdoll).
    UE_LOG(LogMaslow, Log, TEXT("[Sleep] %s: zasypia (HoursAwake=%.2f, temp=%.1f, jakość=%.2f)."),
        GetOwner() ? *GetOwner()->GetName() : TEXT("?"), HoursAwake, CurrentTemp, GetTempQualityMultiplier());
}

void UMaslowBiologicalComponent::StopSleep()
{
    if (!bIsSleeping)
    {
        return;
    }
    bIsSleeping = false;

    // Wyspany do zera w komforcie cieplnym → buff Rested.
    const bool bFullyRested = (HoursAwake <= KINDA_SMALL_NUMBER) && (GetTempQualityMultiplier() >= 1.0f);
    bIsRested = bFullyRested;

    // Przebudzenie znosi omdlenie i wznawia Behavior Tree (jeśli był zatrzymany przez Collapsed).
    if (bIncapacitated)
    {
        bIncapacitated = false;
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
        {
            if (AAIController* AC = Cast<AAIController>(OwnerPawn->GetController()))
            {
                if (UBrainComponent* Brain = AC->GetBrainComponent())
                {
                    Brain->RestartLogic();   // wznawia Behavior Tree
                }
            }
        }
    }

    OnWakeUp();
    if (bFullyRested)
    {
        OnRested();
    }
    UE_LOG(LogMaslow, Log, TEXT("[Sleep] %s: budzi się (HoursAwake=%.2f, Rested=%d)."),
        GetOwner() ? *GetOwner()->GetName() : TEXT("?"), HoursAwake, bIsRested ? 1 : 0);
}

float UMaslowBiologicalComponent::GetStaminaSpeedMultiplier() const
{
    // TASK 2: Stamina → prędkość ruchu. Lerp(MinSpeedMult .. 1.0) po znormalizowanej Staminie (0..100).
    // Wyczerpany NPC idzie wolno (MinSpeedMult), nie staje. BP-ruch mnoży MaxWalkSpeed przez to.
    const float Norm = FMath::Clamp(CurrentStamina / 100.0f, 0.0f, 1.0f);
    return FMath::Lerp(StaminaMinSpeedMultiplier, 1.0f, Norm);
}

void UMaslowBiologicalComponent::ResolveAwakeRateFromWorldClock()
{
    // Jednorazowa próba (nawet przy niepowodzeniu — wtedy zostaje fallback z UPROPERTY).
    bAwakeRateResolved = true;

    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Clock = *It;
        if (!IsValid(Clock) || !Clock->GetClass()->GetName().Contains(TEXT("DayNightCycle")))
        {
            continue;
        }
        WorldClock = Clock;   // cache zegara do offsetu dobowego temp (reużyte co tick przez UpdateDayNightTempOffset)

        // Zmienna BP typu "real" kompiluje się do double w UE5; akceptuj double LUB float.
        FProperty* Prop = Clock->GetClass()->FindPropertyByName(TEXT("TimeSpeed"));
        double TimeSpeed = 0.0;
        bool bGot = false;
        if (const FDoubleProperty* DP = CastField<FDoubleProperty>(Prop))
        {
            TimeSpeed = DP->GetPropertyValue_InContainer(Clock);
            bGot = true;
        }
        else if (const FFloatProperty* FP = CastField<FFloatProperty>(Prop))
        {
            TimeSpeed = static_cast<double>(FP->GetPropertyValue_InContainer(Clock));
            bGot = true;
        }

        if (bGot && TimeSpeed > KINDA_SMALL_NUMBER)
        {
            // "Jeden zegar": przy Idle HoursAwake narasta dokładnie w tempie zegara dobowego.
            AwakeRatePerTick = static_cast<float>(TimeSpeed * MetabolismInterval);
            UE_LOG(LogMaslow, Log,
                TEXT("[Sleep] AwakeRatePerTick dostrojony do zegara '%s': TimeSpeed=%.4f x interwał=%.1fs => %.4f game-h/tick."),
                *Clock->GetName(), TimeSpeed, MetabolismInterval, AwakeRatePerTick);
            return;
        }
    }

    UE_LOG(LogMaslow, Warning,
        TEXT("[Sleep] Brak zegara świata (BP_DayNightCycle.TimeSpeed) — fallback AwakeRatePerTick=%.4f."),
        AwakeRatePerTick);
}

float UMaslowBiologicalComponent::CalculateTotalKcalBurnRate() const
{
    // Baza * Zmęczenie z wykonywanej pracy * Walka z temperaturą
    return BaseBurnRate * CurrentActionKcalMultiplier * TemperatureKcalMultiplier;
}

void UMaslowBiologicalComponent::ConsumeFood(float Proteins, float Fats, float Sugars)
{
    // Cukry trafiają do szybkiej energii (Glucose) i ładują Glikogen
    Glucose = FMath::Clamp(Glucose + Sugars, 0.0f, MaxGlucose);
    GlycogenReserves = FMath::Clamp(GlycogenReserves + (Sugars * 0.5f), 0.0f, MaxGlycogen);

    // Tłuszcze zwiększają warstwę tłuszczową (zapasowy bank)
    BodyFat = FMath::Clamp(BodyFat + Fats, 0.0f, MaxBodyFat);

    // Białko odbudowuje wyniszczenie mięśni i ciał (MaxHP)
    if (Proteins > 0.0f)
    {
        CurrentMaxHP = FMath::Clamp(CurrentMaxHP + (Proteins * 0.5f), 0.0f, AbsoluteMaxHP);
    }
}

void UMaslowBiologicalComponent::ConsumeFoodItem(const FFoodItemRow& FoodData)
{
    // 1. Dodaj Makro i Nawodnienie. NutritionKcal pełni rolę "szybkiej energii"
    //    (dawne Sugars) -> trafia do Glucose/Glikogenu w ConsumeFood().
    ConsumeFood(FoodData.ProteinG, FoodData.FatG, FoodData.NutritionKcal);
    CurrentHydration = FMath::Clamp(CurrentHydration + FoodData.WaterMl, 0.0f, 100.0f);

    bool bGotPoisoned = false;
    bool bGotHallucinations = false;

    // 2. Ryzyko Zatrucia (surowe mięso/ryba). bRequiresCooking oznacza, że surowa
    //    wersja jest ryzykowna; o tym, czy porcja jest ugotowana, decyduje warstwa
    //    gotowania/AI ustawiając PoisonChance=0 dla ugotowanego itemu.
    if (FoodData.PoisonChance > 0.0f)
    {
        if (FMath::FRand() <= FoodData.PoisonChance)
        {
            bIsPoisoned = true;
            bGotPoisoned = true;
        }
    }

    // 3. Ryzyko Halucynacji (grzyby, jagody)
    if (FoodData.HallucinationChance > 0.0f)
    {
        if (FMath::FRand() <= FoodData.HallucinationChance)
        {
            bIsHallucinating = true;
            bGotHallucinations = true;
        }
    }

    // 4. Bonus morale (L3/L4): ulubione jedzenie. MoraleBonus > 0 nadaje status
    //    Wysokiego Morale i regeneruje staminę proporcjonalnie do bonusu.
    if (FoodData.MoraleBonus > 0.0f)
    {
        bHasHighMorale = true;
        CurrentStamina = FMath::Clamp(CurrentStamina + FoodData.MoraleBonus, 0.0f, 100.0f);
    }

    // Odpalamy Event dla Blue-printów, żeby gracz mógł wyświetlić komunikaty lub animacje
    OnFoodConsumed(bGotPoisoned, bGotHallucinations);
}

// Funkcje pomocnicze dla pasków w UI
float UMaslowBiologicalComponent::GetHPPercent() const
{
    // Zabezpieczenie przed dzieleniem przez zero (choć zakladamy Max = 100)
    return FMath::Clamp(CurrentHP / 100.0f, 0.0f, 1.0f);
}

float UMaslowBiologicalComponent::GetHydrationPercent() const
{
    return FMath::Clamp(CurrentHydration / 100.0f, 0.0f, 1.0f);
}

float UMaslowBiologicalComponent::GetGlucosePercent() const
{
    return FMath::Clamp(Glucose / MaxGlucose, 0.0f, 1.0f);
}

float UMaslowBiologicalComponent::GetFatPercent() const
{
    return FMath::Clamp(BodyFat / MaxBodyFat, 0.0f, 1.0f);
}

// Główna funkcja decyzyjna (Sędzia Maslowa)
EMaslowPriority UMaslowBiologicalComponent::EvaluateCurrentNeed(bool bActionableOnly)
{
    // POZIOM 0: Przerwanie Krytyczne (Panic) - Nadpisuje wszystko.
    // Two layers (L3-02): (1) bIsInPanic = Neuroticism latch, rolled on metabolism cadence;
    // (2) CurrentHP <= CriticalHPThreshold = universal hard floor, read LIVE here every BT tick
    // (instant panic on a sudden lethal ApplyDamage, regardless of personality). NO RNG in this judge.
    if (bIsInPanic || CurrentHP <= CriticalHPThreshold)
    {
        return EMaslowPriority::Level_0_FightOrFlight;
    }

    // POZIOM 1: Fizjologia (Sprawdzamy od najszybciej zabijającego czynnika)

    // 1. Zamarznięcie zabija najszybciej. F2: Temperature NIE MA dziś BT-akcji (temperature slice TODO).
    //    Gdy bActionableOnly (z GetActionableNeed) → POMIŃ, by zmarznięty NPC nie zwracał need=0 i nie marzł
    //    bezczynnie, lecz realizował niższą ACTIONABLE potrzebę (pić/jeść/spać). Hipotermia C++ (cold-burn,
    //    obrażenia) działa niezależnie od tej gałęzi. Pełna drabina (bez flagi) wciąż zwraca Temperature dla HUD.
    if (!bActionableOnly && CurrentTemp <= CriticalTempThreshold)
    {
        return EMaslowPriority::Level_1_Temperature;
    }

    // 2. Brak wody zabija w kilka dni
    if (CurrentHydration <= CriticalHydrationThreshold)
    {
        return EMaslowPriority::Level_1_Hydration;
    }

    // 3. Zmęczenie (HoursAwake) — sen jest JEDYNĄ osią, którą sen realnie regeneruje (D1, TASK 2 2026-06-23).
    //    Próg = MentalFogThreshold (16h): NPC szuka snu od mgły umysłowej, ZANIM dojdzie do mikrosnów/omdlenia.
    //    CurrentStamina WYCIĘTA z drabiny potrzeb — teraz wyłącznie gate prędkości (GetStaminaSpeedMultiplier).
    if (HoursAwake >= MentalFogThreshold)
    {
        return EMaslowPriority::Level_1_Rest;
    }

    // 4. Brak szybkiej energii (Glukoza) - to znak, by coś zjeść.
    // EffectiveKcalThreshold = Neuroticism-modulated felt-hunger threshold, precomputed BESIDE this judge
    // on the metabolism cadence (UpdateEffectiveKcalThreshold). Read-only here — no Lerp/identity in the judge,
    // exact mirror of bIsInPanic. CriticalKcalThreshold (500) lives on as StableKcalThr (the calm floor).
    if (Glucose <= EffectiveKcalThreshold)
    {
        return EMaslowPriority::Level_1_Nutrition;
    }

    // Jeśli wszystko jest powyżej progów krytycznych - NPC jest szczęśliwy
    return EMaslowPriority::Satisfied;
}

// Maslow -> BT bridge (slice #1): translate the Maslow LEVEL into a CONCRETE, BT-actionable need.
// Single source of priority truth = EvaluateCurrentNeed(): it already walks the pyramid TOP-DOWN and
// returns the FIRST hit (panic -> temperature -> hydration -> rest/sleep -> nutrition/hunger -> satisfied),
// in EMaslowPriority order (0->6). We do NOT re-implement that ladder here (no drift) — we only MAP its
// result to the BP enum E_NeedState's byte values. Return is CONTRACT-LOCKED to E_NeedState:
//   0 = None, 1 = Hunger, 2 = Thirst, 3 = Sleep, 4 = Flee.
uint8 UMaslowBiologicalComponent::GetActionableNeed()
{
    // F2: bActionableOnly=true → drabina pomija niezaimplementowane poziomy (Temperature) i schodzi do
    // następnej potrzeby z gałęzią BT. Bez tego zmarznięty NPC zwracał need=0 i marzł bezczynnie.
    switch (EvaluateCurrentNeed(/*bActionableOnly=*/true))
    {
        case EMaslowPriority::Level_0_FightOrFlight: return 4; // Flee  (E_NeedState entry 4: name UNVERIFIED; branch disconnected in slice #1 — harmless, no decorator matches 4)
        case EMaslowPriority::Level_1_Hydration:     return 2; // Thirst  <-- slice #1 wired branch
        case EMaslowPriority::Level_1_Rest:          return 3; // Sleep
        case EMaslowPriority::Level_1_Nutrition:     return 1; // Hunger
        // No BT branch / E_NeedState value yet for these higher-priority-but-unimplemented needs:
        case EMaslowPriority::Level_1_Temperature:            // TODO: temperature slice
        case EMaslowPriority::Level_2_Safety:                 // TODO: safety slice
        case EMaslowPriority::Satisfied:
        default:                                     return 0; // None (idle/social)
    }
}

// L3-02: Neuroticism → panic CHANCE. Rolled on the metabolism cadence (mirror of the microsleep roll);
// sets the bIsInPanic latch that EvaluateCurrentNeed reads. The hard floor lives in EvaluateCurrentNeed.
void UMaslowBiologicalComponent::EvaluatePanicRoll()
{
    if (CurrentMaxHP <= 0.0f) { return; }   // EC: dead / nothing to measure against.

    // FLAG A: HP% vs CURRENT max (autophagy lowers it). NOT GetHPPercent() — that divides by literal 100.
    const float HPPercent = CurrentHP / CurrentMaxHP;

    // Lazy-cache the sibling identity (perf: no FindComponentByClass per tick across 500 NPCs).
    if (!CachedIdentity.IsValid())
    {
        if (AActor* OwnerActor = GetOwner())
        {
            CachedIdentity = OwnerActor->FindComponentByClass<UNPCIdentityComponent>();
        }
    }
    const float Neuroticism  = CachedIdentity.IsValid() ? CachedIdentity->GetNeuroticism() : 0.5f;
    const float EffThreshold = FMath::Lerp(StableThr, NervousThr, Neuroticism);
    const bool  bInBand      = (HPPercent < EffThreshold);
    const float PanicChance  = bInBand
        ? FMath::Clamp((EffThreshold - HPPercent) / PanicBand, 0.0f, MaxPanicChancePerTick)
        : 0.0f;

    bool bRolled = false;
    if (bIsInPanic)
    {
        // Hysteresis exit: recovered out of the band → calm. No re-roll while latched (fail-safe).
        // No PanicDuration timer (8s spec is for future flee L0-02/03, not in code) — HP recovery suffices.
        if (!bInBand) { bIsInPanic = false; }
    }
    else if (bInBand)
    {
        bRolled = (FMath::FRand() < PanicChance);
        if (bRolled)
        {
            bIsInPanic = true;
            // State transition (project rule: log on the decision, not per tick).
            UE_LOG(LogMaslow, Log,
                TEXT("[Panic:%s] ENTER stochastic panic — Neuroticism=%.2f EffThr=%.2f HP%%=%.2f PanicChance=%.2f."),
                GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
                Neuroticism, EffThreshold, HPPercent, PanicChance);
        }
    }

    // Killer-demo trace — Verbose (off in shipping; enable LogMaslow Verbose to watch both NPCs each tick).
    UE_LOG(LogMaslow, Verbose,
        TEXT("[PanicRoll:%s] Neuroticism=%.2f EffThr=%.2f HP%%=%.2f PanicChance=%.2f rolled=%d latched=%d"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
        Neuroticism, EffThreshold, HPPercent, PanicChance, bRolled ? 1 : 0, bIsInPanic ? 1 : 0);
}

// Hunger warstwa 1: Neuroticism → felt-hunger URGENCY. Computed on the metabolism cadence (sibling of
// EvaluatePanicRoll), writes the EffectiveKcalThreshold field that EvaluateCurrentNeed reads. Nervous NPCs
// eat EARLIER (higher kcal threshold → triggers at higher Glucose). NO personality math in the judge — exact
// mirror of how bIsInPanic is set beside the judge and only READ inside it. Cascade EHungerPhase untouched.
void UMaslowBiologicalComponent::UpdateEffectiveKcalThreshold()
{
    // Lazy-cache the sibling identity (perf: no FindComponentByClass per tick across 500 NPCs).
    // Shares CachedIdentity with EvaluatePanicRoll — one weak ptr resolved once for the whole component.
    if (!CachedIdentity.IsValid())
    {
        if (AActor* OwnerActor = GetOwner())
        {
            CachedIdentity = OwnerActor->FindComponentByClass<UNPCIdentityComponent>();
        }
    }
    const float Neuroticism = CachedIdentity.IsValid() ? CachedIdentity->GetNeuroticism() : 0.5f;
    // Lerp(calm, nervous, N): N=0 → StableKcalThr (eats latest), N=1 → NervousKcalThr (eats earliest).
    const float TraitFloor = FMath::Lerp(StableKcalThr, NervousKcalThr, Neuroticism);
    // APPETITE slice 1 (część E): rozpchany żołądek → wyższy trigger → je wcześniej (dodatnie sprzężenie).
    const float StretchBonus = FMath::Max(0.0f, GastricCapacity - BaseGastricCapacity) * StretchBonusScale;
    // APPETITE slice 2: LEPTYNA (ujemne sprzężenie, lipostat). dev>0 powyżej setpointu → LeptinBrake>0 → trigger NIŻEJ
    // (je rzadziej); dev<0 poniżej → LeptinBrake<0 → trigger WYŻEJ (głodniejszy). Clamp: floor (gruby je rzadko, nie
    // nigdy → bez cichego runaway w autofagię) i ceiling (wysycenie = "permanentnie głodny" przy MaxGlucose).
    const float LeptinDev   = BodyFat - LeptinSetpointFat;
    const float LeptinBrake = LeptinTriggerGain * LeptinDev;
    EffectiveKcalThreshold  = FMath::Clamp(TraitFloor + StretchBonus - LeptinBrake, LeptinTriggerFloor, LeptinTriggerCeiling);
}

// ============================================================================================
// APPETITE / GRUBAS slice 1 — proces jedzenia + routing makr + dual-driver pojemności
// BP rządzi rytmem (StartEating + AnimNotify→ConsumeBite + StopEating); C++ liczy skutek. Zero animacji w C++.
// ============================================================================================

void UMaslowBiologicalComponent::StartEating(AActor* Food, const FFoodItemRow& Meal, int32 BiteCount)
{
    if (bIncapacitated) { return; }   // EC-EAT-3: omdlały NPC nie zaczyna jeść.

    bIsEating      = true;
    EatTargetFood  = Food;
    bHasFoodActor  = (Food != nullptr);
    CurrentMeal    = Meal;
    TotalBites     = FMath::Max(1, BiteCount);
    RemainingBites = TotalBites;
    CurrentMealSize = 0.0f;

    UE_LOG(LogMaslow, Log,
        TEXT("[Eat:%s] StartEating — Volume=%.1f bites=%d (Carb=%.0f Fat=%.0f Prot=%.0f) StomachFill=%.1f Setpoint=%.1f"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
        Meal.Volume, TotalBites, Meal.NutritionKcal, Meal.FatG, Meal.ProteinG, StomachFill, GetSatietySetpoint());
}

// APPETITE slice 1b — BP-facing wrapper. Resolves the item's row from FoodTable (brain owns the data
// lookup) and starts the meal. Fail-safe: refuses (return false, NO StartEating) on invalid Food / null
// table / missing row, each with a Warning. The eat task gates the montage on the return value so an
// unresolved meal never leaves bIsEating=true hanging over a zero-volume session.
bool UMaslowBiologicalComponent::StartEatingItem(AItemBase* Food, UDataTable* FoodTable, int32 BiteCount)
{
    // FString (not TCHAR*): GetName() returns a temporary FString; caching *FString into a raw pointer
    // would dangle after the statement and print garbage. Hold the FString, deref (*) at each log site.
    const FString OwnerName = GetOwner() ? GetOwner()->GetName() : FString(TEXT("?"));

    if (!IsValid(Food))
    {
        UE_LOG(LogMaslow, Warning, TEXT("[Eat:%s] StartEatingItem REFUSED — Food invalid."), *OwnerName);
        return false;
    }
    if (FoodTable == nullptr)
    {
        UE_LOG(LogMaslow, Warning, TEXT("[Eat:%s] StartEatingItem REFUSED — FoodTable null."), *OwnerName);
        return false;
    }

    const FName RowName = Food->FoodTableRowName;   // data-driven: row id lives on the item, not hardcoded
    const FFoodItemRow* Row = FoodTable->FindRow<FFoodItemRow>(RowName, TEXT("StartEatingItem"), /*bWarnIfRowMissing*/ false);
    if (Row == nullptr)
    {
        UE_LOG(LogMaslow, Warning, TEXT("[Eat:%s] StartEatingItem REFUSED — row '%s' not found in '%s'."),
            *OwnerName, *RowName.ToString(), *FoodTable->GetName());
        return false;
    }

    StartEating(Food, *Row, BiteCount);
    return true;
}

// UTILITY (not biology) — prune null/destroyed entries from an actor array, return the new count.
// See header note + ROADMAP TECH-11 (perception Adds but never removes the Food array).
int32 UMaslowBiologicalComponent::CompactNullActors(TArray<AActor*>& Actors)
{
    Actors.RemoveAll([](const AActor* A) { return !IsValid(A); });
    return Actors.Num();
}

void UMaslowBiologicalComponent::ConsumeBite()
{
    // EC-EAT-3: omdlenie/mikrosen w trakcie → domknij sesję, nie gryź.
    if (bIncapacitated)
    {
        if (bIsEating) { StopEating(EEatStopReason::Incapacitated); }
        return;
    }
    if (!bIsEating) { return; }

    // EC-EAT-2: jedzenie zabrane/zniszczone (P2P) — realny item stał się stale → zamknij, zero crasha.
    if (bHasFoodActor && !EatTargetFood.IsValid())
    {
        StopEating(EEatStopReason::SourceGone);
        return;
    }
    if (RemainingBites <= 0)
    {
        StopEating(EEatStopReason::Finished);
        return;
    }

    // Jeden kęs = równa część zaplanowanego posiłku (Volume + makra). Rytm gryzień gra AnimNotify.
    const float Frac       = 1.0f / static_cast<float>(FMath::Max(1, TotalBites));
    const float BiteVolume = CurrentMeal.Volume * Frac;

    StomachFill     += BiteVolume;     // objętość (sytość) — NIEZALEŻNA od kcal
    CurrentMealSize += BiteVolume;     // rozmiar sesji → stretch-EMA przy StopEating
    DepositBiteMacros(CurrentMeal.NutritionKcal * Frac, CurrentMeal.FatG * Frac, CurrentMeal.ProteinG * Frac);
    CurrentHydration = FMath::Clamp(CurrentHydration + CurrentMeal.WaterMl * Frac, 0.0f, 100.0f);
    --RemainingBites;

    // Nadgryzienie itemu w świecie (mirror ACorpseBase::ExtractMeat) — jedzenie zostaje nadgryzione, nie znika.
    if (AItemBase* Item = Cast<AItemBase>(EatTargetFood.Get()))
    {
        Item->ConsumePortion(Frac);
    }

    UE_LOG(LogMaslow, Verbose,
        TEXT("[Eat:%s] bite vol=%.1f StomachFill=%.1f/%.1f mealSize=%.1f bitesLeft=%d Glucose=%.1f BodyFat=%.1f"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
        BiteVolume, StomachFill, GetSatietySetpoint(), CurrentMealSize, RemainingBites, Glucose, BodyFat);

    // Auto-stop: syty (satiety-stop, D-TWO-THRESH) LUB jedzenie wyczerpane.
    if (StomachFill >= GetSatietySetpoint())
    {
        StopEating(EEatStopReason::Full);
    }
    else if (RemainingBites <= 0)
    {
        StopEating(EEatStopReason::Finished);
    }
}

void UMaslowBiologicalComponent::StopEating(EEatStopReason Reason)
{
    if (!bIsEating) { return; }   // idempotent (EC-EAT-1/4 mogą wołać wielokrotnie)
    bIsEating = false;

    // Driver 1 — STRETCH (event-driven): rozmiar sesji rozpycha pojemność, ale TYLKO W GÓRĘ. Szybki EMA.
    if (CurrentMealSize > GastricCapacity)
    {
        GastricCapacity = FMath::Lerp(GastricCapacity, CurrentMealSize, StretchRate);
        GastricCapacity = FMath::Min(GastricCapacity, MaxGastricCapacity);
    }

    const TCHAR* ReasonStr =
        (Reason == EEatStopReason::Full)          ? TEXT("Full") :
        (Reason == EEatStopReason::Finished)      ? TEXT("Finished") :
        (Reason == EEatStopReason::Interrupted)   ? TEXT("Interrupted") :
        (Reason == EEatStopReason::SourceGone)    ? TEXT("SourceGone") : TEXT("Incapacitated");

    UE_LOG(LogMaslow, Log,
        TEXT("[Eat:%s] StopEating(%s) — mealSize=%.1f GastricCapacity=%.1f Setpoint=%.1f StomachFill=%.1f BodyFat=%.1f"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
        ReasonStr, CurrentMealSize, GastricCapacity, GetSatietySetpoint(), StomachFill, BodyFat);

    OnMealEnd(CurrentMealSize, Reason);   // BP: beknięcie / odłożenie nadgryzionego (CZY liczy C++, JAK gra BP)

    // Sprzątanie sesji. Nadgryzione jedzenie (Item->RemainingPortion > 0) ZOSTAJE w świecie (EC-EAT-1).
    EatTargetFood   = nullptr;
    bHasFoodActor   = false;
    CurrentMealSize = 0.0f;
    RemainingBites  = 0;
}

// Routing makr jednego kęsa → magazyn. Lustro kaskady spalania; różne efektywności depozytu (Atwater-grounded).
void UMaslowBiologicalComponent::DepositBiteMacros(float CarbG, float FatG, float ProteinG)
{
    // CARB → Glucose (cap MaxGlucose). Równoległy glikogen NIETKNIĘTY (zero regresji vs ConsumeFood).
    // Nadwyżka cukru PONAD cap Glucose → tap do tłuszczu (węgle tuczą SŁABO; bez refactoru na kaskadę).
    const float GlucoseSpace = FMath::Max(0.0f, MaxGlucose - Glucose);
    const float ToGlucose    = FMath::Min(CarbG, GlucoseSpace);
    Glucose += ToGlucose;
    const float CarbExcess = CarbG - ToGlucose;                 // ponad cap Glucose
    BodyFat += CarbExcess * CarbToFatEfficiency;                 // SŁABO (~0.75)
    GlycogenReserves = FMath::Clamp(GlycogenReserves + CarbG * 0.5f, 0.0f, MaxGlycogen);  // PARALLEL, jak dotąd

    // FAT → bezpośrednio BodyFat (magazyn ~1:1, makro grubasa).
    BodyFat += FatG * FatToFatEfficiency;                        // MOCNO (~0.95)

    // PROTEIN → odbudowa MaxHP (rewers autofagii, reużycie istniejącej gałęzi P*0.5); surplus → fat NAJSŁABIEJ.
    if (ProteinG > 0.0f)
    {
        const float Gain      = ProteinG * 0.5f;                // istniejąca semantyka ConsumeFood (zero nowej logiki HP)
        const float MaxHPSpace = FMath::Max(0.0f, AbsoluteMaxHP - CurrentMaxHP);
        const float ToMaxHP   = FMath::Min(Gain, MaxHPSpace);
        CurrentMaxHP += ToMaxHP;                                // RepairMuscle: MaxHP wraca ku AbsoluteMaxHP
        BodyFat += (Gain - ToMaxHP) * ProteinToFatEfficiency;   // surplus białka (gdy mięśnie pełne) → fat (~0.5)
    }

    BodyFat = FMath::Clamp(BodyFat, 0.0f, MaxBodyFat);
}

// Drenaż żołądka (trawienie opróżnia objętość) + Driver 2 SHRINK (głodówka kurczy pojemność). Na istniejącym ticku.
void UMaslowBiologicalComponent::UpdateAppetiteTick()
{
    // Trawienie: StomachFill maleje → NPC znów może zjeść. Makra JUŻ zdeponowane per kęs (gate część A: "każdy kęs już dolał").
    if (StomachFill > 0.0f)
    {
        StomachFill = FMath::Max(0.0f, StomachFill - DigestionRatePerTick);
    }

    // Driver 2 — SHRINK (time-driven): głodówka kurczy pojemność ku podłodze. Asymetria: ShrinkRate << StretchRate.
    // Warunek głodówki reużywa istniejący stan: NPC nie je i kaskada zeszła do spalania tłuszczu/głębiej.
    const bool bIsFasting = (!bIsEating && CurrentHungerPhase >= EHungerPhase::Phase_2_FatBurn);
    if (bIsFasting)
    {
        GastricCapacity = FMath::Lerp(GastricCapacity, BaseGastricCapacity, ShrinkRate);
        if (!bWasFasting)
        {
            UE_LOG(LogMaslow, Log,
                TEXT("[Eat:%s] FASTING — shrink begun (phase=%d) GastricCapacity=%.1f → base %.1f (rate=%.3f/tick)"),
                GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
                static_cast<int32>(CurrentHungerPhase), GastricCapacity, BaseGastricCapacity, ShrinkRate);
        }
    }
    bWasFasting = bIsFasting;
}