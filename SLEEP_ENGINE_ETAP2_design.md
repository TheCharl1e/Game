# SILNIK SNU — ETAP 2: SKUTKI ZMĘCZENIA — DESIGN GATE
> Zatwierdzony projekt do implementacji (decyzje architekta + Szymona, 2026-06-19).
> ROZSZERZAMY UMaslowBiologicalComponent (NIE nowy komponent, NIE nowy timer, NIE Event Tick).
> Buduje na ETAP 1 (HoursAwake rośnie, EFatigueState: Awake/MentalFog/Microsleeps/Collapsed).
> C++ liczy WSZYSTKO (mnożnik, los, flagi, blokada AI). Blueprint TYLKO rysuje (BlueprintImplementableEvent).
>
> ZAKRES (PIERWOTNY plan): skutki trzech stanów zmęczenia. NIE obejmuje: temperatury (ETAP 3),
> StartSleep/reset/buff Rested (ETAP 4). Patrz SLEEP_ENGINE_design.md (pełny design rdzenia).
>
> 🔶 **AKTUALIZACJA 2026-06-19 (doc → rzeczywistość; KOD = źródło prawdy):** Ten doc powstał PRZED
> decyzją użytkownika o SCALENIU ETAP 2 z reset/Rested. Faktyczny, zacommitowany, runtime-verified stan:
> **ETAP 2 = skutki drabiny (D-FOG/D-MICRO/D-COLLAPSE) + StartSleep/StopSleep + reset HoursAwake +
> buff Rested** (pierwotny ETAP 4, scalony świadomie). **Temperatura ODŁOŻONA** do osobnego etapu
> **AmbientTemp** — most temp wymaga temperatury OTOCZENIA, a `CurrentTemp` to temp CIAŁA (stała 36.6,
> ustawiana raz w konstruktorze → cały podsystem temp był martwy; patrz diagnoza cold-burn).
> `GetTempQualityMultiplier()` zahardkodowany na 1.0 z TODO AmbientTemp. Implementacja różni się
> KOSMETYCZNIE od snippetów niżej (świadomie zostawione, działają): hardcode 0.7 (nie MinWorkEfficiency
> UPROPERTY), MicrosleepDuration=1.5 (nie 3.0), RestedWorkBonus additive +0.2 (nie mnożnik 1.20),
> OnMentalFogStart/End (nie OnMentalFogChanged), omdlenie bez EnterCollapse/RecoverFromCollapse/
> DisableMovement. Naprawione 2 realne bugi: mikrosen guard `!bIncapacitated` + EndPlay cleanup (EC-1).
> Snippety C++ niżej = PIERWOTNY plan, nie 1:1 z kodem.

═══════════════════════════════════════════════════════════
# DECYZJE PROJEKTOWE (zatwierdzone — NIE zmieniać bez architekta)
═══════════════════════════════════════════════════════════
| # | Skutek | Decyzja | Tryb |
|---|---|---|---|
| D-FOG | Mgła umysłowa (spadek pracy) | **LINIOWO** — narasta stopniowo z HoursAwake | ciągły, lazy w getterze |
| D-MICRO | Mikrosen | **Krótkie zawieszenie akcji** (kilka sek) | losowy, na ticku metabolizmu |
| D-COLLAPSE | Omdlenie (Collapsed) | **C++ BLOKUJE AI/ruch** — NPC realnie unieruchomiony | twarda akcja przy progu |

═══════════════════════════════════════════════════════════
# D-FOG — MGŁA LINIOWA (GetWorkEfficiencyMultiplier)
═══════════════════════════════════════════════════════════
Zamiast skoku 0%→−30%, ciągła interpolacja między progiem mgły (16h) a omdleniem (24h).

```cpp
// BlueprintPure getter — wołany LAZY przez crafting/pracę, NIE co-tick.
float UMaslowBiologicalComponent::GetWorkEfficiencyMultiplier() const
{
    // Poniżej progu mgły: pełna wydajność.
    if (HoursAwake < MentalFogThreshold)   // 16h
    {
        return bIsRested ? RestedWorkBonus : 1.0f;   // bIsRested = ETAP 4 hook, na razie zwykle false
    }
    // Liniowo od 1.0 (16h) do MinWorkEfficiency (24h).
    const float Alpha = FMath::Clamp(
        (HoursAwake - MentalFogThreshold) / (CollapseThreshold - MentalFogThreshold),
        0.0f, 1.0f);
    return FMath::Lerp(1.0f, MinWorkEfficiency, Alpha);   // 1.0 → 0.7
}
```

- 16h → 1.0 (100%), 20h → 0.85, 24h → 0.70. Płynnie. Po 24h klamp trzyma 0.70.
- **Koszt: zero.** Jeden Lerp+Clamp, lazy. Brak ticka, brak pamięci per-instancja (poza UPROPERTY-konfigami).
- KTO CZYTA: przyszły system crafting/pracy mnoży swoją wydajność przez ten getter. ETAP 2 tylko WYSTAWIA getter.
- bIsRested (+20%) to ETAP 4 — tu tylko czytamy flagę jeśli istnieje; nie ustawiamy.

## BP event (opcjonalny feedback wizualny mgły)
```cpp
UFUNCTION(BlueprintImplementableEvent, Category="Maslow|Sleep")
void OnMentalFogChanged(float WorkEfficiency);   // C++ woła przy ZMIANIE stanu na/z MentalFog
```
- Woływany TYLKO na przejściu progu (Awake↔MentalFog), NIE co-tick. BP może desaturować ekran/pokazać ikonę.

═══════════════════════════════════════════════════════════
# D-MICRO — MIKROSEN (losowe zawieszenie akcji)
═══════════════════════════════════════════════════════════
Po przekroczeniu MicrosleepThreshold (20h), na ISTNIEJĄCYM ticku metabolizmu rzut kością.

## Nowe pola (per-instancja):
```cpp
UPROPERTY(BlueprintReadOnly, Category="Maslow|Sleep")
bool bMicrosleeping = false;          // czy AKTUALNIE w mikrośnie

FTimerHandle MicrosleepTimer;          // NIE UPROPERTY — wewnętrzny, jednorazowy, sam się kasuje
```

## Nowe UPROPERTY-konfigi (per-klasa, EditDefaultsOnly, data-driven):
```cpp
UPROPERTY(EditDefaultsOnly, Category="Maslow|Sleep", meta=(ClampMin="0.0", ClampMax="1.0"))
float MicrosleepChancePerTick = 0.15f;    // szansa na mikrosen w jednym ticku metabolizmu (≥20h)

UPROPERTY(EditDefaultsOnly, Category="Maslow|Sleep", meta=(ClampMin="0.5"))
float MicrosleepDuration = 3.0f;          // ile sekund trwa zawieszenie (realtime, nie game-h)
```

## Logika (wewnątrz UpdateFatigue(), po przeliczeniu HoursAwake):
```cpp
// Tylko jeśli ≥20h, NIE już w mikrośnie, NIE omdlony (Collapsed nadpisuje):
if (HoursAwake >= MicrosleepThreshold && !bMicrosleeping && !bIncapacitated)
{
    if (FMath::FRand() < MicrosleepChancePerTick)
    {
        bMicrosleeping = true;
        UE_LOG(LogMaslow, Log, TEXT("[Sleep] %s: MIKROSEN start (HoursAwake=%.2f)"),
               *GetNameSafe(GetOwner()), HoursAwake);
        OnMicrosleep();   // BP: kiwnięcie głową / zamarcie
        if (UWorld* W = GetWorld())   // fail-safe: świat może nie istnieć w teardown
        {
            W->GetTimerManager().SetTimer(MicrosleepTimer, this,
                &UMaslowBiologicalComponent::EndMicrosleep, MicrosleepDuration, false);
        }
    }
}
```

```cpp
void UMaslowBiologicalComponent::EndMicrosleep()
{
    bMicrosleeping = false;
    UE_LOG(LogMaslow, Log, TEXT("[Sleep] %s: MIKROSEN koniec"), *GetNameSafe(GetOwner()));
    OnMicrosleepEnd();   // BP: powrót do normalnej animacji
}
```

## BP eventy:
```cpp
UFUNCTION(BlueprintImplementableEvent, Category="Maslow|Sleep")
void OnMicrosleep();      // start zawieszenia
UFUNCTION(BlueprintImplementableEvent, Category="Maslow|Sleep")
void OnMicrosleepEnd();   // koniec
```

## 🔴 INTEGRACJA Z BT (krytyczne — żeby zawieszenie było logiczne, nie tylko wizualne):
- `bMicrosleeping` musi trafić do Blackboard, żeby BT wstrzymał bieżące zadanie.
- `BTService_MaslowBlackboardSync` JUŻ czyta komponent (FindComponentByClass) — DODAĆ klucz BB `bIsMicrosleeping` (bool) do tego serwisu (jeden getter więcej + jeden SetValueAsBool).
- BT: dekorator na bieżącym zadaniu — jeśli `bIsMicrosleeping`, abortuj/pauzuj. (Sam dekorator = robota BP/BT, NIE w tym zadaniu C++; ETAP 2 tylko WYSTAWIA klucz BB.)

## Koszt:
- Pamięć: 1 bool + 1 FTimerHandle (~16B) per NPC = ~17B × 500 = ~8.5 KB. Ale timer ISTNIEJE tylko podczas aktywnego mikrosnu (jednorazowy, samokasujący) — w praktyce kilka NPC naraz.
- CPU: 1× FRand() na tick TYLKO dla NPC ≥20h. Znikome.

═══════════════════════════════════════════════════════════
# D-COLLAPSE — OMDLENIE: C++ BLOKUJE AI/RUCH (najmocniejszy)
═══════════════════════════════════════════════════════════
Przy ≥24h (CollapseThreshold) C++ REALNIE wyłącza NPC: zatrzymuje Behavior Tree + ruch.

## Nowe pole (per-instancja):
```cpp
UPROPERTY(BlueprintReadOnly, Category="Maslow|Sleep")
bool bIncapacitated = false;          // czy NPC jest unieruchomiony (omdlony)
```

## Wejście w omdlenie (wywoływane raz, przy przejściu progu w UpdateFatigue):
```cpp
void UMaslowBiologicalComponent::EnterCollapse()
{
    if (bIncapacitated) return;   // idempotentne
    bIncapacitated = true;

    // Jeśli akurat w mikrośnie — sprzątnij (omdlenie nadpisuje).
    if (bMicrosleeping)
    {
        bMicrosleeping = false;
        if (UWorld* W = GetWorld()) { W->GetTimerManager().ClearTimer(MicrosleepTimer); }
    }

    // FAIL-SAFE: akcja na INNYM aktorze (AIController) — wszystko przez IsValid.
    AActor* OwnerActor = GetOwner();
    if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
    {
        if (AAIController* AC = Cast<AAIController>(OwnerPawn->GetController()))
        {
            if (UBrainComponent* Brain = AC->GetBrainComponent())
            {
                Brain->StopLogic(TEXT("Maslow_Collapse"));   // zatrzymuje BT
            }
            AC->StopMovement();
        }
        // ruch postaci też zatrzymujemy bezpośrednio (gdyby był momentum):
        if (UCharacterMovementComponent* Move = OwnerPawn->FindComponentByClass<UCharacterMovementComponent>())
        {
            Move->StopMovementImmediately();
            Move->DisableMovement();   // wejście w MOVE_None
        }
    }

    UE_LOG(LogMaslow, Warning, TEXT("[Sleep] %s: OMDLENIE — AI+ruch ZABLOKOWANE (HoursAwake=%.2f)"),
           *GetNameSafe(OwnerActor), HoursAwake);
    OnCollapse();   // BP: ragdoll, NPC pada
}
```

## Wyjście z omdlenia (API dla ETAP 4 resetu — w ETAP 2 tylko wystawiamy, nie wołamy automatycznie):
```cpp
UFUNCTION(BlueprintCallable, Category="Maslow|Sleep")
void RecoverFromCollapse()
{
    if (!bIncapacitated) return;
    bIncapacitated = false;

    AActor* OwnerActor = GetOwner();
    if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
    {
        if (UCharacterMovementComponent* Move = OwnerPawn->FindComponentByClass<UCharacterMovementComponent>())
        {
            Move->SetMovementMode(MOVE_Walking);
        }
        if (AAIController* AC = Cast<AAIController>(OwnerPawn->GetController()))
        {
            if (UBrainComponent* Brain = AC->GetBrainComponent())
            {
                Brain->RestartLogic();
            }
        }
    }
    UE_LOG(LogMaslow, Log, TEXT("[Sleep] %s: wybudzenie z omdlenia"), *GetNameSafe(OwnerActor));
    OnRecoverFromCollapse();   // BP: wstaje z ragdolla
}
```
- W ETAP 2 blokada jest JEDNOKIERUNKOWA do testów (wszedł = leży). RecoverFromCollapse istnieje, ale wołany ręcznie (test) lub przez ETAP 4 (reset HoursAwake).

## BP eventy:
```cpp
UFUNCTION(BlueprintImplementableEvent, Category="Maslow|Sleep")
void OnCollapse();              // NPC pada (ragdoll)
UFUNCTION(BlueprintImplementableEvent, Category="Maslow|Sleep")
void OnRecoverFromCollapse();   // NPC wstaje
```

## 🛡️ FAIL-SAFE'Y (edge-cases — KRYTYCZNE dla 500 NPC):

### EC-1: NPC ginie/niszczony w trakcie omdlenia LUB mikrosnu (scenariusz kontraktu P2P)
W `EndPlay` LUB `OnUnregister`/`UninitializeComponent` komponentu — BEZWARUNKOWE sprzątanie:
```cpp
void UMaslowBiologicalComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(MicrosleepTimer);
        // (istniejący timer metabolizmu też tu czyszczony — sprawdź czy ETAP 1 już to robi)
    }
    bMicrosleeping = false;
    bIncapacitated = false;
    Super::EndPlay(Reason);
}
```
Cel: martwy NPC NIE zostawia wiszącego timera ani flagi blokady. AIController i tak ginie z pawnem, ale flagi czyścimy jawnie.

### EC-2: NPC nie ma AIController (np. opętany przez gracza, albo spawn bez controllera)
Wszystkie Cast<AAIController> przez `if`. Jeśli brak — blokada ruchu (CharacterMovement) i tak działa; BT-stop pomijany bez crasha.

### EC-3: GetWorld() null podczas teardown
Każdy dostęp do TimerManager poprzedzony `if (UWorld* W = GetWorld())`.

### EC-4: Collapsed nadpisuje mikrosen
EnterCollapse() jawnie kasuje aktywny mikrosen (patrz wyżej). Mikrosen NIE odpala przy bIncapacitated (warunek w UpdateFatigue).

## Koszt:
- Pamięć: 1 bool per NPC = 500B przy 500 NPC. Znikome.
- CPU: akcja jednorazowa przy przejściu progu, NIE co-tick. Zero ciągłego narzutu.

═══════════════════════════════════════════════════════════
# PODŁĄCZENIE DO ISTNIEJĄCEGO UpdateFatigue() (ETAP 1)
═══════════════════════════════════════════════════════════
ETAP 1 już liczy HoursAwake i EFatigueState z drabiną progów + UE_LOG na przejściu.
ETAP 2 DODAJE do tej samej funkcji (NIE nowy tick):

1. Po wyliczeniu nowego EFatigueState:
   - jeśli wszedł w Collapsed (≥24h) i !bIncapacitated → EnterCollapse().
   - jeśli wszedł/wyszedł z MentalFog → OnMentalFogChanged(GetWorkEfficiencyMultiplier()).
2. Rzut na mikrosen (sekcja D-MICRO) — tylko jeśli ≥20h && !bMicrosleeping && !bIncapacitated.

Wszystko na ISTNIEJĄCYM timerze metabolizmu. Zero nowych ticków poza jednorazowym MicrosleepTimer.

═══════════════════════════════════════════════════════════
# WARTOŚCI DO MECHANICS (po implementacji — [LOCKED] po dostrojeniu)
═══════════════════════════════════════════════════════════
| Zmienna | Typ | Wartość proponowana | Status |
|---|---|---|---|
| MinWorkEfficiency (mgła przy 24h) | float | **0.70** (−30% przy omdleniu, liniowo od 16h) | [TBD→approve] |
| MicrosleepChancePerTick | float | **0.15** (szansa/tick ≥20h) | [TBD→tune] |
| MicrosleepDuration | float (s) | **3.0** | [TBD→tune] |
| RestedWorkBonus (hook ETAP 4) | float | **1.20** | [LOCKED, nieaktywne w ETAP 2] |
| Collapse blokuje BT+ruch | bool | **tak** (StopLogic + DisableMovement) | [LOCKED] |

Uwaga: MentalFogThreshold=16, MicrosleepThreshold=20, CollapseThreshold=24 JUŻ są z ETAP 1.

═══════════════════════════════════════════════════════════
# WERYFIKACJA (Claude Code — twarde dane, zero placeholderów)
═══════════════════════════════════════════════════════════
Po buildzie (edytor zamknięty → UHT), PIE + odczyt z dysku Saved/Logs + Monolith pie_get_object_properties:

1. **D-FOG liniowo:** wymuś HoursAwake=16 → GetWorkEfficiencyMultiplier()==1.0;
   HoursAwake=20 → ==0.85 (±0.01); HoursAwake=24 → ==0.70. Sprawdź interpolację, nie skok.
2. **D-MICRO:** HoursAwake≥20 przez N ticków → log `[Sleep] ... MIKROSEN start` pada,
   bMicrosleeping=true (Monolith live read), po MicrosleepDuration → `MIKROSEN koniec`, flaga false.
   Sprawdź że BB klucz bIsMicrosleeping się ustawia (jeśli BTService rozszerzony w tym zadaniu).
3. **D-COLLAPSE:** HoursAwake≥24 → log `[Sleep] ... OMDLENIE — AI+ruch ZABLOKOWANE`,
   bIncapacitated=true (live read). Sprawdź że BrainComponent zatrzymany (BT nie tika)
   i MovementMode==MOVE_None. OnCollapse zawołany (BP licznik/log).
4. **EC-1 (śmierć w omdleniu):** zniszcz NPC gdy bIncapacitated=true → brak warningów o wiszącym
   timerze, brak crasha. (DestroyActor w teście → EndPlay czyści.)
5. **EC-4:** NPC w mikrośnie wchodzi w omdlenie → mikrosen skasowany, bMicrosleeping=false,
   bIncapacitated=true.

WSZYSTKIE liczby z LIVE logu/obiektu — nie z placeholderów. Format logu dokładnie jak w kodzie.

═══════════════════════════════════════════════════════════
# CO TO ZADANIE ROBI / CZEGO NIE ROBI
═══════════════════════════════════════════════════════════
✅ ROBI:
- GetWorkEfficiencyMultiplier() — liniowa mgła (lazy getter).
- Mikrosen: bMicrosleeping + MicrosleepTimer + OnMicrosleep/End + rzut na ticku.
- Omdlenie: bIncapacitated + EnterCollapse (StopLogic+DisableMovement) + OnCollapse + RecoverFromCollapse + OnRecoverFromCollapse.
- Fail-safe'y EC-1..EC-4 (EndPlay sprzątanie, IsValid wszędzie).
- (Opcjonalnie, jeśli mieści się: klucz BB bIsMicrosleeping w BTService_MaslowBlackboardSync.)
- Wartości do MECHANICS, runtime-weryfikacja.

❌ NIE ROBI (HOOKI na przyszłość) — ⚠️ ZAKTUALIZOWANE 2026-06-19 po scaleniu A+B:
- Temperatura / jakość snu / śmierć z zimna we śnie → ODŁOŻONE do etapu **AmbientTemp** (CurrentTemp = temp ciała 36.6 stała → most temp martwy bez wejścia otoczenia; temp-quality zahardkodowany 1.0).
- ~~StartSleep/StopSleep, reset HoursAwake, buff Rested → ETAP 4~~ → **JUŻ ZROBIONE w tym passie** (scalenie, runtime-verified: StartSleep→HoursAwake maleje 4/tick→StopSleep→Rested + GetWorkEfficiencyMultiplier=1.2).
- Dekorator BT pauzujący zadanie na mikrosen → robota BP/BT (ETAP 2 wystawia tylko klucz BB).
- Automatyczne wybudzenie z omdlenia → ETAP 4 (reset). Tu blokada jednokierunkowa + ręczne RecoverFromCollapse.
- "Gorszy osąd" we mgle (mylenie wróg/sojusznik) → HOOK na OCEAN/BT, nie teraz.

═══════════════════════════════════════════════════════════
# REGUŁA PATCHOWANIA
═══════════════════════════════════════════════════════════
To rozszerzenie ISTNIEJĄCEGO MaslowBiologicalComponent.{h,cpp}. NIE regeneruj całych plików —
dodaj nowe pola/funkcje do istniejących, wepnij wywołania w istniejący UpdateFatigue().
Patrz CODE_REGISTRY (komponent dodany c22d414, ETAP 1 a7efcdb).
