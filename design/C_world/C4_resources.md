# C4 — Zasoby (afordancje)

Oś: C (świat) · Warstwa: C4 · Status: 🔨

**Źródło treści:** `docs/L0_TrackA_Slice1_BuildBrief.md` (afordancje) + `DESIGN_how_it_works.md` CZ.II (FOOD/spawn) + `WorldAffordanceSubsystem`

**Zakres (1 zdanie):** Zasoby świata jako **afordancje** (jedzenie/woda/schronienie) w rejestrze z spatial-hashem + regenem na timerze + atomowym claimem — nie aktor per jagoda.

**Import:** C4 importuje C0-C3 (czas dla regenu, geografia/woda/klimat dla rozmieszczenia). Konsumpcja przez NPC = oś L (EQS→navigate→consume), świat tylko wystawia i regeneruje.

> SZKIELET (ETAP 1). Treść do zmigrowania w ETAP 2 wg `_gates/MIGRATION_MAP.md`.
