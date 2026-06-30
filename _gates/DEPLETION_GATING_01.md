# GATE: DEPLETION-GATING-01 — wyczerpywanie jedzenia gatuje dostępność afordancji

Data otwarcia: 2026-07-01 · Status: **✅ DONE — D1=A zalockowane (dyrektor), PIE-VERIFIED** · Target: E:\Game_58 (UE 5.8)
Następca [[BTTASK_EAT_WIRING_01]] (zamknięty, PIE-verified). Dług zgłoszony tam jako „depletion gating".

## Problem (1 zdanie)
NPC re-claimuje i je **w nieskończoność to samo jedzenie** — wyczerpanie nie usuwa go z puli afordancji.

## RECON (żywy kod — file:line, potwierdzone)
**Istnieją DWA niezależne liczniki, niepołączone:**
1. **Afordancja `FAffordanceHandle.RemainingYield`** (`WorldAffordanceSubsystem.h:36`) — `QueryNearest` JUŻ filtruje `RemainingYield>0` (`:98-101`); jest `Consume(Id,Amount,OutGranted)` (`:113`) i regen (`RegenPerHour`, `RegenTick`). To „co świat oferuje".
2. **`ConsumableComponent.RemainingPortion`** (`ConsumableComponent.h:33`) — wyczerpywany przez kęsy (`ConsumePortion_Implementation` `cpp:10-22`: odejmuje, przy 0 pali `OnDepleted.Broadcast()`), nośnik food-data.

**Luka:** rich eat-task (`BTTask_Eat`) je przez `ConsumeBite→IConsumable::ConsumePortion` — **NIGDY nie woła afordancyjnego `Consume`**. Więc `RemainingYield` nie spada, a `QueryNearest` dalej zwraca jedzenie. `OnDepleted` pali się, ale **nic nie jest pod nie podpięte**.

**Auto-cleanup już jest:** `AAffordanceSourceActor::EndPlay` (`AffordanceSourceActor.cpp:28-44`) wyrejestrowuje afordancję; claim auto-zwalnia się przez `ReservedByActor` weak-ptr. Czyli **zniszczenie aktora BP_Food = czyste usunięcie z puli, bez ghostów/crasha.**

**Testowy `BP_Food`:** `Yield=100.0`, `RegenPerHour=0.0` (NIEODNAWIALNY), `AffordanceType=Nourishment`.

## DECYZJE DO LOCKA (NIE wybieram — defining mechanic)
- **D1 — Źródło prawdy dostępności jedzenia:**
  - **A) afordancyjny `RemainingYield`** = SoT: eat dekrementuje Yield w miarę jedzenia (`Consume(Id, …)` per-bite lub raz/meal). `QueryNearest` już filtruje Yield≤0. **Odnawialne-ready** (regen działa). `RemainingPortion` → tylko wizualna „nadgryzioność"/scrap albo do usunięcia.
  - **B) `RemainingPortion`** = SoT aktora: bind `OnDepleted → DestroyActor` → `EndPlay` wyrejestrowuje. **Najprostsze, tylko nieodnawialne**, zgodne z ICONSUMABLE-D1. Afordancyjny Yield=100 staje się vestigial.
  - **C) hybryda** (oba, reconcyl) — odradzam (smell dwóch liczników).
- **D2 — Odnawialne vs nieodnawialne:** apple = destroy-on-zero; krzak jagód / rzeka = deplete-and-regen (yield-based, bez destroy). Model docelowy musi obsłużyć oba → wskazuje na A jako ogólny, z destroy tylko dla `RegenPerHour=0`.
- **D3 — Częściowe odżywianie:** dziś `StartEating` deponuje PEŁNY wiersz FoodTable przez BiteCount, **niezależnie** od pozostałej porcji/yield. Czy na wpół zjedzone jedzenie ma dawać mniej? (nutrition-accounting).
- **D4 — Scrap/ogryzek vs pełne zniszczenie** przy wyczerpaniu (wizual, BP-side).
- **D5 — Gdzie dzieje się consume:** per-bite (ConsumeBite woła afordancyjny Consume) vs przy starcie posiłku (claim całej porcji) vs na końcu.

## REKOMENDACJA EXECUTORA (default, assume-log jeśli nie nadpiszesz)
**D1=A** (afordancyjny `RemainingYield` SoT — to JEST rejestr „co świat oferuje", ma już gating+regen; pasuje do emergent-scarcity rdzenia symulacji). **Minimalny slice:**
1. Eat dekrementuje Yield per-bite (D5=per-bite): w `ConsumeBite` po `ConsumePortion` wołać afordancyjny `Consume(ClaimedId, YieldFrac)` — wymaga, by task/Maslow znał `ClaimedId` (dziś zna go tylko task, nie Maslow → drobny przepływ do zaprojektowania).
2. `QueryNearest` już wyklucza Yield≤0 — zero zmian.
3. **D2:** `RegenPerHour=0` + Yield=0 → `OnDepleted`/BP destroy aktora (wizual + porządek); odnawialne zostają i regenerują.
4. **D3:** ODŁOŻYĆ (osobny gate nutrition-scaling) — na teraz pełny wiersz/meal.

ALE: jeśli chcesz **najmniejszą łatkę pod test apple** bez ruszania C++ → **D1=B**: jeden bind `OnDepleted → DestroyActor` w `BP_Food`. Naprawia infinite-re-eat natychmiast, ale nie służy odnawialnym i zostawia dwa liczniki.

## STOP — nie ruszam do locka
- Zero kodu/assetu do czasu „lock D1–D5" (architekt/dyrektor). To defining mechanic (niedobór = rdzeń emergencji).
- Recon zrobiony; gdy zatwierdzisz kierunek, dopiero implementuję + PIE-verify (infinite-re-eat znika; odnawialne regenerują; cancel-on-death nadal trzyma).

## DoD (po locku, FALSYFIKOWALNY)
1. PIE: NPC zjada `BP_Food` → po N posiłkach/kęsach `QueryNearest` przestaje go zwracać (twardy log: brak kolejnego `[Eat] claimed id=X` na tym samym id; lub `UnregisterAffordance`/destroy log).
2. (jeśli D2 odnawialne w teście) źródło z `RegenPerHour>0` wraca do puli po regenie.
3. Brak ghostów/crasha; cancel-on-death claim-release nadal działa.

## ✅ IMPLEMENTACJA + PIE-VERIFY 2026-07-01 (D1=A zalockowane przez dyrektora)
**Kod (patch, nie regen):**
- `WorldAffordanceSubsystem.{h,cpp}` — nowy `DepleteAffordance(int32 Id)` (Yield→0, idempotentny; renewable regrowth via RegenTick; non-renewable callers mają Destroyować ownera).
- `AffordanceSourceActor.{h,cpp}` — w `BeginPlay` opt-in bind sibling `UConsumableComponent.OnDepleted` → `HandleConsumableDepleted`: `RegenPerHour>0` → `DepleteAffordance` (zostaje, regen refill); else (nieodnawialny) → `Destroy()` (EndPlay wyrejestrowuje + claim auto-free). Coupling tylko AffordanceSourceActor↔ConsumableComponent (oba na BP_Food); zero Maslow↔afordancja.
- Editor-build `Game_58Editor` → Result: Succeeded.

**PIE DoD #1 — SPEŁNIONE (twarde liczby, `Saved/Logs/Game_58.log`, oba NPC):** każde `BP_Food` zjedzone DOKŁADNIE RAZ, potem znika z puli. Sekwencja: `[Eat] claimed id=14 → StopEating(Finished) → [Affordance] Unregister id=14 type=1 → released id=14 → claimed id=8 (INNE id)`. Claimowane id: 1,14,8,2,7,9,4,5 — wszystkie RÓŻNE, każde raz, każde z `Unregister`. Wcześniej (przed slice) NPC re-claimował to samo `id=20` w nieskończoność. Niedobór realnie emerguje (NPC przechodzą przez pulę spawnera). Zero ghostów/crasha, claim-release działa.

## POZOSTAŁE DŁUGI (osobne, NIE blokują)
- **D2 odnawialne (RegenPerHour>0):** `DepleteAffordance` zeruje Yield + regen refill DZIAŁA na yield, ALE `ConsumableComponent.RemainingPortion` NIE jest resetowany przy regenie → po refillu yield jadalne, lecz bites zwrócą Taken=0 (StartEating i tak deponuje pełny wiersz). Reset RemainingPortion na regen = follow-up. NIE testowane w PIE (testowy BP_Food nieodnawialny).
- **D3 partial-nutrition** (odłożone): pełny wiersz/meal niezależnie od pozostałej porcji.
- **D4 scrap/ogryzek** zamiast destroy (wizual) — odłożone.
