# L1 — Głód (Hunger / Nutrition)

**Oś:** L (NPC) · **Warstwa:** L1 · **Status:** ✅ · **Migrowano:** ETAP 2 (2026-07-01). Strzałka-w-dół: L1 zna tylko L0. Single-bridge BB = `BTService_MaslowBlackboardSync`.

Warstwa L1 piramidy C/L/G: odżywianie NPC. Głód to NIE jeden pasek, tylko DWA rozjechane systemy — sygnał **ODCZUWANY** (felt, grelina/leptyna) napędza zwykłe jedzenie; stan **PRAWDZIWY** (true, kaskada kataboliczna) napędza skrajność. Ta sama głodówka, inna reakcja per NPC (OCEAN × człowieczeństwo).

## Kaskada kataboliczna (spalanie — `EHungerPhase`)
Kolejność wyczerpywania rezerw (JUŻ w kodzie):
- **Glucose → Glycogen → FatBurn (0.5× tempa) → Autophagy (MaxHP −0.25× burn, TRWALE) → Death.**
- W głębokiej ketozie/autofagii ODCZUWANY głód **PRZYGASA** (felt spada, true rośnie) — NPC umiera czując mniej głodu. To rdzeń felt-need vs true-state.
- Kaskada MUSI dalej liczyć fazy nawet w warstwie 1 — jest hookiem pod warstwę 2.

## Warstwy głodu
- **Warstwa 1 (głód odczuwany → jedzenie):** wyzwalacz `Glucose ≤ EffectiveKcalThreshold`. `GetActionableNeed()` mapuje `Level_1_Nutrition → 1 (Hunger)`. Neurotyczność wzmacnia pilność (je wcześniej/gwałtowniej), sumienny gromadzi/racjonuje.
- **Warstwa 2 (prawda kaskady → skrajność, PÓŹNIEJ):** głęboka faza pcha NaciskInstynktu; Restraint = człowieczeństwo × OCEAN(A,C); `SzansaPęknięcia = Clamp((Nacisk − Restraint)/Pasmo)`; rzut FRand → kradzież/kanibalizm. Tragiczny haczyk: człowieczeństwo SŁABNIE pod głodówką (Minnesota).

## Jedzenie jako TRZYMANY proces (SLICE 1)
Przerywalny proces, nie instant. BP gra anim, AnimNotify (ugryzienie) → C++ liczy skutek (zero nowego timera).
- `StartEating(Food)` — otwiera sesję, cache weak ptr (IsValid guard).
- `ConsumeBite()` — 1 kęs: `StomachFill += biteVolume`, routuj makra proporcjonalnie, dekrementuj `Food.RemainingPortion`. Early-return gdy `!bIsEating || bIncapacitated || !EatTargetFood.IsValid()`.
- `StopEating(EEatStopReason)` — Full / Finished / Interrupted / SourceGone / Incapacitated. Zamknięcie → `CurrentMealSize` → stretch-EMA. Nadgryzione jedzenie ZOSTAJE w świecie (RemainingPortion > 0).
- Głód zaspokojony PROPORCJONALNIE automatycznie — każdy kęs już dolał Glucose; przerwanie = po prostu mniej dolane.
- STOP automatyczny: `StomachFill >= SatietySetpoint` LUB Food wyczerpane.
- Edge-cases (fail-safe 500 NPC): śmierć/omdlenie/zabranie jedzenia (P2P)/panika L0 → StopEating z odpowiednim Reason, zero crasha.

## Rozpychanie żołądka — `GastricCapacity` (dual-driver)
Jeden float, popychany z dwóch stron w dwóch momentach:
- **STRETCH (event-driven, na zamknięciu posiłku):** gdy `MealSize > GastricCapacity` → `Lerp(GastricCapacity, MealSize, StretchRate)`, tylko W GÓRĘ, cap `MaxGastricCapacity`. Szybki.
- **SHRINK (time-driven, na ticku metabolizmu, gdy `bIsFasting`):** `Lerp(GastricCapacity, BaseGastricCapacity, ShrinkRate)`. Wolny (asymetria: `ShrinkRate << StretchRate`).
- `GetSatietySetpoint() = GastricCapacity × SatietyOverfillFactor (1.15)`. Overfill > 1 KONIECZNY — bez niego MealSize nigdy nie przekracza pojemności i stretch-EMA jest matematycznie zamrożony.
- Bez shrink EMA zamarza przy pustym żołądku. Shrink daje realne „głodówka kurczy żołądek" + refeeding-realizm. Skala per game-day (jak sen): stretch ~kilka dni przejadania, shrink ~tydzień głodówki.

## Makra → magazyn (routing, depozyt tłuszczu)
`FFoodMacros` (DataTable, data-driven): `Volume`, `CarbG`, `FatG`, `ProteinG`. kcal NIE storowane — liczone Atwater `Carb*4 + Protein*4 + Fat*9`.
- **OBJĘTOŚĆ ≠ KALORIE:** `StomachFill` napędzany `Volume` (sytość); energia/depozyt z makr (osobno). Gęste tłuste tuczy przy małej objętości; bujne nisko-kal wypełnia bez tycia.
- **Carb** → Glucose (cap 1000) → Glycogen (cap 1000) → BodyFat; efektywność NISKA `CarbToFatEfficiency ~0.75` (DNL).
- **Fat** → bezpośrednio BodyFat (cap 5000); efektywność WYSOKA `FatToFatEfficiency ~0.95` (~1:1) — makro grubasa.
- **Protein** → RepairMuscle / MaxHP (rewers autofagii), gluconeogeneza; surplus → BodyFat `ProteinToFatEfficiency ~0.5` (najniższa). Jedyne źródło odbudowy MaxHP po autofagii.
- Emergencja za darmo: mięso (Fat+Protein) = top survival ale grubas; jagody/korzenie (Carb) = chudszy NPC.

## Fat → izolacja (`InsulationFactor`, most do AmbientTemp)
`FatRatio = Clamp(BodyFat / MaxBodyFat)`; `InsulationFactor = Lerp(1.0, MinInsulationFromFat ~0.6, FatRatio)`. Grubas wolniej stygnie (przeżywa zimno Ocean/Mountain), przegrzewa się w Lava/AshSlope. JEDEN writer (fat); przyszłe ubranie → kompozycja mnożna, nie nadpisywanie. Aspekt odczuwania temp → [[L1_temperature]].

## Kompozycja triggera (FLAG 2)
`EffectiveKcalThreshold = TraitFloor(Neuroticism)` [asymetryczny `Lerp(StableKcalThr, NervousKcalThr, N)`] `+ StretchBonus(GastricCapacity)` [rozpchany = czuje pusto wcześniej, dodatnie sprzężenie] `− LeptinBrake(BodyFat)` [= 0 w slice 1].

## Kluczowe tunable
| Zmienna | Wartość |
|---|---|
| Atwater kcal/g (Carb/Protein/Fat) | 4 / 4 / 9 [LOCKED] |
| FatToFatEfficiency | ~0.95 |
| CarbToFatEfficiency | ~0.75 |
| ProteinToFatEfficiency | ~0.5 |
| SurplusToFatEfficiency | ~0.8 |
| MaxGlucose / MaxGlycogen / MaxBodyFat | 1000 / 1000 / 5000 |
| BaseGastricCapacity | = obecny rozmiar posiłku (recon #2) |
| MaxGastricCapacity | ~2.5× Base (skrajni 2–4×) |
| SatietyOverfillFactor | 1.15 |
| StretchRate / ShrinkRate (per game-day) | szybki / << StretchRate |
| MinInsulationFromFat | ~0.6 |
| LeptinBrake | 0 (slice 1, hook slice 2) |

## Status / staging
- **SLICE 1 (ZATWIERDZONY 2026-06-21, w toku):** proces jedzenia (Start/Bite/Stop) + FFoodMacros + routing makr→BodyFat + dwa progi + GastricCapacity dual-driver + fat→InsulationFactor + StretchBonus. Produkuje **runaway-grubasa BEZ hamulca — stan ZAMIERZONY**, weryfikowalny.
- **SLICE 2 (HOOK):** LeptinBrake(BodyFat) → ujemne sprzężenie → stabilizacja.
- **SLICE 3 (HOOK):** leptynooporność (przewlekły fat gasi hamulec) → trap.
- Warstwa 2 (skrajność: kradzież/kanibalizm, restraint, erozja) = osobny plaster PÓŹNIEJ, gdy dojrzeją L2 własność i L0 panika.
- Rozszerza `UMaslowBiologicalComponent` (patch, NIE nowy komponent/timer/Event Tick). C++ liczy wszystko, BP tylko rysuje.

**Źródło:** HUNGER_design.md + APPETITE_GRUBAS_design.md (Game_git)
