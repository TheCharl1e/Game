# TICKET: SLEEP-FAINT-TRAP-01 — NPC mdleje z wyczerpania zanim głód dostanie turę

Data: 2026-06-30 · Status: OPEN (osobny od BTTASK_EAT_WIRING_01 — NIE mieszać) · Target: E:\Game_58

## Problem (z PIE-verify 2026-06-30, log)
W PIE pętla jedzenia nie odpaliła się, bo:
- **Rest(prio 3) > Nutrition(prio 4)** w drabinie need ORAZ
- **Handle Sleep jest pusta** (plaster #3 — patrz [[bp-need-brain-zombie]] / sleep engine).

→ NPC wspina się w faint-trap: `[Sleep] zmęczenie → OMDLENIE (HoursAwake=25.00) — BT zatrzymany, ragdoll` (`Game_58.log` 19:06:14) ZANIM głód dostanie turę. Efekt: DoD jedzenia jest **nietestowalne** niezależnie od okablowania.

## Opcje (do decyzji)
- **A) Seed PIE „rested+hungry, krótka sesja":** wstaw NPC z HoursAwake≈0 + niski Glucose/zero GlycogenReserves+StomachFill, dilation jak [[pie-single-need-bt-branch-test]]. Najszybsze do verify, zero zmian gameplay.
- **B) Szczątkowe Handle Sleep:** minimalna gałąź snu w BT, by NPC realnie spał i resetował HoursAwake zamiast mdleć — domyka plaster #3.

## Dlaczego osobno
To dług sleep-engine/BT, nie część kontraktu eat-wiring. Blokuje TYLKO test PIE jedzenia, nie implementację C++ taska. Domknięcie BTTASK_EAT_WIRING_01 (DoD #2 PIE) zależy od tego ticketu.
