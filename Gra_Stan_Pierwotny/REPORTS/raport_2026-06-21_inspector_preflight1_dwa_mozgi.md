# Raport — INSPECTOR_CONSOLIDATION pre-flight #1: LUSTRO czy DRUGI MÓZG · 2026-06-21

> RECON ONLY — zero kodu, zero edycji. Cel: ustalić, czy metabolizm w BP_NPC_Character to lustro C++
> czy drugi, rozłączny mózg, ZANIM podepniemy UI inspektora.
> Źródła: żywy kod `Source/Stan_Pierwotny` (file:line) + Monolith live (PIE, CaldrethMap). `[SRC]`/`[LIVE]`.

═══════════════════════════════════════════════
## WERDYKT: **DRUGI, ROZŁĄCZNY MÓZG** 🔴
═══════════════════════════════════════════════
Metabolizm BP (`MetabolismStats`/`BodyComposition` + `HungerLevel/ThirstLevel/SleepLevel`) to osobny,
legacy model BP — **NIE lustro C++**. C++ `MaslowBiologicalComponent` go nie zna i nie synchronizuje.
Ten sam wzorzec rozjazdu co Maslow/BT Bridge.

## 1) Źródło prawdy — kto pisze BP-structy `[SRC]+[LIVE]`
- **C++ = 0 odwołań** do `ST_Metabolism`/`ST_BodyComposition`/`MetabolismStats`/`BodyComposition`/`CalculateTotalWeight` (grep `Source` = 0). C++ ich nie definiuje/czyta/pisze. **Brak sync C++→BP** (żadnej funkcji/eventu mirror).
- **Pisarz `MetabolismStats`:** wyłącznie BP-funkcja **`ApplyActionCost`** (3× `Set MetabolismStats` przez Break→Make→Set).
- **`BodyComposition`:** czytany tylko w `CalculateTotalWeight`; **statyczne defaulty** `Skeletal=20, Muscle=35, Fat=15`. Nie liczony z C++ tłuszczu/mięśni.
- **Równoległy mózg BP** (`BP_NPC_Character`, `has_tick=false`): zmienne `HungerLevel/HungerThreshold/ThirstLevel/ThirstThreshold/SleepLevel/SleepThreshold/SaftyLevel/CurrentNeed/isSleeping`; funkcje `IncreaseHunger`, `EvaluateNeeds`, `IncreaseThirst`, `IncreaseSleep`, `ScanForFood`, `ScanForDrinks`, `IsSleep?`, `CharacterStats` (init progów/poziomów), `ApplyActionCost`. Pre-C++ silnik NPC, rozłączny.
- **Kierunek sync:** żaden. Dwa niezależne światy.

## 2) Dowód live — te same pola, ta sama klatka, NPC `BP_NPC_Character_C_1` (CaldrethMap) `[LIVE]`
| Wielkość | C++ MaslowBiologicalComponent | BP (drugi mózg) |
|---|---|---|
| Głód | `Glucose=990/1000` (99%), `GlycogenReserves=1000`, `CurrentHungerPhase=Phase_0_Glucose` | `HungerLevel=70` (0-100) |
| Pragnienie | `CurrentHydration=98` | `ThirstLevel=70` |
| Sen | `HoursAwake=1.67`, `FatigueState=Awake` | `SleepLevel=40` |
| Stamina | `CurrentStamina=100` | (struct ST_Metabolism) |
| HP | `CurrentHP=100/CurrentMaxHP=100` | — (brak w BP) |
| Temp | `AmbientTemp=35.7`, `CurrentTemp=36.6` | — (brak w BP) |

**Rozjazd ewidentny:** C++ = „głód 99%, pragnienie 98%, świeży po 1.67h"; BP = `Hunger=70/Thirst=70/Sleep=40` w innej skali, bez korelacji. Lustro → liczby by się pokrywały. Nie pokrywają się → **drugi mózg**.

⚠️ **Caveat:** dokładnych pól `ST_Metabolism` (CurrentKcal/CurrentStamina) **nie dało się odczytać live** — pola BP user-structa mają GUID-owe nazwy wewnętrzne, niedostępne przez `get_editor_property` (repr `{}` = wartości domyślne/niezapełnione). Werdykt nie potrzebuje ich — opiera się na rozjeździe `HungerLevel/ThirstLevel/SleepLevel` vs C++ + 0 referencji C++.

## 3) Konsekwencja dla gate'a
Inspektor po konsolidacji **NIE może** czytać metaboliki przez `WBP_NPC`→`MetabolismStats`/`BodyComposition` (martwy/legacy model). Musi czytać **bezpośrednio C++ `MaslowBiologicalComponent`**. `WBP_NPC` w obecnej formie (Event Tick → pull `MetabolismStats`/`BodyComposition`) pokazuje liczby drugiego mózgu — mylące.

## 4) Tanie odczyty do gate'a `[SRC]`
**Enumy (`MaslowBiologicalComponent.h`):**
- `EHungerPhase` (h:107): `Phase_0_Glucose`, `Phase_1_Glycogen`, `Phase_2_FatBurn`, `Phase_3_Autophagy`, `Phase_4_Death`.
- `EFatigueState` (h:131): `Awake`, `MentalFog` (≥16h), `Microsleeps` (≥20h), `Collapsed` (≥24h).
- `EMaslowPriority` (h:118): `Level_0_FightOrFlight`, `Level_1_Temperature`, `Level_1_Hydration`, `Level_1_Rest`, `Level_1_Nutrition`, `Level_2_Safety`, `Satisfied`.
- `E_NeedState` (zwracany przez GetActionableNeed, `BTService_MaslowBlackboardSync.cpp:73`): `0=None, 1=Hunger, 2=Thirst, 3=Sleep, 4=Flee`.

**Sygnatury getterów (C++ `MaslowBiologicalComponent`):**
- `uint8 GetActionableNeed()` (h:648, cpp:736) — byte `E_NeedState`.
- `EFatigueState GetFatigueState() const` (h:258) — BlueprintPure.
- `float GetHydrationPercent() const` (h:489, cpp:670).
- HP: `CurrentHP`/`CurrentMaxHP` — UPROPERTY (live ✓), brak gettera %.
- Temp: `AmbientTemp` (h:339, BlueprintReadOnly), `CurrentTemp` — UPROPERTY (live ✓).
- Głód surowy: `Glucose`/`MaxGlucose`, `GlycogenReserves`/`MaxGlycogen`, `CurrentStamina`, `CurrentHungerPhase` — UPROPERTY (live ✓).

**Imię NPC (use-name/true-name):** **NIE ISTNIEJE.** Tożsamość = wyłącznie `int32 NPCId` (`UNPCIdentityComponent::GetNPCId()`). Brak pola `FName`/imienia. Jedyne „TrueName" w projekcie to rola POI (`CaldrethPOIMarker.h:37` „FireGate_TrueName_L5"), nie NPC.

**SceneCapture / render target / materiał portretowy:** **ZERO** (grep `Source` = 0, glob `Content` = 0). Portret = od zera.

---
**STOP — raport oddany. Nie kodowałem, nie edytowałem.**
