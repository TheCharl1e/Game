# L1 — Sen (Sleep / Rest)

**Oś:** L (NPC) · **Warstwa:** L1 · **Status:** ✅ · **Migrowano:** ETAP 2 (2026-07-01). Strzałka-w-dół: L1 zna tylko L0. Single-bridge BB = `BTService_MaslowBlackboardSync`.

Warstwa modelująca zmęczenie i regenerację NPC. Fizjologicznie oparta na modelu dwuprocesowym (Borbély 1982); `HoursAwake` = adenozyna (zużyty ATP). Zasada projektu: **C++ liczy** (timer, progi, los, blokada AI), **Blueprint tylko rysuje** (przez `BlueprintImplementableEvent`). Rozszerza istniejący `UMaslowBiologicalComponent` — bez nowego komponentu, bez nowego timera, bez Event Tick.

## ETAP 1 — Narastanie zmęczenia (silnik)

- Pole `HoursAwake` istniało już wcześniej (getter gotowy, nigdy nie rosło). ETAP 1 buduje silnik, który je inkrementuje na **istniejącym timerze metabolizmu (10 s tick)**:
  `HoursAwake += AwakeRatePerTick × ActivityMultiplier`.
- `ActivityMultiplier` = ten sam, którego używa metabolizm (`SetCurrentActionMultiplier`: Idle 1.0 / Work ~3.0 / Combat 5.0). Reużyty, nie tworzony od nowa → drwal (Work) męczy się szybciej niż NPC odpoczywający (D1: skalowanie aktywnością, emergentny realizm).
- `AwakeRatePerTick` dostrojony do skali „1 tick = X h gry" tak, by przy Idle NPC osiągał progi mgły (16 h) i omdlenia (24 h) w sensownym czasie gry. Wyliczone i zapisane w MECHANICS. Dostrajany do zegara świata ([[C0_clocks]], „jeden zegar"); fallback na UPROPERTY gdy brak zegara.
- Stan zmęczenia jako `EFatigueState`: **Awake / MentalFog / Microsleeps / Collapsed**, wyliczany z drabiny progów **16 / 20 / 24 h**. `UE_LOG` (`LogMaslow`) tylko na przejściu progu.
- Fail-safe: dostęp do świata/timera przez `if (UWorld* W = GetWorld())` (teardown może nie mieć świata).

## ETAP 2 — Skutki drabiny zmęczenia

Trzy skutki dopięte do istniejącego `UpdateFatigue()` (bez nowego ticka). Decyzje zatwierdzone 2026-06-19.

- **D-FOG — mgła liniowa.** `GetWorkEfficiencyMultiplier()` (BlueprintPure, wołany lazy przez pracę/crafting, nie co-tick): poniżej 16 h = 1.0, liniowa interpolacja **1.0 @16 h → 0.70 @24 h** (20 h ≈ 0.85), klamp trzyma 0.70 po 24 h. Zamiast skoku 0%→−30% — ciągłe narastanie. ETAP 2 tylko wystawia getter; system pracy sam przez niego mnoży wydajność. BP event `OnMentalFogStart/End` (feedback) tylko na przejściu progu.
- **D-MICRO — mikrosen.** Po ≥20 h, na ticku metabolizmu rzut kością `FMath::FRand() < MicrosleepChancePerTick` (jeśli `!bMicrosleeping && !bIncapacitated`) → `bMicrosleeping=true`, `OnMicrosleep()` (BP), jednorazowy `MicrosleepTimer` na `MicrosleepDuration` → `EndMicrosleep()` → flaga false + `OnMicrosleepEnd()`. Integracja BT: flaga trafia do Blackboard (`bIsMicrosleeping`) via `BTService_MaslowBlackboardSync`, by BT wstrzymał zadanie (sam dekorator = robota BP/BT).
- **D-COLLAPSE — omdlenie.** Przy ≥24 h `EnterCollapse()` (idempotentne): `bIncapacitated=true`, realnie unieruchamia NPC — `BrainComponent->StopLogic("Maslow_Collapse")` + `StopMovement()` + `CharacterMovement->DisableMovement()` (MOVE_None), wszystko za `IsValid`/`Cast`. `OnCollapse()` (BP: ragdoll). Wyjście `RecoverFromCollapse()` istnieje (RestartLogic + MOVE_Walking), w ETAP 2 wołane ręcznie/testowo — blokada jednokierunkowa.
- **StartSleep/StopSleep + reset + Rested** (scalony pierwotny ETAP 4). `StartSleep()` → `HoursAwake` maleje (`SleepRecoveryPerTick`, runtime ~4/tick) do 0; `StopSleep()` → buff **Rested** (`bIsRested`) → `RestedWorkBonus` podnosi wydajność pracy powyżej bazy. API dla BT/L3 (decyzja KIEDY spać = poza tą warstwą).

**Fail-safe'y (EC-1..EC-4):** `EndPlay` bezwarunkowo czyści `MicrosleepTimer` + resetuje flagi (martwy NPC w kontrakcie P2P nie zostawia wiszącego timera); brak AIController → blokada ruchu i tak działa, BT-stop pomijany bez crasha; każdy dostęp do TimerManager za `GetWorld()`; omdlenie nadpisuje i kasuje aktywny mikrosen.

## Kluczowe tunable

| Zmienna | Wartość | Uwaga |
|---|---|---|
| Progi (Fog/Micro/Collapse) | 16 / 20 / 24 h | z ETAP 1 |
| MinWorkEfficiency (mgła @24 h) | 0.70 | liniowo od 16 h |
| MicrosleepChancePerTick | 0.15 | rzut/tick przy ≥20 h |
| MicrosleepDuration | 3.0 s (kod: 1.5) | realtime, nie game-h |
| SleepRecoveryPerTick | ~4/tick | reset przy StartSleep |
| RestedWorkBonus | 1.20 (kod: additive +0.2) | buff po przespaniu |

## Most temperatury (jakość snu)

`GetTempQualityMultiplier()` moduluje jakość snu — komfort (~15–24°C otoczenia) = 1.0 (pełna regeneracja + Rested), poza komfortem <1.0 (sen płytki, brak buffa); dodatkowo śmiertelna utrata termoregulacji we śnie (amplifikacja kary zimna, hipotermia z fail-safe). Historycznie martwy (CurrentTemp było stałe 36.6), ożywiony przez warstwę AmbientTemp — model i getter w [[L1_temperature]] / [[C3_climate]].

**Źródło:** SLEEP_ENGINE_design.md + SLEEP_ENGINE_ETAP2_design.md (Game_git)
