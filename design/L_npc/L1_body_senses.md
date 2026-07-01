# L1 — Ciało i zmysły (Body / Senses)

Oś: L (NPC) · Warstwa: L1 · Status: ✅

**Źródło treści:** `DESIGN_how_it_works.md` CZ.I + `Gra_Stan_Pierwotny/CODE_REGISTRY.md` (Body) + `TECH-11_PERCEPTION_design.md`

**Zakres (1 zdanie):** Ciało 26-częściowe (hierarchia, kaskada uszkodzeń) + zmysły pochodne (wzrok/słuch/precyzja dłoni/mobilność) liczone i cache'owane, recompute tylko na uszkodzeniu.

**Strzałka-w-dół:** L1 zna tylko L0. Zmysły gatują percepcję (VisionAcuity→zasięg); dane ciała czytane O(1), bez per-frame.

> SZKIELET (ETAP 1). Treść do zmigrowania w ETAP 2 wg `_gates/MIGRATION_MAP.md`.
