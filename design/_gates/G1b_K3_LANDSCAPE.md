# GATE: G1b-K3 — Landscape z heightmap + navmesh + diagnoza chodliwości
> Wykonawca: Claude Code · 2026-07-01 · E:\Game_58 · Branch: docs/pyramid-C-L-G
> Status: **AUDYT (0) DONE · UTIL heightmap (1) DONE · Landscape (2)/navmesh (3)/re-osadzenie (4) = STOP, czekam „tak"**
> Zero aktorów postawionych, zero PIE, zero edycji Build.cs. Level-edit gated.

## RESOLVED (dyrektor):
Droga A `ALandscape` z heightmap · WorldSizeUU **1000000** (XY) · elevation 1.0→**90000 UU** (~900m), sea_level 0.2→Z bazowy · teren **STATYCZNY**. Poza zakresem: elastyczność generatora, koszt-nachylenia.

## 0. AUDYT
### Build.cs — czego trzeba do heightmap-import
- **Runtime** `Stan_Pierwotny.Build.cs`: Core/CoreUObject/Engine/InputCore/GameplayStateTree/StateTree/EnhancedInput/AIModule/**NavigationSystem**/UMG. Brak Landscape.
- **Editor** `Stan_PierwotnyEditor.Build.cs`: Public Core/CoreUObject/Engine/Stan_Pierwotny · Private **UnrealEd/ImageWrapper/Json**. **Brak Landscape.**
- **Do data-driven importu w C++** (analogicznie do `CaldrethImportLibrary`) trzeba dodać do editor-modułu: `+= "Landscape", "LandscapeEditor"` (private). To **edycja Build.cs + rebuild z zamkniętym edytorem = TWARDA BRAMKA**.
- **Ścieżki importu heightmap w 5.8 (recon, do wyboru w sekcji 2):**
  - **(i) Ręcznie/edytor:** Landscape Mode → Manage → Import from File (`.png` 16-bit / `.r16`). Zero kodu, ale nie w pełni data-driven.
  - **(ii) MCP Python:** sterowanie edytorem (utility) — potencjalnie bez zmiany Build.cs (edytor ma moduł Landscape załadowany). Do potwierdzenia że API importu osiągalne przez reflection.
  - **(iii) C++ util** `ImportCaldrethLandscape` (spójne z „C++=mózg"): `ALandscape` + `ULandscapeInfo`/`FLandscapeImportHelper::GetHeightmapImportData` → wymaga Build.cs += Landscape (twarda bramka).

### Rozdzielczość Landscape (512 → kanoniczny rozmiar) — PROPOZYCJA, wybór = OPEN
Landscape woli `(quads×sekcje)+1`. 512 nie jest kanoniczne. Opcje:
| Rozmiar | Jak | Δ vs 512 | Koszt | Uwaga |
|---|---|---|---|---|
| **505×505** | 63 quads × 8 komp. (sekcja 63×1) | downsample −7 px | 🟢 ~255k verts | najbliżej 512, minimalna strata — **rekomendacja** |
| 509×509 | 127 quads × 4 komp. | downsample −3 px | 🟢 podobny | mniej komponentów |
| 1009×1009 | 63 quads × 16 komp. | upsample ×~2 | 🔴 ~1M verts (×4) | więcej detalu, drożej |
> Rekomendacja **505** (najtaniej + najbliżej). Ostateczny wybór Twój (OPEN).

## 1. UTIL npz→heightmap (`Tools\MapGen\heightmap_export.py`, venv 3.12) — DONE
Wejście `MapData/caldreth_data.npz` (elevation 512×512 f32). Wyjście → `MapData/`:
| Plik | Rozmiar | Dane |
|---|---|---|
| `caldreth_height16.png` | 322 KB | 16-bit greyscale, 512×512, **min 0 / max 65535** (pełny zakres) |
| `caldreth_height.r16` | **524288 B** (=512×512×2 ✅) | raw LE uint16 |
- Mapping (liniowy w elevation): **0→0 UU · sea_level 0.2→18000 UU · 1.0→90000 UU**. Skala Z=90000 aplikowana transformem Landscape przy budowie (nie w pikselach → plik działa dla dowolnego WorldSize).
- **63.2% komórek nad poziomem morza** (grunt-kandydat) — większość wyspy to ląd, dobry omen dla chodliwości.
- Heightmapy **gitignored** (regen z npz + util deterministycznie; util `.py` tracked).

## 2. BUDOWA LANDSCAPE — STOP (czekam „tak")
**PLAN (nie wykonany):** postawić `ALandscape` na `Content/DocelowaGra/CaldrethMap.umap`, wyśrodkowany na origin, z `caldreth_height16.png`/`.r16`, XY tak by pokryć 1000000 UU, Z-scale by 65535≡90000 UU. Zapis mapy = Twój Ctrl+S.
- **Wymaga wyboru:** (a) rozdzielczość (505/509/1009), (b) ścieżka importu (i/ii/iii — jeśli iii to Build.cs edit).
- Z-scale Landscape: przy komponencie i XY-scale = `1000000/(res-1)` na oś; Z-scale dobrany tak, że zakres 16-bit = 90000 UU. Dokładne liczby policzę po wyborze rozdzielczości.

## 2 c.d. (2026-07-01, RESOLVED: 505×505, MCP Python, zgoda) — PRÓBA WYKONANIA → BLOCKED (edytor down)
- **Prep DONE:** `Tools\MapGen\` resample 512→**505×505** (bilinear, scipy zoom) → `MapData/caldreth_height_505.r16` = **510050 B** (=505×505×2 ✅), min16 0 / **max16 65501** (szczyt przetrwał resample). Gitignored.
- **MCP niedostępny:** `mcp-unreal :8090` → connection refused; `monolith :9316` → „Unreal Editor not running". **Edytor Game_58 NIE działa** → import Landscape (MCP Python) niemożliwy.
- **STOP — potrzebne:** uruchom edytor **Game_58 (5.8)** na `Content/DocelowaGra/CaldrethMap.umap`, pojedyncza instancja (unikać kolizji portu 2. edytora), plugin MCPUnreal + Python Editor Script Plugin aktywne. Po starcie wznawiam: import 505 r16 → read-back min/max Z → Ctrl+S.
- READ-BACK (min/max Z realnej geometrii) = **PENDING** (asekuracja na dryf MCP wg gate 2b).

## 2 d. (2026-07-01, edytor wstał) — DWA BLOKERY, STOP przed importem
Edytor Game_58 żywy (`execute_script` działa; stdout nie wraca → zapis do `Saved/*.txt` + odczyt z dysku, znany wzorzec).

### BLOKER 1 — zła mapa
`get_editor_world()` = **`Game`** (`/Game/DocelowaGra/Game.Game`), **NIE `CaldrethMap`**. Gate wprost: import tylko na CaldrethMap. → trzeba `Content/DocelowaGra/CaldrethMap.umap` (File→Open Level lub `LevelEditorSubsystem.load_level`; nie przełączam sam — ryzyko utraty niezapisanych zmian na Game.umap).

### BLOKER 2 — Python nie ma file-importu Landscape (reality ≠ plan „MCP Python")
Introspekcja `unreal` (5.8) wykazała:
- `Landscape`/`LandscapeProxy` wystawiają **tylko** `landscape_import_heightmap_from_render_target(rt, import_height_from_rg_channel, edit_layer_index)` — **nadpisuje heightmapę ISTNIEJĄCEGO** Landscape z `TextureRenderTarget2D` (RTF_RGBA16f/32f/8). **Nie tworzy** Landscape z pliku.
- `LandscapeSubsystem`, `EditorLandscapeLibrary`, `LandscapeEditorObject`, `LandscapeInfo`, `LandscapeImportHeightmapData`, `new_landscape` = **ABSENT**.
- **Wniosek:** Python nie zbuduje bazowego Landscape 505×505 z r16. RT-import wymaga już istniejącej bazy o właściwej rozdzielczości → sama baza to blocker.

### OPCJE (decyzja dyrektora — nie rozstrzygam)
| Opcja | Jak | Plusy | Minusy |
|---|---|---|---|
| **A — ręczny import w edytorze (REKOMENDACJA)** | Landscape Mode→Manage→Import from File → `MapData/caldreth_height_505.r16`, 505×505, wg moich dokładnych ustawień | najszybciej (~2 min), najpewniej, jednorazowy statyczny bake, zero rebuildu | ręczny krok dyrektora (nie w pełni data-driven) |
| B — hybryda | dyrektor tworzy PŁASKI Landscape 505×505, ja wpycham wysokości Python RT-import (RGBA16f R-channel) | częściowo Python | więcej kroków, ryzyko precyzji RT, i tak ręczna baza |
| C — C++ util (path iii) | `+= Landscape/LandscapeEditor` w Build.cs + `ImportCaldrethLandscape` (`FLandscapeImportHelper`/`ALandscape::Import`) + rebuild (edytor zamknięty) | w pełni data-driven, „C++=mózg", powtarzalne | Build.cs edit + rebuild = twarda bramka, cięższe |

**Gotowe ustawienia dla Opcji A** (podam dokładnie po Twoim „tak"): rozmiar 505 (63 quads × 8 komponentów, sekcja 63×1); Scale X=Y ≈ **1984.13** (1000000/504 quadów); Scale Z ≈ **17578** + Location.Z ≈ **+45000** → elev0→Z0, sea_level0.2→Z18000, elev1.0→Z90000; wyśrodkowany na origin.

**STOP — proszę o decyzję:** (1) otwórz/pozwól otworzyć **CaldrethMap**; (2) wybierz ścieżkę importu **A / B / C**. Po tym wznawiam: import → **read-back realnego max Z** → Twój Ctrl+S → navmesh.

## 2 e. (2026-07-01, dyrektor wybrał OPCJĘ C — C++ util) — KOD NAPISANY, rebuild pending
**Korekta liczby:** `ScaleZ = 90000×128/65535 ≈ **175.78**` (nie 17578 — błąd ×100 w propozycji A; przez `LANDSCAPE_ZSCALE=1/128`).
Zmiany (patch, nie regeneracja):
- `Stan_PierwotnyEditor.Build.cs`: PrivateDeps += **`Landscape`, `LandscapeEditor`, `LandscapeEditorUtilities`**.
- `CaldrethImportLibrary.h/.cpp`: nowa `static AActor* ImportCaldrethLandscape(HeightmapR16Path, SizeVerts=505, SubsectionSizeQuads=63, NumSubsections=1, WorldSizeUU=1000000, ZScale=175.78, ZOffsetUU=45000)`.
  - czyta `.r16` LE uint16 (walidacja bajtów = SizeVerts²×2), waliduje layout (504 % 63 == 0 → 8×8 komponentów), spawn `ALandscape` wyśrodkowany (Loc `(-500000,-500000,+45000)`, Scale `(1984.13,1984.13,175.78)`), `ALandscape::Import` (default edit-layer, bez weightmap), `CreateLandscapeInfo`+`UpdateLayerInfoMap`, log min/max16 + oczekiwane world-Z.
- **Ryzyko:** pierwszy build — sygnatura `ALandscape::Import` / nagłówek `ELandscapeImportAlphamapType` mogą się różnić w 5.8 → dogram po błędach kompilatora.
- **BUILD (2026-07-01):** 2 błędy naprawione i **rebuild ZIELONY (exit 0)**: (1) `bCanHaveLayersContent` nie istnieje w 5.8 → usunięte; (2) `ALandscapeProxy::Import` w 5.8 ma **12. param** `TArrayView<const FLandscapeLayer>&` (bez defaultu) → dodany pusty. `Game_58Editor` target zlinkowany.

**STOP — twarda bramka rebuild:** editor-moduł zmienił deps → potrzebny **pełny rebuild z ZAMKNIĘTYM edytorem** (Live Coding nie łyknie zmiany Build.cs). Proszę: **zamknij edytor Game_58** → ja odpalam build editor-targetu (UBT) → po zielonym: otwórz ponownie na **CaldrethMap** → wołam `ImportCaldrethLandscape` → read-back max Z → Twój Ctrl+S → navmesh.

## 2 f. (2026-07-01) — LANDSCAPE POSTAWIONY + READ-BACK + 2 bugi złapane
- **Mapa:** `load_level` NIE zadziałał (świat zostawał `Game`); **`EditorLoadingAndSavingUtils.load_map("/Game/DocelowaGra/CaldrethMap")` zadziałał** → LEVEL=CaldrethMap potwierdzony.
- **Import:** `ImportCaldrethLandscape("",505,63,1,1000000,175.78,45000)` → `ALandscape "CaldrethLandscape"`, LANDSCAPE_COUNT=1. Heightmapa **weszła** (realny relief).
- **BUG #1 — skala:** `ALandscape::Import` **nadpisuje transform** własną domyślną skalą: read-back scale=`(253968, 253968, 44999)` = intended ×128 (XY) / ×256 (Z), bounds Z ±11.5M. → **fix C++:** `SetActorTransform` PO `Import`.
- **READ-BACK po korekcie skali (twarde, realna geometria):** scale `(1984.127,1984.127,175.78)`, **XY span = 1000000×1000000 ✅**, **Z_MIN 0.3 / Z_MAX 89951.6 ✅** (≈0..90000, szczyt ~900m), origin (0,0) — wyśrodkowany. **Heightmapa poprawna.**
- **BUG #2 — kolizja:** przeskalowanie z Pythona zostawia **nieaktualny heightfield kolizji** → line-trace: center trafia stary Floor (−170), `ne`=116225 (resztka ×256), west/corner **MISS** (brak kolizji). `RecreateCollisionComponents` **nie wystawione do Pythona**. → **fix C++:** `RecreateCollisionComponents()` po `SetActorTransform`.
- **Status:** żywy Landscape ma poprawną geometrię ale **stalą kolizję** → navmesh na nim = śmieć. **Nie bakeuję.** Poprawki C++ wniesione → potrzebny rebuild + czysty re-import (transform+kolizja od zera).

**STOP — proszę: zamknij edytor BEZ zapisu** (odrzuca zepsuty Landscape) → ja rebuild → otwórz CaldrethMap → czysty re-import (skala+kolizja poprawne od startu) → read-back bounds+trace (wszystkie trafienia) → Twój Ctrl+S → navmesh.

## 2 g. (2026-07-01) — CZYSTY RE-IMPORT PO FIXIE — READ-BACK ZIELONY
Rebuild zielony → CaldrethMap → usunięto stary Landscape (DELETED_OLD=1) → `ImportCaldrethLandscape` czysty:
- **Skala poprawna OD STARTU:** `(1984.127, 1984.127, 175.78)` — fix (SetActorTransform po Import) działa.
- **Bounds:** XY span **1000000×1000000**, Z **0.3..89951.6** (≈0..90000), wyśrodkowany.
- **KOLIZJA (line-trace 11 punktów): 11/11 trafień** ✅, surface Z **[669..83522]** — center ~83522 (szczyt wulkanu), farE 669 (wybrzeże). Kolizja gęsta → **navmesh-ready** (fix RecreateCollisionComponents działa).
> DoD 2b spełniony: realny max Z ~90000 (nie założenie), kolizja potwierdzona traceami. **STOP 2c → proszę o Ctrl+S** (zapis CaldrethLandscape do CaldrethMap.umap) przed navmeshem.

## 3. NAVMESH — próba #1 (default cell) → NIE SKALUJE do 10 km, restart + grubszy nav
- **Recon nav:** mapa ma **18 stref** (Z=0), **5 POI**, Landscape, + istniejący NavMeshBoundsVolume + RecastNavMesh (z ery podłogi 8000×8000). Recast: **agent_max_slope=44°**, radius 35, height 144.
- **bSpawnable (z zone_defs.json):** 9 stref spawnable (Beach×2, Savanna×3, Grassland×2, SlopeForest×1, Oasis×1); 9 non-spawnable (Ocean×2, Desert×2, Mountain, AshSlope, Caldera, River, Lava).
- **Próba:** resize NavMeshBounds na pokrycie 1M×1M auto-wyzwolił **synchroniczny rebuild** → edytor zablokowany (2× timeout na read-only), **RAM 7.0→7.2 GB i rosnący** (~85 MB/6s), CPU narasta. Przyczyna: RecastNavMesh **domyślny cell-size (~19 uu) × 10 km = setki tysięcy kafli**.
- **Werdykt:** navmesh nie skaluje do WorldSize=1 000 000 przy domyślnych ustawieniach. **Decyzja dyrektora: restart edytora + zgrubienie RecastNavMesh** (większy CellSize/TileSize; opcjonalnie ciaśniejszy bounds bez oceanu). Build nie zapisany → odrzucony przy restarcie; Landscape zapisany (Ctrl+S) ocalał.
- **NASTĘPNY KROK (po reopen):** ustaw grubszy RecastNavMesh PRZED rozszerzeniem bounds → rebuild feasible → diagnoza chodliwości.

## 3b. NAVMESH — próba #2 (grubszy) — ZBUDOWANY + DIAGNOZA
- **Config:** cell_size w `nav_mesh_resolution_params` (default było **19** → (1e6/19)²≈2.7 mld cells = eksplozja #1). Podniesione; tile_size_uu=25000, pool=2048.
- **TWARDY LIMIT SKALI:** Recast wymusza `cell_size ≥ tile_size/max_cells_per_tile` → przy 10 km i puli kafli cell **wpada na ~397 (4 m)**, nie da się zejść niżej bez ~27000 kafli = eksplozja RAM. **Przy WorldSize=1 000 000 (10 km) navmesh rozróżniający stromiznę na poziomie metra jest NIEWYKONALNY w rozsądnej pamięci.** Build @ cell 397: RAM stabilny 4 GB, ~441/1764 kafli.
- **DIAGNOZA CHODLIWOŚCI (grid 16×16 = 256 + line-trace + project_point_to_navigation, tight extent):**
  - **Chodliwa powierzchnia: 84.0%** (215/256); **16% odcięte** = najstromsze flanki stożka.
  - **Strefy na navmeshu: 14/18.** **Spawnable na navmeshu: 7/9 (78%) → bramka bezpieczeństwa PASS** (≥50%, brak re-tune Z).
  - Non-walk strefy: Mountain(826m ✅ stromo), SlopeForest, Savanna, River.
  - agent_max_slope Recast = **44°** (do przyszłego kosztu-nachylenia).
- **ZNALEZISKO — coarse nav zawyża chodliwość:** przy cell 4 m nawet szczyt (835m) bywa WALK=True (uśrednia stożek). 84% to górna granica; drobny nav ciąłby więcej — ale drobny jest niewykonalny (patrz limit skali).
- **ZNALEZISKO #2 — strefy NIE pokrywają się z terenem (XY):** surfZ pod strefami: **Beach@714m, Ocean@337m** (powinny być nisko) vs Caldera@883m/Mountain@826m (poprawnie wysoko). Część stref (centralne/wysokie) pasuje, brzegowe/niskie NIE → **strefy (import historyczny) mają XY z INNEGO biome.png niż obecny Landscape.** Naiwne re-osadzenie (sekcja 4, tylko Z-drop) posadzi je na złym terenie — trzeba re-wyprowadzić XY ze świeżego manifestu/maski. → **sekcja 4 = STOP, decyzja dyrektora.**

## WERDYKT K3
- ✅ Landscape stoi z heightmapy (wulkan ~900m, pierścienie), real max Z 89951 (read-back).
- ✅ Navmesh zbakowany; **wyspa PRZECHODNIA (84% chodliwe, ścieżki po terenie istnieją)**; spawnable 7/9 na nav (PASS).
- ⚠️ Drobny (slope-gating) navmesh **niewykonalny przy 10 km** → decyzja: (a) zaakceptować coarse nav + nachylenie jako koszt L1 (nie nav-gate), (b) mniejszy WorldSize, (c) nav invokers (dynamiczny nav wokół NPC).
- ⚠️ Strefy XY rozjechane z Landscape → re-osadzenie wymaga re-derywacji XY (nie sam Z-drop).

## 4. RE-IMPORT STREF (dyrektor: ze świeżego biome.png) — DONE + KOREKTA diagnozy
- Usunięto stare 18 stref + 5 POI → `ImportCaldrethZones("",DT_ZoneDefs,1e6)` + `ImportCaldrethPOIs` (Python: `import_caldreth_po_is`, mangling „POIs") → **18 stref + 5 POI**, re-osadzone na terenie (line-trace Z-drop, **0 miss**).
- **KOREKTA „Finding #2" (była BŁĘDNA):** re-import dał ~identyczne pozycje (Beach 71452→71105, Ocean 33700→33699) → to NIE był inny biome.png. Dane biome↔elevation są **spójne** (jeden `generate(cfg)`).
- **PRAWDZIWA przyczyna — artefakt centroidu regionu niewypukłego** (lokalny test na npz, Z w UU):
  | biome | centroid-elev | mean-pixel-elev | n |
  |---|---|---|---|
  | Ocean | **71286** | 9466 | 96537 |
  | Beach | **68998** | 19426 | 21419 |
  | Savanna | 74669 | 39675 | 20823 |
  | Grassland | 78033 | 40028 | 8644 |
  | Caldera | 88205 | 86642 | 808 |
  Piksele biomów są na dobrych wysokościach (Ocean 95m, Beach 194m≈poziom morza); **centroid pierścienia/pasma wpada w wysoki środek wyspy**. `ImportCaldrethZones` stawia aktor w centroidzie → dla biomów-pierścieni (Ocean/Beach/Savanna) marker ląduje na górze. Kompaktowe (Caldera/AshSlope) OK.
- **Implikacja:** strefa to REGION (`NormalizedOutline`), więc dla `GetZoneAtLocation` centroid-marker jest kosmetyczny. Jeśli marker ma siadać na swoim biomie → importer potrzebuje **reprezentatywnego punktu** (pole-of-inaccessibility / najbliższy piksel-regionu do centroidu), nie centroidu. = osobny gate (poprawka importera).
- **UWAGA:** wcześniejsze „7/9 spawnable na nav" mierzyło centroidy (dla pierścieni przesunięte) → nie jest wiarygodną miarą pokrycia biomu; wyspa-poziom **84% chodliwe** stoi.

## OPEN (decyzja dyrektora)
1. **Nav invokers (wybrane):** dynamiczny nav wokół NPC → pozwala na DROBNY cell (bounded area). Wymaga: config (`bGenerateNavigationOnlyAroundNavigationInvokers`, RuntimeGeneration=Dynamic) + `UNavigationInvokerComponent` na NPC + **PIE-verify**. To osobny pod-etap (C++/BP + PIE). Plan gotowy — czekam na „tak" na sekwencję (config → invoker na NPC → PIE).
2. **Importer stref — reprezentatywny punkt** zamiast centroidu (fix markerów pierścieni)? Osobny gate — robimy?
3. Zapis mapy (Landscape + strefy-na-terenie; nav-config) — Twój Ctrl+S, gdy zdecydujemy nav-invoker kierunek.
Plan: `NavMeshBoundsVolume` nad Landscape + `RecastNavMesh` bake → twarde liczby: % chodliwej powierzchni, ile z 18 stref (i ile bSpawnable) na navmeshu, gdzie nav urywa się na stożku wulkanu, agent-max-slope Recast. **Wymaga postawionego Landscape.**

## 4. RE-OSADZENIE STREF/POI — STOP (po sekcji 2/3)
Plan: line-trace w dół → osadzić istniejące 18 stref + 5 POI na powierzchni (dziś Z=0 hardcode importera). Bez globalnego przepisania importera.

## OPEN (decyzja dyrektora)
1. **Rozdzielczość Landscape:** 505 (rekomendacja) / 509 / 1009?
2. **Ścieżka importu:** (i) ręcznie-edytor / (ii) MCP Python / (iii) C++ util (+Build.cs Landscape, rebuild)?
3. Zgoda na sekcję 2 (postawienie Landscape = level edit)?
> Bramka bezpieczeństwa (Twoja): jeśli po nav-bake <50% stref bSpawnable ma chodliwy grunt → STOP, re-tune Z, nie brnę.

## DoD — stan
- [x] Audyt Build.cs + ścieżki importu + propozycja rozdzielczości (OPEN)
- [x] Util heightmap: 16-bit 0..65535, r16 524288 B, Z-map 0/18000/90000, 63.2% ląd
- [ ] Landscape postawiony — **STOP (sekcja 2)**
- [ ] Navmesh + twarde % chodliwości + strefy-na-navmeshu — **STOP (sekcja 3)**
- [ ] Strefy/POI osadzone na terenie — **STOP (sekcja 4)**
