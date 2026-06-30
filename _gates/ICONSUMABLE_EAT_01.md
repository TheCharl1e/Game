# GATE: ICONSUMABLE-EAT-01 — odblokowanie BTTask_Eat przez interfejs IConsumable

Data otwarcia: 2026-06-30 · Status: **RECON-STOP** (czeka na lock D1–D7 + „zatwierdzam") · Target: E:\Game_58 (UE 5.8)
Pełna diagnoza i powierzchnia: `Gra_Stan_Pierwotny/REPORTS/gate_request_2026-06-30_iconsumable_bp_food.md` (źródło prawdy, nie duplikuję).

## Problem (1 zdanie)
W 5.8 `BP_Food` jest reparentowany na `AAffordanceSourceActor` (żywy forage L0-TA-S1), a warstwa jedzenia zakłada `AItemBase` — `BTTask_Eat` nie kompiluje (pin `Food: AItemBase*` ≠ `Cast To BP_Food`), więc pętla jedzenia jest zerwana.

## RESOLVED (zalockowane)
- **Kierunek = `IConsumable`** (decyzja dyrektora). NIE merge baz (rejestracja afordancji jest bezwarunkowa — `AffordanceSourceActor.cpp:14-26`), NIE robocopy 5.7→5.8 (cofnęłoby afordancje + 4 nowe pliki). BP_Food zostaje `AffordanceSourceActor` i DODATKOWO niesie jadalność.
- Diagnoza dowiedziona z logu (`Game_58.log` 12:13:53) + read-only audyt call-site'ów AItemBase (Maslow:1055/1126, NPCPerceptionComponent, graf BTTask_Eat).

## DEFAULTS (rekomendacja executora — przyjmę, jeśli nie nadpiszesz)
- **D2 = `UConsumableComponent`** (nośnik food-data: `FoodTableRowName`/`RemainingPortion`/`ConsumePortion`, wpinany na BP_Food obok afordancji; data-driven, „C++ mózg / BP ciało", reużywalny dla corpse).
- **D7 = slice minimalny NAJPIERW**: odblokować tylko BTTask_Eat (BP_Food→IConsumable, `StartEatingItem` przez interfejs, re-cast w grafie). Percepcja (D5) i deprecjacja AItemBase (D3) = osobny późniejszy slice z własnym verify.

## OPEN (wymaga locka — NIE wybieram sam)
- **D1** — kształt API `IConsumable` (minimalny: `GetFoodRow`/`ConsumePortion`/`GetRemainingPortion`/`CanBeEatenBy`/`OnDepleted` vs pełny z własnością `OwnerID`/`IsStolenBy`).
- **D2** — nośnik: interfejs-na-BP (`BlueprintNativeEvent`) vs `UConsumableComponent` (rekomendacja: komponent).
- **D3** — los `AItemBase` (zostaje jako impl `IConsumable` vs deprecjacja). Gdzie ostatecznie żyje `FoodTableRowName`/`RemainingPortion`.
- **D4** — sygnatury C++: `StartEatingItem(AItemBase*)` → `TScriptInterface<IConsumable>` lub `(AActor* + Implements<>)`; + typ `EatTargetFood`.
- **D5** — percepcja: `PerceivedFood<AItemBase>` → IConsumable vs porzucić na rzecz afordancji+EQS (dług TECH-11).
- **D6** — graf `BTTask_Eat`: `Cast To BP_Food` → interfejs (manual editor save, ryzyko RF_Transient — patrz „no Monolith save for BP defaults").
- **D7** — zakres slice'a (minimalny vs pełny).

## Definition of Done (FALSYFIKOWALNY — bramka zamknięta dopiero gdy)
1. `BTTask_Eat` kompiluje czysto (build editor-closed → UHT, zero błędów pinów).
2. PIE: pełna pętla jedzenia na BP_Food-jako-AffordanceSourceActor — `głód → find → StartEating → ConsumeBite×N → StopEating → OnDepleted` — z twardymi liczbami z `Saved/Logs/` (wzorzec L1-11b). Runtime value change udowodniony, nie sam fakt że kod istnieje.

## Pending niezwiązane (do uprzątnięcia osobno)
- TECH-07: fix `GetHPPercent` naniesiony w źródle `MaslowBiologicalComponent.cpp` (odwracalny, niezbudowany) — czeka na zamknięcie edytora do builda.

> Następny krok: architekt/dyrektor lockuje D1–D7 → „zatwierdzam" → dopiero wtedy implementacja. Do tego czasu: zero kodu/assetu.
