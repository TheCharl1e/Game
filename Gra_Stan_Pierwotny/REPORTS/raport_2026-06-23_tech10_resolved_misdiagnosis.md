# Raport — TECH-10 ROZSTRZYGNIĘTE: misdiagnoza (reinstancing, nie BP-writer) · 2026-06-23

> Recon pod plaster #5 obalił premisę plastra #5 ORAZ samego TECH-10. STOP-AND-FLAG: nie buduję fixu
> na fałszywej diagnozie. Plaster #5 ANULOWANY (brak celu). Branch `docs/tech10-resolved`.

## Premisa (TECH-10 / plaster #5) — była:
*„BP-drugi-mózg re-aplikuje BP-defaulty na pola C++ Maslow co kadencję (BodyFat→1500, GlycogenReserves→1000,
CollapseThreshold→24, CurrentStamina→100). One-writer violation → plaster #5 = kasacja BP need-calc."*

## Dowód obalający (twardy, statyczny + live)
1. **BP_NPC_Character ma 0 referencji do komponentu C++ Maslow:** `search_nodes "Maslow"` = **0**, `"BodyFat"` = **0**,
   `"Stamina"` = **0**. Graf, który nie referuje komponentu, **NIE MOŻE go zapisać**. BP „drugi mózg"
   (`MetabolismStats` ST_Metabolism + `EvaluateNeeds`/`IncreaseHunger`) to **osobny, rozłączony struct** — nie dotyka C++.
2. **Pola reset-owe pisane WYŁĄCZNIE przez C++** (grep): konstruktor (Glycogen=1000, Stamina=100), BeginPlay
   (BodyFat=StartingBodyFat), ProcessMetabolism (drain/regen/burn — DECREASE, nigdy „reset w górę"). Nikt inny.
3. **DECYZYJNE (PIE):** `get_component_by_class(MaslowBiologicalComponent)` zwraca **`TRASH_MaslowBiologicalComponent_0`**
   (martwa reinstancowana kopia); ŻYWY komponent = **`MaslowBiological`** z realną wartością BeginPlay (BodyFat=1500).
   **Moje injekcje przez całą sesję (TASK 1/2, L1-08) trafiały w TRASH_ ghosta → żywa instancja się nie zmieniała →
   wyglądało jak „reset do defaultów per kadencja".**

## WERDYKT
**TECH-10 NIE jest realnym bugiem — to artefakt test-harnessu (reinstancing / TRASH_ instance-mismatch).**
Nie ma BP-writera ani resetu per-kadencja na żywej instancji. To dokładnie [[trash-reinstancing-fresh-editor-for-pie-verify]].
**Plaster #5 (kasacja BP-drugiego-mózgu „by przestał nadpisywać C++") ANULOWANY — celował w writera, który nie istnieje.**

## Konsekwencje
- **L1-08 gradient prędkości** (i każdy inny „efekt staminy/progów") był nieweryfikowalny live z TEGO powodu —
  nie reset, lecz injekcja-w-ghost. Dowód przez kompozycję (L1-08) był poprawnym obejściem.
- **Realny problem = TOOLING:** `get_component_by_class` (singular) bywa TRASH_. Technika verify: rozwiąż
  NIE-`TRASH_` komponent; przy ciężkim churnie restart edytora / weryfikuj przez kompozycję. (Pamięć istnieje.)
- **Higiena (osobno, opcjonalnie):** martwy BP-drugi-mózg (`EvaluateNeeds`/`IncreaseHunger`/`MetabolismStats`,
  0 ref do Maslow) można skasować jako dead-code — ale to NIE jest fix TECH-10, tylko sprzątanie.

## STATUS
TECH-10 → **ZAMKNIĘTE jako misdiagnoza/test-harness artefakt** (re-klasyfikacja, nie bug). Plaster #5 anulowany.
Pamięć `maslow-bp-resets-cpp-fields-per-cadence` → oznaczona DISPROVEN. Zero zmian w kodzie/asset.
