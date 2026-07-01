# Oś G — Roadmapa grywalna

Oś **G** to **kolejne grywalne stany** projektu. G(n) **NIE opisuje systemu** (od tego są osie C i L) —
G(n) **komponuje** istniejące warstwy C + L + rolę gracza w **stan, który da się odpalić i zmierzyć**.

Każdy G to integracja, nie greenfield: bierze gotowe klocki (C0-C4, L0-L5) i skleja je w żywą pętlę.
Jeśli klocek nie istnieje, G go **wskazuje** jako brakujące zadanie — ale sam pozostaje kompozycją.

**Falsyfikowalne DoD:** każdy G ma **killer-demo** — konkretny, jednoczesny, log-backed dowód, że
stan działa bez skryptu. Nie „system istnieje", lecz „w logu jednocześnie widać X, Y, Z przy skali N".
Dowód zawsze z twardych liczb (UE_LOG / live-object), nigdy z założenia.

## Spis stanów
| ID | Stan | Status |
|---|---|---|
| G0 | `G0_nothing` — pusty Game_58 (silnik+MCP, zero gameplayu) — punkt zero pomiaru | ✅ |
| G1 | `G1_animal_world` — żywy świat na poziomie zwierzęcym (C0-3 + L0-1 + gracz-obserwator) | ⬜ w budowie |
| G2 | `G2_hunting_tools` — polowanie + pierwszy tech-plaster (ostry kamień) + mięso/padlina | ⬜ |

> Legenda: ✅ osiągnięty · 🔨 w toku · ⬜ TODO.
