# C0 — Zegary (czas świata)

Oś: C (świat) · Warstwa: C0 · Status: ✅ (zegar istnieje, do postawienia na mapie)

**Źródło treści:** `DESIGN_how_it_works.md` CZ.II + `Gra_Stan_Pierwotny/MECHANICS.md` (zegar doby) + `REPORTS/raport_2026-06-19_warstwa_doby.md`

**Zakres (1 zdanie):** Deterministyczny zegar doby/pory (`BP_DayNightCycle`, 2.4 min/doba) jako jedno źródło czasu dla całego świata i „jednego zegara" snu.

**Import:** C0 nie importuje niczego (najniższa warstwa czasu). Reszta świata i NPC czytają czas, nigdy odwrotnie.

> SZKIELET (ETAP 1). Treść do zmigrowania w ETAP 2 wg `_gates/MIGRATION_MAP.md`.
