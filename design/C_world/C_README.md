# Oś C — Świat / Substrat

Oś **C** to świat jako **deterministyczny substrat**: geografia, woda, klimat, czas, zasoby.
Warstwy C **NIE wiedzą o NPC** — nie znają potrzeb, decyzji ani ciała. Świat po prostu *jest*,
a NPC (oś L) go **odczytuje** przez interfejsy (np. `GetZoneAtLocation`, `AmbientTemp`, afordancje).

**Zasada importu:** warstwa importuje **tylko warstwy niższe** (C(n) zna wyłącznie C(<n)).
Brak zależności w górę, brak zależności od osi L. To gwarantuje, że świat jest testowalny w izolacji.

## Spis warstw
| ID | Warstwa | Status |
|---|---|---|
| C0 | `C0_clocks` — zegar doby/pory (źródło czasu) | ✅ |
| C1 | `C1_geography` — koncentryczne strefy Caldreth (18) | ✅ |
| C2 | `C2_water` — woda stała (rzeki/wybrzeże/oaza) | 🔨 |
| C3 | `C3_climate` — temperatura strefowa + offset doby | ✅ |
| C4 | `C4_resources` — zasoby jako afordancje (jedzenie/woda/schronienie) | 🔨 |

> Legenda: ✅ stoi · 🔨 w toku · 🔴 model gotowy, kod nie · ⬜ TODO.
> Treść warstw migrowana w ETAP 2 (per-warstwa, osobna sesja) wg `_gates/MIGRATION_MAP.md`.
