# L0 — Stres (Fight or Flight)

Oś: L (NPC) · Warstwa: L0 · Status: 🔴 (model gotowy, kod nie)

**Źródło treści:** `DESIGN_how_it_works.md` CZ.I (L0) + `OCEAN_L3-02_design.md` (panika, Neurotyczność) + `G1_animal_world.md` (model S1/S2 do zbudowania)

**Zakres (1 zdanie):** Najniższy priorytet Maslowa — percepcja zagrożenia → panika/flee/fight; latch `bIsInPanic` policzony poprawnie (OCEAN), ale most percepcja→arbitraż→akcja (S1/S2) do zbudowania.

**Strzałka-w-dół:** L0 nie zna żadnej warstwy L wyższej. Czyta świat (zagrożenia) przez percepcję; pisze do BB wyłącznie przez `BTService_MaslowBlackboardSync`.

> SZKIELET (ETAP 1). Treść do zmigrowania w ETAP 2 wg `_gates/MIGRATION_MAP.md`.
