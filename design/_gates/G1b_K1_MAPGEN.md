# GATE: G1b-K1 — MapGen run + import verify + diagnoza terenu
> Wykonawca: Claude Code · 2026-07-01 · Repo: E:\Game_58 · Branch: docs/pyramid-C-L-G
> Status: **AUDYT (0) DONE · RUN (1) ZABLOKOWANY na env → STOP+pytanie o pip · DIAGNOZA-TEREN (3) DONE z kodu**
> Zero aktorów postawionych, zero PIE, zero pip. Cel: potwierdzić pipeline danych + zdiagnozować lukę elevation→teren.

## 0. AUDYT (raport, nic nie budowane)

### 0a. Python env
| Składnik | Stan |
|---|---|
| Interpreter | Python **3.14.5** (python/python3/py — wszystkie 3.14.5) |
| numpy | ✅ OK |
| **scipy** | ❌ **MISSING** — a `caldreth_mapgen.py:214` woła `gaussian_filter` **bezwarunkowo** na ścieżce `generate()` → generacja padnie bez scipy |
| **matplotlib** | ❌ **MISSING** — `caldreth_render.py` (map.png) importuje matplotlib+scipy na topie → `build_all` pada już przy `from caldreth_render import render` |
| **Pillow (PIL)** | ❌ **MISSING** — `export()` pisze `caldreth_biome.png` tylko `if PIL` (try/except pass). **biome.png to JEDYNE wejście importera C++** → bez Pillow pipeline jest bezużyteczny dla UE |

**Wniosek env:** RUN niemożliwy bez instalacji. numpy sam nie wystarcza (scipy na ścieżce generate). → **STOP, sekcja 1.**

### 0b. Tools\MapGen\ + build_all.py
4 pliki potwierdzone: `caldreth_mapgen.py` (18.6 KB, generator, numpy+scipy), `caldreth_render.py` (5.4 KB, matplotlib+scipy), `build_all.py` (4.0 KB, orkiestrator), `zone_defs.json` (12 biomów).
**build_all.py produkuje → `<ProjectRoot>/MapData/`** (`E:\Game_58\MapData\`, jeszcze NIE istnieje):
| Artefakt | Zawartość | Konsument | Gitignore |
|---|---|---|---|
| `caldreth_manifest.json` | island+grid_size+sea_level+wind+biome_legend(12)+**points_of_interest**(x/y 0..1) | `ImportCaldrethPOIs` | tracked (input) |
| `caldreth_biome.png` | indeksowana maska 8-bit, 1 px = 1 komórka, kanał = biome id (0..11) | **`ImportCaldrethZones`** | tracked (input) |
| `caldreth_data.npz` | **elevation**(f32) + rainfall(f32) + biome(i16), 512×512 | **NIKT** (patrz 0c) | ignored (regen) |
| `caldreth_map.png` | kanon wizualny (biomy+wiatr+opady) | człowiek | ignored (regen) |

### 0c. Strona C++ importu (5.8) — TWARDA ODPOWIEDŹ
Przeczytane: `CaldrethImportLibrary.{h,cpp}` (`ImportCaldrethZones` + `ImportCaldrethPOIs`), `CaldrethZone.h`, `CaldrethPOIMarker.h`.

**(a) Co import robi:** stawia **strefy jako DANE/markery, NIE buduje terenu.**
- `ImportCaldrethZones`: dekoduje `biome.png` → flood-fill (BFS 4/8-conn) na spójne regiony jednego biomu → dla każdego 1× `ACaldrethZone` w centroidzie, `Location = ((CX-0.5)*WorldSizeUU, (CY-0.5)*WorldSizeUU, **0.f**)`. Ustawia `ZoneType`, `ZoneTable`, `WorldSizeUU`, `NormalizedOutline` (2D bbox 4 rogi). Log: „placed N zones … WorldSize=… ”.
- `ImportCaldrethPOIs`: parsuje manifest → 1× `ACaldrethPOIMarker` per POI, `Location.Z = **0.f**`, tag roli.

**(b) Czy czyta elevation → chodliwy grunt?** **NIE.** Import czyta wyłącznie `biome.png` (id biomu/piksel) i manifest (POI x/y). **`caldreth_data.npz` z `elevation` NIE jest przez import w ogóle otwierany.** `Z = 0.f` zahardkodowane dla stref i POI. Efekt = **płaskie strefy-dane 2D na origin**, bez ukształtowania. `ACaldrethZone.h` nie ma pola wysokości/heightmap.

**(c) Czego BRAKUJE do terenu pod strefami (właściwy blocker G1b):**
1. **Konsument `elevation`** — nikt nie czyta npz; elevation 512×512 to gotowy kandydat na heightmap, leży odłogiem.
2. **Generacja UE Landscape** — brak ścieżki elevation→`ALandscape` (ani import heightmap-PNG→Landscape, ani proceduralny mesh w C++).
3. **Wysokość stref/POI** — `Z=0` hardkod; brak próbkowania elevation w centroidzie (strefy nie siadają na gruncie).
4. **NavMesh** — brak `RecastNavMesh` bake nad ukształtowanym terenem (dziś navmesh tylko nad podłogą 8000×8000 @ origin — patrz BLOCKER #1 w GAME_STATE).
> **Diagnoza:** import = kompletna, działająca ścieżka **danych stref 2D** (biome.png→ACaldrethZone, manifest→POI). **Brakuje CAŁEJ warstwy 3D terenu.** Pipeline „elevation→landscape→navmesh" **nie istnieje** — to jest realny map-gate G1b, nie sama migracja.

### 0d. .gitignore / MapData
`MapData/caldreth_data.npz` + `MapData/caldreth_map.png` = ignored (regenerowalne). `biome.png` + `manifest.json` = **tracked** (wejścia importera). `Tools/MapGen/__pycache__/` ignored. Zgodne z notatkami.

## 1. RUN MapGen — ZABLOKOWANY (STOP, czekam na „tak")
`build_all.py` nie odpali: brak scipy (generate) + matplotlib (render) + Pillow (biome.png). **Propozycja (NIE wykonana, czekam na zgodę):**
```
python -m venv E:\Game_58\Tools\MapGen\.venv
E:\Game_58\Tools\MapGen\.venv\Scripts\python -m pip install numpy scipy matplotlib pillow
E:\Game_58\Tools\MapGen\.venv\Scripts\python E:\Game_58\Tools\MapGen\build_all.py
```
⚠️ **Ryzyko:** Python **3.14** jest bardzo świeży — koła (wheels) scipy/matplotlib/Pillow dla `cp314` mogą jeszcze nie istnieć na PyPI → instalacja mogłaby próbować budować ze źródeł (długo/ryzykownie). Jeśli tak — rekomendacja: venv na **3.11/3.12** (stabilne wheels). `.venv/` dopiszę do `.gitignore`.

## 2. WERYFIKACJA ARTEFAKTÓW — PENDING (wymaga RUN)
Twarde liczby będą po odpaleniu. **Oczekiwane z kodu (do potwierdzenia runem):**
- `grid_size` = **512** → elevation/rainfall/biome tablice **512×512** (elevation f32 = kandydat na heightmap 512²).
- `biome_legend` = **12** (OCEAN=0..OASIS=11) — zgodne z `zone_defs.json` (12) ✅.
- POI = **do 5** (Caldera zawsze; Great Tree/Obsidian/Grey Spring/River Source warunkowo; seed=7).
- #regionów biomów = **nieznane bez runu** (flood-fill na biome.png; ≥12, zwykle więcej — jeden biom = wiele rozłącznych plam). Porównanie „18 stref żywej CaldrethMap vs regiony manifestu" → **po runie**.

## 3. DIAGNOZA IMPORTU (RECON) — patrz 0c
- Czy UE 5.8 wczyta ŚWIEŻY manifest/biome.png? **Kod tak** (ścieżki `<Project>/MapData/…`, ImageWrapper PNG + Json parser gotowe) — ale **weryfikacja live wymaga runu MapGen + edytora** (dry-run inspekcja, BEZ stawiania — czeka na „tak" osobno).
- Mapowanie: `biome.png` → N× `ACaldrethZone` (2D, Z=0); `manifest.POI` → M× `ACaldrethPOIMarker` (2D, Z=0). **Luka:** brak landscape z elevation, brak Z z terenu, brak navmesh — jak 0c(1-4).

## OPEN (decyzja dyrektora — NIE samo-rozstrzygam)
1. **Rozmiar świata:** `WorldSizeUU` (mnożnik coords 0..1 → UU). Default w kodzie = `1000000.f` (100 km @ 1 uu=0.1 mm). Ile UU = wyspa Caldreth?
2. **elevation→landscape:** UE `ALandscape` z heightmap-PNG (elevation→16-bit PNG→import) czy proceduralny mesh/heightfield w C++? (audyt 0c pokazał: dziś ani jedno).
3. **Env:** zgoda na venv+pip? I czy trzymać 3.14, czy zejść na 3.11/3.12 dla wheels?

## DoD — stan
- [ ] build_all przeszedł + MapData/ artefakty — **BLOCKED (env, sekcja 1)**
- [x] Odpowiedź czy import czyta świeży manifest i CO powstaje — **TAK: 2D strefy+POI (Z=0), kod gotowy; live po runie**
- [x] **LUKA do terenu wypisana jednoznacznie** — 0c(1-4): brak konsumenta elevation, brak landscape, brak Z-z-terenu, brak navmesh
- [x] Zero aktorów, zero PIE
