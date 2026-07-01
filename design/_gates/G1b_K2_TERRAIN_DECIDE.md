# GATE: G1b-K2-DECIDE — teren: Landscape vs proceduralny mesh (RECON + rekomendacja)
> Wykonawca: Claude Code · 2026-07-01 · E:\Game_58 · Branch: docs/pyramid-C-L-G
> Status: **RECON DONE · MapGen RUN DONE (twarde liczby) · REKOMENDACJA: DROGA A (ALandscape)**
> Zero budowy terenu, zero aktorów, zero PIE. Wybór drogi = decyzja dyrektora.

## 0. MapGen RUN — twarde liczby (venv 3.12, numpy2.5/scipy1.18/matplotlib3.11/pillow12.3)
`build_all.py` przeszedł (exit 0). Artefakty w `E:\Game_58\MapData\`:
| Artefakt | Rozmiar | Twarde dane |
|---|---|---|
| `caldreth_data.npz` | 1.43 MB | **elevation 512×512 f32, min 0.0 max 1.0** (znormalizowana) · rainfall 512×512 f32 [0..1] · biome 512×512 i16 [0..11] |
| `caldreth_biome.png` | 5.25 KB | **512×512, mode L (8-bit grey)**, kanał = biome id |
| `caldreth_manifest.json` | 1.57 KB | grid_size **512**, sea_level **0.2**, biome_legend **12** (OCEAN..OASIS), **POI=5** (caldera/tree/obsidian/grey/spring) |
| `caldreth_map.png` | 546 KB | kanon wizualny |

**Flood-fill (replikacja importera: 4-conn, MinRegionPixels=8) → dokładnie 18 stref-aktorów:**
Ocean 2 · Beach 2 · Savanna 3 · Desert 2 · Grassland 2 · SlopeForest 1 · Mountain 1 · AshSlope 1 · Caldera 1 · River 1 · Lava 1 · Oasis 1. (bSkipOcean → 16). Legenda 12 = zgodna z `zone_defs.json` ✅.
> **KONFIRMACJA:** te 18 = te same „18 stref żywej CaldrethMap". Pipeline danych (Python→manifest/biome.png→import) **żyje po migracji**. `elevation` [0..1] to gotowy heightmap-kandydat — sea_level 0.2 = 20% zanurzenia.

## 1. RECON stanu projektu (fakty pod obie drogi)
- `Stan_Pierwotny.Build.cs` deps: Core/CoreUObject/Engine/InputCore/GameplayStateTree/StateTree/EnhancedInput/AIModule/**NavigationSystem**/UMG. **Brak Landscape, brak ProceduralMeshComponent, brak GeometryFramework** → każda droga = +1 moduł.
- Source: **zero** użycia Procedural/Dynamic/RuntimeMesh ani ALandscape (greenfield w obie strony).
- Pluginy: **ModelingToolsEditorMode + AllToolsets** (GeometryScript/DynamicMesh dostępne), Water, DynamicWind, NavigationSystem obecny.

### DROGA A — UE `ALandscape` z heightmap
- **Data-driven z npz:** elevation 512×512 f32 [0..1] → skala ×65535 → **uint16 heightmap** → import editor-side (`ALandscape` + `ULandscapeInfo`/`FLandscapeImportHelper`). Mała util (npz→.r16/PNG16 + wywołanie importu).
- **Rozdzielczość:** Landscape woli rozmiary `(sekcje×komponenty)+1` (505/509/1009…); **512 nie jest kanoniczne** → resample/pad do 505 lub upsample do 1009. Znany, rozwiązywalny detal.
- **Navmesh:** `RecastNavMesh` na Landscape = **out-of-box**, dojrzały, sprawdzony przy 500+ (kolizja Landscape natywna).
- **Edytowalność ręczna:** sculpt/paint natywnie ✅.
- **Moduł:** +`Landscape` (+`LandscapeEditor` w module edytora) — silnikowe, bez pluginu.

### DROGA B — proceduralny mesh/heightfield w C++
- **Data-driven z npz:** pełna kontrola — C++ czyta elevation, buduje siatkę 512×512 wierzchołków. **Najsilniejsza zgodność z „C++=mózg"** ✅.
- **Navmesh:** **znany dług UE** — `ProceduralMeshComponent`/`DynamicMeshComponent` mają słabe/kruche wsparcie navmeshu; Recast bake na runtime-generowanym meshu bywa zawodny przy 500+ NPC. **To główne ryzyko drogi B.**
- **Edytowalność ręczna:** brak (tylko regeneracja).
- **Kod od zera:** dużo — generacja heightfield + kolizja + LOD + nav collision własnoręcznie.
- **Moduł:** +`ProceduralMeshComponent` lub GeometryScript (pluginy są), ale infra nav do dopisania.

## 2. TABELA PORÓWNAWCZA (fakt-po-fakcie, per nasz projekt)
| Kryterium | DROGA A — Landscape | DROGA B — Proc. mesh |
|---|---|---|
| Data-driven z npz (bez ręcznej roboty) | ✅ util npz→r16→import (mały kod) | ✅✅ C++ czyta npz wprost (pełna kontrola) |
| **Navmesh out-of-box (500+ NPC)** | ✅ **RecastNavMesh natywnie, dojrzały** | ❌ **dług UE: nav na PMC/DynamicMesh kruchy** |
| Edytowalność ręczna (sculpt/paint) | ✅ natywnie | ❌ brak |
| Spójność z „C++=mózg" | ⚠️ teren = asset edytora (util steruje) | ✅ pełna, teren z matematyki C++ |
| Ile kodu od zera | 🟢 mało (reuse importu silnika) | 🔴 dużo (mesh+kolizja+LOD+nav) |
| Ryzyko/długi UE | 🟢 niskie (mature) | 🔴 wysokie (navmesh) |
| Skalowanie do WorldSizeUU | ✅ transform/tiling standard | ⚠️ ręczny chunking |
| 512-grid dopasowanie | ⚠️ resample do 505/1009 | ✅ 512 wprost |

## 3. REKOMENDACJA: **DROGA A — ALandscape z heightmap**
Rekomenduję Landscape, bo **właściwym blockerem G1b jest chodliwy grunt + navmesh dla 500+ NPC**, a Recast na Landscape działa out-of-box i dojrzale — droga B upada dokładnie na tym (navmesh na proceduralnym meshu to znany dług UE, wysokie ryzyko przy naszej skali). Do tego elevation→r16→import to mały krok z reuse silnika, a ręczny sculpt zostaje jako wentyl bezpieczeństwa. „C++=mózg" nie jest naruszone: teren to „ciało" (dane), a util importujący jest cienki i data-driven z npz.
> **Warunek-obalenia:** jeśli wymóg to **runtime'owa regeneracja/deformacja terenu w trakcie symulacji** (bez edytora, np. wulkan zmienia grunt na żywo) — Landscape (asset edytora) tego nie zrobi → wtedy wygrywa proceduralny mesh. G1b = statyczna wyspa zapieczona raz → Landscape pasuje.

## OPEN (decyzja dyrektora — NIE rozstrzygam)
1. **WorldSizeUU** (default w kodzie 1000000): ile UU = wyspa Caldreth? Determinuje skalę XY importu i zakres Z heightmapy (elevation [0..1] → ile UU wysokości?).
2. **Landscape vs mesh** — wybór Twój na podstawie tabeli+rekomendacji powyżej.
3. (pochodne po wyborze A) docelowa rozdzielczość Landscape (505/509/1009) + zakres Z.

## DoD — stan
- [x] MapGen run + twarde liczby (#regionów **18**, #POI **5**, elevation **512×512 [0..1]**, biome.png **512×512 L**)
- [x] Tabela porównawcza wypełniona faktami z kodu/docs projektu
- [x] Jedna rekomendacja (DROGA A) + warunek-obalenia
- [x] Zero budowy terenu, zero aktorów, zero PIE
