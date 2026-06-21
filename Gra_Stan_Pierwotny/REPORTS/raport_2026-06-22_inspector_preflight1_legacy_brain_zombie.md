# Raport — INSPECTOR_CONSOLIDATION pre-flight #1b: legacy BP-mózg ŻYWY/MARTWY · 2026-06-22

> RECON ONLY — zero kodu, zero edycji. Pytanie: czy legacy BP-mózg potrzeb (HungerLevel/ThirstLevel/
> SleepLevel + EvaluateNeeds/IncreaseHunger/ApplyActionCost/ScanForFood/CharacterStats) ŻYJE, czy MARTWY.
> Źródła: żywy kod `Source/Stan_Pierwotny` (file:line) + Monolith live (PIE, CaldrethMap). `[SRC]`/`[LIVE]`.

═══════════════════════════════════════════════
## WERDYKT: **ZOMBIE** — okablowany i wciąż piszący klucz decyzyjny, ale w runtime zamrożony → decyzje napędza C++.
═══════════════════════════════════════════════
Jednym zdaniem: legacy BP-mózg jest podpięty pod żywy event i **wciąż pisze ten sam blackboard `CurrentNeed` co C++**
(plaster #1 NIE usunął BP-writera), ale jego stan **nie advansuje w PIE** (poziomy stoją), więc faktycznie nie
steruje zachowaniem — steruje C++.

## 1) Kto woła legacy funkcje `[live graf]`
- **`CharacterStats`** — wołane RAZ w `BeginPlay` (init progów/poziomów). `BP_NPC_Character.has_tick=false`, brak timera (search „Timer"=0).
- **`IncreaseHunger`** — wołane z eventu **`On Hour Passed (TimeManager)`** (ComponentBoundEvent, delegate `BP_DayNightCycle.OnHourPassed`). `IncreaseHunger` = `Set HungerLevel → Print String`.
- **`IncreaseThirst`** — również wołane w EventGraph (CallFunction_6). `IncreaseSleep` — zdefiniowane.
- **`EvaluateNeeds`** — wołane w EventGraph (CallFunction_7), w łańcuchu OnHourPassed.
- **Broadcast potwierdzony:** `BP_DayNightCycle.TIME` ma węzeł **`Call On Hour Passed`** (gated LastHour≠Hours) — delegat realnie fire'uje co godzinę gry.
- `ScanForFood/ScanForDrinks/ApplyActionCost` — funkcje istnieją; `ApplyActionCost` jest jedynym pisarzem `MetabolismStats` (3× Set). Pełnej listy ich callerów nie domknąłem — ale nieistotne dla werdyktu (patrz #3 frozen).

→ **NIE sieroty** — legacy loop jest okablowany pod żywy delegat OnHourPassed.

## 2) Czy BP-pola napędzają DECYZJE `[SRC + live graf]`
- **`EvaluateNeeds` pisze blackboard `CurrentNeed`**: drabina priorytetów na `HungerLevel/ThirstLevel/SleepLevel/SaftyLevel` vs progi → każda gałąź `SetValueAsEnum(KeyName="CurrentNeed", E_NeedState)` (×4, `MakeLiteralName="CurrentNeed"`) + `Set CurrentNeed` (BP var) + Print „Hunger/Thirst/Sleep/None".
- **C++ pisze TEN SAM klucz**: `BTService_MaslowBlackboardSync` co 1.0s `SetValueAsEnum(CurrentNeed, GetActionableNeed())` (`cpp:77`). Komentarz „C++ is now the sole writer" (l.76/88) — **NIEAKTUALNE**: BP wciąż pisze.
- → **Podwójny pisarz `CurrentNeed`.** BT czyta blackboard `CurrentNeed`; faktyczny zwycięzca = C++ (cadence 1s vs godzinowy BP + BP frozen).
- **Residual nierozstrzygnięty:** `isSleeping` (BP) vs `bIsSleeping` (C++) — które czyta animacja/BT, nie potwierdziłem (BT_NPC to `UBehaviorTree`, nieczytelny przez `blueprint_query`).

## 3) LIVE — ruch vs statyka, NPC `BP_NPC_Character_C_1` `[LIVE]`
| t | CurrentTime (gra) | BP Hunger/Thirst/Sleep | BP CurrentNeed | C++ Glucose | C++ GetActionableNeed |
|---|---|---|---|---|---|
| T0 | 12.51 | **70 / 70 / 40** | NONE | 990 | 0 (None) |
| T1 (+~4h gry) | 16.59 | **70 / 70 / 40** | NONE | **970** | 0 (None) |

→ Przez ~4h gry C++ Glucose spadło (C++ żyje), a **BP poziomy zamrożone na init 70/70/40**. Mimo broadcastu OnHourPassed increment BP realnie się **nie wykonuje** → EvaluateNeeds zawsze NONE. ⚠️ Najpewniejsza przyczyna (inference): `On Hour Passed` to ComponentBoundEvent, a `TimeManager` to runtime-przypisana referencja aktora (nie komponent) → bind nie łapie. Hard-fakt = poziomy stoją; mechanizm = do potwierdzenia. NPC nie wykonywał akcji BP-need (ScanForFood) — `CurrentNeed=NONE`.

## Rozbicie per-potrzeba
**Brak czystego podziału.** Wszystkie trzy (hunger/thirst/sleep) są **i** liczone przez C++ (Glucose/Hydration/HoursAwake→`GetActionableNeed`) **i** okablowane w BP (`*Level`→`EvaluateNeeds`→`CurrentNeed`). C++ wygrywa runtime dla **całej trójki** (1s cadence + BP frozen). BP = martwy-ale-podpięty duplikat dla wszystkich.

## 🔴 Konsekwencje dla architekta
1. **Plaster #1 NIEDOKOŃCZONY** — BP `EvaluateNeeds` wciąż drugi pisarz `CurrentNeed`. Gdyby poziomy BP ruszyły lub bind OnHourPassed zaczął łapać → BP stomp'uje decyzję C++ co godzinę gry. Bomba uśpiona.
2. **Print String spam** (Hunger/Thirst/Sleep/None/Hello) w EvaluateNeeds/IncreaseHunger/WBP_NPC.
3. Inspektor po konsolidacji ma czytać **C++ `MaslowBiologicalComponent`**, ignorować BP-need-mózg (martwy duplikat).

---
**STOP — werdykt: ZOMBIE.** Nie kodowałem, nie edytowałem.
