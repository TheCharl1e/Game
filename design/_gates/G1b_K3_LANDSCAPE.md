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

## 3. NAVMESH + DIAGNOZA CHODLIWOŚCI — STOP (po sekcji 2)
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
