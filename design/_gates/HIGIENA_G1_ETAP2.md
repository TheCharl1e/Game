# HIGIENA + G1 PRECYZJA — ETAP 2 (segregacja + split G1a/G1b)

**Data:** 2026-07-01 · **Branch:** `docs/pyramid-C-L-G` · **Target:** `E:\Game_58\design\` · Docs-only (zero kodu/sceny).

---

## RESOLVED (rozstrzygnięcia OPEN z PYRAMID_STRUCTURE_01 — zablokowane)
1. **Nazwy L** — ZATWIERDZONE: `L0_fight_or_flight`, `L1_*`, `L2_safety`, `L3_belonging`, `L4_esteem`, `L5_actualization`.
2. **`NPC_DEEPENING_concepts.md`** — NIE istnieje → źródło TBD, treść powstanie później. Hook zostawiony w `L2_safety` (podrozdział L2.2).
3. **Archiwum Game_git** — NIE robimy snapshotu (oryginały żyją w repo comms; MIGRATION_MAP wskazuje in-place). OK.
4. **`ARCHITECTURE`/`MILESTONES`** — ZOSTAJĄ w Game_git (nie przenosimy do `design/` top). Link, nie kopia.
5. **Rozjazd L2** — ujednolicono: **L2 = „bezpieczeństwo + stabilność psychiczna"**; człowieczeństwo/maski = **podrozdział L2.2**, nie osobna warstwa. Nagłówek `L2_safety.md` przepisany.

## CO ZMIGROWANO (ETAP 2 — TYLKO warstwy ✅, zero interpretacji)
| Warstwa | Źródło (Game_git) | MIGRATION_MAP |
|---|---|---|
| `L1_hunger` | HUNGER_design.md + APPETITE_GRUBAS_design.md | #3,#4 → **done** |
| `L1_thirst` | DESIGN_how_it_works CZ.I (Hydration — źródło skąpe, zaznaczone) | #1 → **partial** |
| `L1_temperature` | AMBIENT_TEMP_design.md (aspekt: odczuwanie/ciało) | #6 → **done** |
| `C3_climate` | AMBIENT_TEMP_design.md (aspekt: świat) | #6 → **done** |
| `L1_sleep` | SLEEP_ENGINE_design.md + ETAP2 | #5 → **done** |
| `L1_body_senses` | DESIGN_how_it_works CZ.I (Body) + TECH-11_PERCEPTION_design.md | #1,#11 → **partial/done** |

**Zasada aspektu przez interfejs** dochowana: temperatura = `C3_climate` (świat: BaseTemp/strefa/doba) + `L1_temperature` (odczuwanie: sprzężenie ciała, hipotermia, izolacja), granica = `GetFeltTemperature`. **Zero duplikacji** — BaseTemp/strefy TYLKO w C3, sprzężenie ciała TYLKO w L1_temperature, wzajemne linki `[[...]]`. Analogicznie fat→izolacja liczony w `L1_hunger` (jeden writer), `L1_temperature` linkuje.

## NIE RUSZONE (szkielet, treść = ETAP 3)
`L0_fight_or_flight` (🔴, model żyje w G1, kod nie), `L2_safety` (🔨, tylko nagłówek ujednolicony), `L3_belonging`/`L4_esteem`/`L5_actualization` (⬜), `C0_clocks`/`C1_geography`/`C2_water`/`C4_resources`. `DESIGN_how_it_works` CZ.I/CZ.II **NIE rozbijane auto** — per-warstwa PÓŹNIEJ (architekt dzieli gruby plik).

## G1a / G1b — DIFF (rozbicie G1)
`G1_animal_world.md` przepisany: jeden plik, dwa pod-stany + wciągnięty blocker terenu z K1.

| | **G1a — ORIGIN SANDBOX** | **G1b — ZALUDNIONA WYSPA** |
|---|---|---|
| Teren | podłoga 8000×8000 @ origin (istnieje) | teren+navmesh pod 18 strefami = **osobny MAP-GATE** |
| Strefa | JEDNA (GetZoneAtLocation origin) | 18 stref, różnicowanie |
| AmbientTemp | jednostrefowa (dług jawny) | reaguje na strefę (Ocean 8° vs AshSlope 28° vs Lava 80°) |
| DoD tutaj | pije/je/śpi/panika-flee/śmierć-cleanup + skala O(P·local) 50/200/500 | AmbientTemp-per-strefa, migracja do wody, popiół jako ucieczka od zimna, „blisko lawy=blisko śmierci" |
| K1-K6 | ✅ tutaj (K1 DONE) | po MAP-GATE terenu |

## ZNANE DŁUGI (dopisane do G1)
- **(a)** strefy bez terenu → różnicowanie = G1b (MAP-GATE).
- **(b)** stan sceny Game_58 NIE w gitcie (Content gitignored) → backup ręczny (Ctrl+S).
- **(c)** HoursAwake sync → rozrzut startowy (per-instancja offset).
- + długi K1 d1-d7 (`G1_K1_SCENE.md`).

## OPEN (do dyrektora)
1. **Nagrywanie testów** — ffmpeg NIE zainstalowany; decyzja: (a) instal ffmpeg → mp4 60s na Pulpicie, czy (b) stykówka ze zrzutów viewportu. (Poza tym gate'em.)
2. **ETAP 3** — kolejność rozbijania `DESIGN_how_it_works` CZ.I/CZ.II per-warstwa (architekt dzieli).
3. **G1b MAP-GATE** — kiedy startujemy teren+navmesh pod strefami (blokuje różnicowanie strefowe).

## Batch-report
- Zmigrowano **6 warstw ✅** (5 subagentów-destylatorów, treść wierna, encje odkodowane, aspekt-split C3/L1_temperature bez duplikacji).
- MIGRATION_MAP: 4× **done**, 2× **partial** (#1, #11).
- L2 nagłówek ujednolicony; G1 → G1a/G1b + ZNANE DŁUGI.
- BRAMKI dochowane: migrowano TYLKO ✅; `DESIGN_how_it_works` NIE rozbijany auto; kod/scena nietknięte; **NIE push**.
- Commit: `docs(etap2): migrate DONE layers + split G1a/G1b + resolve OPEN` na `docs/pyramid-C-L-G`.
