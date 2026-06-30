# GATE: ICONSUMABLE-EAT-01 — odblokowanie BTTask_Eat przez interfejs IConsumable

Data otwarcia: 2026-06-30 · Status: **C++/IConsumable READY — PARKED under BTTASK_EAT_WIRING_01** · Target: E:\Game_58 (UE 5.8)
> Werdykt architekta 2026-06-30: rama A/B była fałszywa. Rich model (StartEating/ConsumeBite/StopEating) to zatwierdzony+zshipowany APPETITE_GRUBAS (nie otwarta decyzja); B (kasacja) odpada. Brak okablowania = wiring-debt w MILESTONES. C++ + graf BP IConsumable ZOSTAJĄ (nie cofamy) — okablowanie kanonicznego eat-taska przejmuje **`_gates/BTTASK_EAT_WIRING_01.md`**.
Pełna diagnoza i powierzchnia: `Gra_Stan_Pierwotny/REPORTS/gate_request_2026-06-30_iconsumable_bp_food.md` (źródło prawdy, nie duplikuję).

## Problem (1 zdanie)
W 5.8 `BP_Food` jest reparentowany na `AAffordanceSourceActor` (żywy forage L0-TA-S1), a warstwa jedzenia zakłada `AItemBase` — `BTTask_Eat` nie kompiluje (pin `Food: AItemBase*` ≠ `Cast To BP_Food`), więc pętla jedzenia jest zerwana.

## RESOLVED (zalockowane)
- **Kierunek = `IConsumable`** (decyzja dyrektora). NIE merge baz (rejestracja afordancji jest bezwarunkowa — `AffordanceSourceActor.cpp:14-26`), NIE robocopy 5.7→5.8 (cofnęłoby afordancje + 4 nowe pliki). BP_Food zostaje `AffordanceSourceActor` i DODATKOWO niesie jadalność.
- Diagnoza dowiedziona z logu (`Game_58.log` 12:13:53) + read-only audyt call-site'ów AItemBase (Maslow:1055/1126, NPCPerceptionComponent, graf BTTask_Eat).

## DEFAULTS (rekomendacja executora — przyjmę, jeśli nie nadpiszesz)
- **D2 = `UConsumableComponent`** (nośnik food-data: `FoodTableRowName`/`RemainingPortion`/`ConsumePortion`, wpinany na BP_Food obok afordancji; data-driven, „C++ mózg / BP ciało", reużywalny dla corpse).
- **D7 = slice minimalny NAJPIERW**: odblokować tylko BTTask_Eat (BP_Food→IConsumable, `StartEatingItem` przez interfejs, re-cast w grafie). Percepcja (D5) i deprecjacja AItemBase (D3) = osobny późniejszy slice z własnym verify.

## ZALOCKOWANE (dyrektor 2026-06-30) + DEFAULTY domknięte
- **D2 = `UConsumableComponent`** (dyrektor). **D7 = slice minimalny** (dyrektor).
- **D1 (default, assume-log):** `IConsumable` = `BlueprintNativeEvent`: `GetFoodTableRowName()`, `ConsumePortion(float)→float`, `GetRemainingPortion()`. Własność/kradzież (CanBeEatenBy/IsStolenBy, L5) ODŁOŻONA. Depletion = `OnDepleted` (BlueprintAssignable multicast na komponencie; BP_Food binduje destroy/scrap).
- **D4 (default, assume-log):** `StartEatingItem(AItemBase*)` → `StartEatingItem(TScriptInterface<IConsumable>)`. `StartEating(AActor*)` BEZ zmian (BP-callable). Dodano runtime member `EatTargetConsumable` (weak UObject); `ConsumeBite` woła `IConsumable::Execute_ConsumePortion` zamiast `Cast<AItemBase>`. `EatTargetFood` (actor-validity) = owning actor konsumowalnego.
- **D3 ODŁOŻONE:** `AItemBase` nietknięty (zostaje z własnym ConsumePortion; może impl IConsumable w późniejszym slice).
- **D5 ODŁOŻONE:** percepcja `PerceivedFood<AItemBase>` nietknięta (osobny slice / dług TECH-11).

## IMPLEMENTACJA — C++ DONE (compile-validated)
- **Nowe pliki:** `Source/Stan_Pierwotny/IConsumable.h`, `ConsumableComponent.{h,cpp}`.
- **Patch (nie regen):** `MaslowBiologicalComponent.{h,cpp}` — include IConsumable, sygnatura `StartEatingItem`, `EatTargetConsumable`, `ConsumeBite` przez interfejs, `StopEating` zeruje konsumowalny. Usunięto martwy `#include "ItemBase.h"` + forward-decl AItemBase (zero pozostałych referencji).
- **DOWÓD kompilacji:** interim build GAME-target `Game_58 Win64 Development` → **`Result: Succeeded`** (UHT 158 plików gen, ConsumableComponent.cpp + MaslowBiologicalComponent.cpp OK, link Game_58.exe OK; ostrzeżenia tylko istniejące StateTree-deprecation, nie z tej zmiany).

## ⛔ BLOKER ARCHITEKTONICZNY (wykryty w PIE-verify 2026-06-30 — wymaga decyzji)
**Nasz naprawiony BP `BTTask_Eat.uasset` (rich model) NIE jest w żywym drzewie.** Dowód z import-table `BT_Exploration.uasset`: referuje C++ taski z `/Script/Stan_Pierwotny` (`BTTask_Eat`, `BTTask_Drink`, `BTTask_RunBudgetedEQS`, `BTTask_MoveTo`, `BTTask_RollWanderDirection`) — BRAK referencji do assetu `/Game/DocelowaGra/AI_NPC/BTTask_Eat`. Kolizja nazw: żywy `BTTask_Eat` = **klasa C++** (afordancja → `ConsumeFood`, prosty model), nasz BP `BTTask_Eat` (StartEatingItem → ConsumeBite, rich) = **sierota**.
- Konsekwencja: fix IConsumable jest poprawny i kompiluje się (graf BP czysty od 19:03:22), ALE rich-eat nigdy nie wykona się w PIE, bo BT nie odpala BP taska. PIE 2026-06-30: NPC zemdleli z wyczerpania (sen), zero `StartEating`, zero afordancyjnego „ate".
- **DECYZJA (architekt/dyrektor — NIE wybieram):**
  - **A) Wire rich model:** przepiąć gałąź jedzenia w `BT_Exploration` na BP `BTTask_Eat` (rich) zamiast C++ afordancyjnego. Wymaga integracji: jak rich-task dostaje cel (blackboard Key vs afordancja Reserve/Consume), reconcyl z claim/release afordancji. = większy slice / osobny gate.
  - **B) Retire rich model:** afordancyjny C++ eat jest kanoniczny; BP `BTTask_Eat` + `StartEatingItem`/`ConsumeBite` rich-layer = martwy duplikat do deprecjacji (jak [[tech11-perception-model-decision]]). Wtedy IConsumable warstwa nie jest potrzebna do jedzenia (ew. zostaje pod corpse/przyszłość).
- Rekomendacja executora: to decyzja architektury (czy appetite/stomach-fill model jest chcianą mechaniką), nie mechaniczny fix — potrzebny input architekta przed dalszą pracą.

## POZOSTAŁO do zamknięcia bramki (DoD) — ZABLOKOWANE powyższym
1. **Editor-build** nowego C++ (zamknij edytor → build `Game_58Editor`, albo Live Coding) — żeby edytor znał `UConsumableComponent`/`IConsumable`. **AKCJA: zamknięcie edytora po Twojej stronie.**
2. **Manual BP (instruuję, Ty klikasz — MCP koruptuje piny object):**
   a. `BP_Food`: Add Component → `ConsumableComponent`; ustaw `FoodTableRowName` (ten sam co dziś żywi DT_FoodStats); (opcjonalnie) bind `OnDepleted` → Destroy/scrap.
   b. Graf `BTTask_Eat.uasset`: zamień `Cast To BP_Food` → `Get Component by Class (ConsumableComponent)`; podłącz wyjście do pinu **Food** `StartEatingItem` (TScriptInterface<IConsumable> przyjmie komponent automatycznie). `FoodTable` + `BiteCount` zostają. Save.
3. **PIE verify (twarde liczby z `Saved/Logs/`):** głód → find → `StartEating` → `ConsumeBite`×N → `StopEating` → `OnDepleted`, na BP_Food-jako-AffordanceSourceActor. Bramka zamknięta dopiero z tym logiem.

## Definition of Done (FALSYFIKOWALNY — bramka zamknięta dopiero gdy)
1. `BTTask_Eat` kompiluje czysto (build editor-closed → UHT, zero błędów pinów).
2. PIE: pełna pętla jedzenia na BP_Food-jako-AffordanceSourceActor — `głód → find → StartEating → ConsumeBite×N → StopEating → OnDepleted` — z twardymi liczbami z `Saved/Logs/` (wzorzec L1-11b). Runtime value change udowodniony, nie sam fakt że kod istnieje.

## Pending niezwiązane (do uprzątnięcia osobno)
- TECH-07: fix `GetHPPercent` naniesiony w źródle `MaslowBiologicalComponent.cpp` (odwracalny, niezbudowany) — czeka na zamknięcie edytora do builda.

> Następny krok: architekt/dyrektor lockuje D1–D7 → „zatwierdzam" → dopiero wtedy implementacja. Do tego czasu: zero kodu/assetu.
