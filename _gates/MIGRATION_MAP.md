# MIGRATION_MAP — stary dokument → nowa piramida C/L/G

> ETAP 1 (ten gate) = tylko szkielet + ta mapa. **ETAP 2** = faktyczne rozbicie/migracja treści,
> per-warstwa, osobna sesja (NIE automatycznie — gruby plik dzieli architekt). Status startowy: **pending**.
>
> Źródła design-doców: **`E:\Game_git\`** (repo comms/design — oryginały ZOSTAJĄ na miejscu).
> Źródła stanu/raportów: **`E:\Game_58\Gra_Stan_Pierwotny\`** (snapshot w `design/_archive/state/`).

## Mapowania (wg architekta + audyt)

| # | Stary (źródło) | Nowy (cel) | Zakres migracji | Status |
|---|---|---|---|---|
| 1 | `DESIGN_how_it_works.md` **CZ.I** | `L_npc/L*` | rozbić na warstwy L0-L5 (ETAP 2) | **partial** ✅ (Hydration→L1_thirst, Body→L1_body_senses 2026-07-01; reszta CZ.I pending) |
| 2 | `DESIGN_how_it_works.md` **CZ.II** | `C_world/C*` | rozbić na warstwy C0-C4 (ETAP 2) | pending |
| 3 | `HUNGER_design.md` | `L_npc/L1_hunger.md` | całość | **done** ✅ (ETAP2 2026-07-01) |
| 4 | `APPETITE_GRUBAS_design.md` | `L_npc/L1_hunger.md` | scalić z HUNGER (grubas/appetite) | **done** ✅ (ETAP2 2026-07-01) |
| 5 | `SLEEP_ENGINE_design.md` + `SLEEP_ENGINE_ETAP2_design.md` | `L_npc/L1_sleep.md` | oba etapy | **done** ✅ (ETAP2 2026-07-01) |
| 6 | `AMBIENT_TEMP_design.md` | `C_world/C3_climate.md` (świat) **+** `L_npc/L1_temperature.md` (odczuwanie) | **aspekt przez interfejs** — świat vs odczuwanie | **done** ✅ (ETAP2 2026-07-01, C3+L1_temperature bez duplikacji) |
| 7 | `NPC_DEEPENING_concepts.md` ⚠️ źródło TBD (nie znaleziono w Game/Game_58/Game_git) | `L_npc/L2_safety.md` | człowieczeństwo/maski (rozproszenie, ETAP 2) | pending |
| 8 | `OCEAN_L3-02_design.md` | `L_npc/L0_fight_or_flight.md` (panika) + `L_npc/L3_belonging.md` (osobowość-filtr) | podział wg aspektu | pending |
| 9 | `MASLOW_BT_BRIDGE_design.md` (+`_disarm`, +`_plaster2_hunger_gate`) | `L_npc/L_README.md` (single-bridge) + `L1_hunger` | zasada jednego mostu | pending |
| 10 | `NPCREGISTRY_design.md` (+`_design2`) | `L_npc/L3_belonging.md` | keystone socjalny | pending |
| 11 | `TECH-11_PERCEPTION_design.md` | `L_npc/L1_body_senses.md` + `L0_fight_or_flight.md` | percepcja jako jedna prawda | **partial** ✅ (→L1_body_senses done 2026-07-01; L0 pending) |
| 12 | `PROGRESSION_DESIGN.md`, `RPG_DESIGN_CANON.md` | `L_npc/L4_esteem.md`, `L5_actualization.md`, `G_roadmap/G2` | progresja/canon RPG | pending |
| 13 | `ARCHITECTURE.md`, `MILESTONES.md` | `design/` (poziom top) | przenieść na szczyt design (ETAP 2) | pending |
| 14 | red-team skill / czerwony zespół | `_redteam/` | skill audytu | pending |
| 15 | `Gra_Stan_Pierwotny/MECHANICS.md` | rozdzielić: strefy→`C1`, woda→`C2`, temp→`C3`+`L1_temperature`, potrzeby→`L1_*` | dane liczbowe mechanik | pending |
| 16 | `Gra_Stan_Pierwotny/ROADMAP.md` | `G_roadmap/G*` (grywalne stany) + statusy warstw | roadmapa Maslow → oś G | pending |

## Zasady migracji (ETAP 2)
- **Aspekt przez interfejs** (#6): temperatura ma stronę świata (C3, `AmbientTemp`) i stronę odczuwania
  (L1_temperature, sprzężenie ciała) — migrować ROZDZIELNIE, połączone tylko interfejsem odczytu.
- **Nie rozbijać grubych plików automatycznie** — podział per-warstwa robi architekt (osobna sesja/gate).
- Oryginały **NIE** kasowane/przenoszone; migracja = kopia treści + oznaczenie źródła w warstwie.
