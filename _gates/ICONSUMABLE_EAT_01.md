# GATE: ICONSUMABLE-EAT-01 — odblokowanie BTTask_Eat przez interfejs IConsumable

Data otwarcia: 2026-06-30 · Status: **C++ DONE (compile-validated) — czeka na Editor-build + kroki BP + PIE** · Target: E:\Game_58 (UE 5.8)
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

## POZOSTAŁO do zamknięcia bramki (DoD)
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
