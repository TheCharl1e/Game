# L1 — Temperatura (odczuwanie)

Oś: L (NPC) · Warstwa: L1 · Status: ✅

**Źródło treści:** `AMBIENT_TEMP_design.md` (odczuwanie, aspekt) + `Gra_Stan_Pierwotny/MECHANICS.md` (temp ciała)

**Zakres (1 zdanie):** Odczuwanie temperatury — sprzężenie ciało↔otoczenie (czyta `AmbientTemp` z C3 **przez interfejs**), hipotermia stopniowa, cold-burn, izolacja (fat + clothing + fire).

**Strzałka-w-dół:** L1 zna tylko L0. Klimat (C3) jest wejściem świata przez interfejs — L1 nie modyfikuje świata; potrzeba do BB tylko przez `BTService_MaslowBlackboardSync`.

> SZKIELET (ETAP 1). Treść do zmigrowania w ETAP 2 wg `_gates/MIGRATION_MAP.md`.
