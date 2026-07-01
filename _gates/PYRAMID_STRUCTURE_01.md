# PYRAMID_STRUCTURE_01 — szkielet dokumentacji C/L/G (ETAP 1)

**Data:** 2026-07-01 · **Gate:** PIRAMIDA C/L/G — ETAP 1 (szkielet, NIE migracja treści)
**Target:** `E:\Game_58\design\` (nazwy EN, treść PL) · **Branch:** `docs/pyramid-C-L-G` (NIE push)

---

## RESOLVED (wykonane wg gate'u)

1. **Audyt (krok 0):** przeskanowano `.md` w `E:\Game` i `E:\Game_58` (pominięto `Plugins/Intermediate/Binaries/.git/Saved` — vendor-docy nie są „starymi plikami"). 40 docy w Game, 42 w Game_58. Design-docy źródłowe zlokalizowane w **`E:\Game_git\`** (20 plików: DESIGN_how_it_works, *_design, ARCHITECTURE, MILESTONES, PROGRESSION/RPG_CANON, OPERATING_PROTOCOL…).
2. **Drzewo (krok 1):** utworzone `design/{C_world,L_npc,G_roadmap,_archive}`.
3. **README osi (krok 2):** `C_README` (świat/substrat, import tylko niższe, spis C0-4) · `L_README` (Maslow, strzałka-w-dół, single-bridge `BTService_MaslowBlackboardSync`, spis L0-5) · `G_README` (kompozycja C+L+gracz, falsyfikowalne DoD, spis G0-2).
4. **Szkielety warstw (krok 3):** 5×C + 10×L — każdy: nagłówek + status + „Źródło treści" + 1 zdanie zakresu + reguła importu/strzałki. Statusy wg gate'u (C0✅ C1✅ C2🔨 C3✅ C4🔨 · L0🔴 L1_*✅ L2🔨 L3/4/5⬜).
5. **G-pliki (krok 4):** `G0_nothing` (punkt zero) · **`G1_animal_world` — treść DOKŁADNIE z BLOKU B dyrektora (verbatim)** · `G2_hunting_tools` (stub ⬜).
6. **Archiwizacja (krok 5):** 38 docy stanu Game_58 (`Gra_Stan_Pierwotny/*.md` + `REPORTS/*`) skopiowane do `design/_archive/state/` — **oryginały nietknięte**.
7. **Mapa migracji (krok 6):** `_gates/MIGRATION_MAP.md` — 16 mapowań stary→nowy, status **pending**, zasada „aspekt przez interfejs" (temp: C3 świat vs L1 odczuwanie).
8. **BRAMKI (STOP) uszanowane:** nie usunięto/nie przeniesiono oryginałów; **nie pushowano**; **nie rozbito** grubych plików (ETAP 2, osobna sesja).

---

## OPEN (do decyzji architekta)

1. **Nazwy L0/L2-L5** — architekt jawnie nazwał C0-4, L1_*, G0-2, ale NIE L0/L2-5. Przyjąłem sufiksy EN wywiedzione z Maslow-poziomów ROADMAP: `L0_fight_or_flight`, `L2_safety`, `L3_belonging`, `L4_esteem`, `L5_actualization`. **Potwierdź lub przemianuj.**
2. **`NPC_DEEPENING_concepts.md` (migracja #7)** — **NIE znaleziono** w żadnym repo (Game/Game_58/Game_git). Oznaczone „źródło TBD". Wskaż plik albo potwierdź, że treść dopiero powstanie.
3. **Zakres archiwum** — zarchiwizowałem tylko audytowane docy stanu Game_58 (38), **NIE** design-docy z `E:\Game_git` (bezpieczne w repo comms; MIGRATION_MAP wskazuje je in-place). Jeśli chcesz snapshot Game_git w `_archive/design_gates/` — jedno słowo.
4. **`ARCHITECTURE.md`/`MILESTONES.md` → `design/` top (migracja #13)** — NIE utworzone/kopiowane w ETAP 1 (żyją w Game_git; przeniesienie = ETAP 2). Potwierdź kierunek.
5. **Rozjazd L2** — Maslow-standard L2 = bezpieczeństwo (ROADMAP POZIOM 2), a migracja #7 kieruje tu „człowieczeństwo/maski" z NPC_DEEPENING. Nazwa pliku = `L2_safety` (wg ROADMAP); zakres wspomina oba. Do ujednolicenia w ETAP 2.

---

## DRZEWO utworzone

```
E:\Game_58\design\
├── C_world\   C_README, C0_clocks✅, C1_geography✅, C2_water🔨, C3_climate✅, C4_resources🔨
├── L_npc\     L_README, L0_fight_or_flight🔴, L1_hunger✅, L1_thirst✅, L1_temperature✅,
│              L1_sleep✅, L1_body_senses✅, L2_safety🔨, L3_belonging⬜, L4_esteem⬜, L5_actualization⬜
├── G_roadmap\ G_README, G0_nothing✅, G1_animal_world⬜(w budowie), G2_hunting_tools⬜(stub)
└── _archive\state\  (38 docy stanu Game_58 — snapshot, oryginały w miejscu)

E:\Game_58\_gates\MIGRATION_MAP.md         (16 mapowań, pending)
E:\Game_58\_gates\PYRAMID_STRUCTURE_01.md  (ten raport)
```

## LISTA utworzonego
- **21 szkieletów** w `design/` (3 README + 5 C + 11 L + 3+1 G... ) → dokładnie: C(6) + L(11) + G(4) = 21 plików `.md`.
- **38 plików** archiwum w `design/_archive/state/`.
- **2 pliki** w `_gates/`: MIGRATION_MAP.md + PYRAMID_STRUCTURE_01.md.
- Commit: `docs: C/L/G pyramid skeleton + G1` na branchu `docs/pyramid-C-L-G` (bez push).
