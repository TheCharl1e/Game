# Raport — AmbientTemp ETAP A: RECON (odświeżenie) · 2026-06-23

> RECON only. Zero kodu produkcyjnego: nie dodano pól do FZoneDef, nie napisano
> resolvera, nie ruszono nagłówków. Nic nie commitowane poza tym raportem.
> Odświeżenie raportu z 2026-06-19 — liczby przeliczone od nowa z dysku.

## Metoda
1. **Zone table** — `python Gra_Stan_Pierwotny/Scripts/zone_table_from_npz.py` (uruchomione dziś).
   Replikuje flood-fill z `Source/Stan_PierwotnyEditor/CaldrethImportLibrary.cpp`
   (4-connected, MinRegionPixels=8, bSkipOcean=false, skip biome>11) na `MapData/caldreth_data.npz`.
   Wynik: **dokładnie 18 stref, 0 odrzuconych jako szum** (deterministyczne — 1:1 z 2026-06-19).
2. **Sun curve** — model SunFactor potwierdzony ze źródła C++ (`MaslowBiologicalComponent.cpp`).
   Krzywa SunIntensity (4 fazy) z live-verified raportu doby 2026-06-19 (graf `THE ATMOSPHERE`).
3. **Night floor** — edytor OFFLINE dziś (Monolith :9316 i MCPUnreal :8090 → `editor_online:false`,
   brak procesu UE). Live read NIEMOŻLIWY; podana wartość z DYSKU (live-verified 2026-06-19) + metoda
   świeżego odczytu. **Nie wołano setterów na ślepo** (zgodnie z zadaniem).

---

## BLOK 1 — ZONE TABLE (18 stref, z caldreth_data.npz)

NPZ: `elevation (512×512 float32)`, `rainfall`, `biome (512×512 int16, id 0..11)`.
Elewacja **znormalizowana 0..1** (== `elev/elev.max()` z mapgen; global min/max = 0.000/1.000).
`sea_level=0.200`. Grid 512×512 → area = liczba komórek. 12 biomów rozpada się na 18 **spójnych
komponentów** (np. OCEAN i SAVANNA są geograficznie rozdzielone). id = kolejność napotkania (row-major).

| id | biome        | area (komórki) | e_mean | e_min | e_max |  cx   |  cy   |
|---:|--------------|---------------:|-------:|------:|------:|------:|------:|
| 0  | OCEAN        | 16513          | 0.147  | 0.044 | 0.200 | 0.127 | 0.114 |
| 1  | BEACH        | 19711          | 0.216  | 0.200 | 0.232 | 0.385 | 0.461 |
| 2  | OCEAN        | 80024          | 0.097  | 0.000 | 0.200 | 0.715 | 0.634 |
| 3  | SLOPE_FOREST | 39496          | 0.370  | 0.232 | 0.696 | 0.679 | 0.459 |
| 4  | GRASSLAND    | 5050           | 0.440  | 0.232 | 0.696 | 0.575 | 0.199 |
| 5  | SAVANNA      | 10003          | 0.446  | 0.232 | 0.696 | 0.488 | 0.197 |
| 6  | DESERT       | 50943          | 0.389  | 0.232 | 0.696 | 0.247 | 0.446 |
| 7  | MOUNTAIN     | 12464          | 0.767  | 0.696 | 0.840 | 0.490 | 0.495 |
| 8  | ASH_SLOPE    | 5303           | 0.885  | 0.840 | 0.944 | 0.515 | 0.503 |
| 9  | CALDERA      | 808            | 0.963  | 0.944 | 1.000 | 0.539 | 0.505 |
| 10 | SAVANNA      | 2583           | 0.405  | 0.232 | 0.660 | 0.209 | 0.646 |
| 11 | LAVA         | 882            | 0.498  | 0.197 | 0.939 | 0.360 | 0.720 |
| 12 | OASIS        | 1652           | 0.393  | 0.232 | 0.627 | 0.203 | 0.652 |
| 13 | RIVER        | 575            | 0.367  | 0.195 | 0.607 | 0.185 | 0.665 |
| 14 | GRASSLAND    | 3594           | 0.451  | 0.232 | 0.696 | 0.515 | 0.768 |
| 15 | DESERT       | 2803           | 0.352  | 0.232 | 0.691 | 0.323 | 0.793 |
| 16 | SAVANNA      | 8022           | 0.441  | 0.232 | 0.696 | 0.426 | 0.769 |
| 17 | BEACH        | 1718           | 0.216  | 0.200 | 0.232 | 0.134 | 0.819 |

Per-biome liczba stref: OCEAN×2, BEACH×2, SAVANNA×3, DESERT×2, GRASSLAND×2, SLOPE_FOREST×1,
MOUNTAIN×1, ASH_SLOPE×1, CALDERA×1, RIVER×1, LAVA×1, OASIS×1 = **18**. Total komórek = 262144.

⚠️ **Do lapse-rate:** plik NIE ma metrów — elewacja jest 0..1 (ułamek wysokości szczytu).
`BaseTemp = biome_base + lapse(elev)` wymaga przyjęcia skalaru "wysokość szczytu w m".
⚠️ Najzimniejsze po lapse (najwyższe e_mean): **CALDERA id9 (0.963)**, **ASH_SLOPE id8 (0.885)**,
**MOUNTAIN id7 (0.767)**. Strefy tego samego biomu = osobne id (osobne komponenty).
⚠️ LAVA id11 ma e_min 0.197 / e_max 0.939 (ścieżka przecina elewacje) — biome-base musi dominować, nie lapse.

---

## BLOK 2 — SUNINTENSITY CURVE

**Plik:** `/Game/DocelowaGra/WORLD/BP_DayNightCycle` (`Content/DocelowaGra/WORLD/BP_DayNightCycle.uasset`)
— jedyny żywy zegar (drugi, `AI_NPC/BP_DayNightCycle`, martwy: 0 ref mapy).

**Funkcja licząca SunIntensity:** `THE ATMOSPHERE` (NIE `THE SUN` — `THE SUN` liczy tylko geometrię
obrotu DirectionalLight: SunPitch/SunYaw). Krzywa = **4 fazy doby, cap = `MaxSunIntensity` = 100**:

| Faza | Zakres | Wzór SunIntensity |
|---|---|---|
| **Noc** | poza oknem dnia | `0` (literał) ← **night floor** |
| **Świt** | [DawnStart..DawnEnd] | `Lerp(0, 25, T)`, T=(t−DawnStart)/(DawnEnd−DawnStart) — liniowo 0→25 |
| **Dzień** | [DawnEnd..DustStart] | `Lerp(25, 100, sin(π·T))`, T=(t−DawnEnd)/(DustStart−DawnEnd) — **pik 100 w południe** |
| **Zmierzch** | [DustStart..DustEnd] | `Lerp(25, 0, T)`, T=(t−DustStart)/(DustEnd−DustStart) — liniowo 25→0 |

→ krzywa = **saturating sin z capem**, NIE liniowa, NIE czysty sin. Pik klampowany cap=100.

**MaxSunIntensity = 100, stała:** literał ustawiany RAZ w EventGraph BeginPlay (CDO=0, więc tylko runtime;
0 setterów poza BeginPlay). **Liczba referencji w grafie:** `find_variable_references` zwraca **0 = BUG
narzędzia Monolith** dla tej zmiennej; `search_nodes` pokazuje **27 trafień** (init w EventGraph + obliczenia
w THE ATMOSPHERE + odczyt). Liczba autorytatywna = **27 (search_nodes), nie 0**. (live-verified 2026-06-19.)

**Konsument C++ (źródło prawdy, na dysku dziś):** `MaslowBiologicalComponent.cpp:477-495`
`UpdateDayNightTempOffset()` czyta z zegara przez reflection:
```
SunFactor          = Clamp(SunIntensity / MaxSunIntensity, 0..1)        // :493
DayNightTempOffset = DayNightAmplitude * (SunFactor - 0.5) * 2.0        // :494  → noc −Amp, południe +Amp
```
Guard: brak zegara / MaxSunIntensity≤0 → offset = 0 (mapa bez BP_DayNightCycle).

**🔴 DRIFT do koperty sezonowej (potwierdzony 2026-06-19, NADAL aktualny):**
Komentarz C++ (`:479-481`) twierdzi *"SunFactor niesie SEZON za darmo (zima w południe = niższe
SunIntensity)"* — **GRAF TEMU PRZECZY.** Pik dzienny = `Lerp(25,100,sin(π·0.5))=100` w KAŻDE
południe słoneczne, cały rok (MaxSunIntensity=100 const). → **noon SunFactor=1.0 cały rok → noon offset
≈ +Amp ZAWSZE.** Sezon zmienia TYLKO długość/timing okna dnia (THE CALLENDAR: lato 14.5h ↔ zima 8.0h),
NIE amplitudę piku. "Zimno zimy" = więcej godzin doby w okolicy −Amp, nie niższy pik.
→ Jeśli projekt chce zimniejszych zim w południe, musi to przyjść z: sezonowego `GetZoneBaseTemp`,
sezonowej modulacji `DayNightAmplitude`/`MaxSunIntensity`, albo użycia `MaxElevationSun` zamiast SunIntensity.
**To decyzja architekta — poza ETAP A.**

---

## BLOK 3 — NIGHT FLOOR SUNINTENSITY (worst-case zimny offset)

🔴 **Live read NIEMOŻLIWY dziś (2026-06-23):** `monolith_status` → "Unreal Editor not running";
`mcp-unreal status` → `editor_online:false, plugin_online:false`; brak procesu UnrealEditor w tasklist.
NIE wołano setterów na ślepo (zgodnie z zadaniem).

**Wartość z DYSKU (live-verified 2026-06-19, PIE, BP_DayNightCycle_C_1):**
| prop | wartość |
|---|---|
| **SunIntensity** | **0 (DOKŁADNIE)** ← night floor, zero resztki księżycowej |
| **MaxSunIntensity** | **100** |
| **Pora** | **Night** |
| CurrentTime | 0.4446 (~00:27) |
| SunPitch | −0.375 (pod horyzontem) |

**Skutek (worst-case zimny offset):** SunFactor = 0/100 = 0 → `DayNightTempOffset =
DayNightAmplitude·(0−0.5)·2 = −DayNightAmplitude`. Przy Amp=8: **offset = −8.0 °C**.
W teście doby 2026-06-19: Mountain ambient = −4.0 °C (hipotermia na jawie). Night floor = `−Amp` na dobę.

**Jak zrobić ŚWIEŻY live read (gdy edytor wstanie) — bezpiecznie:**
1. `editor_query pie_get_object_properties` na aktorze `BP_DayNightCycle` w PIE → odczyt
   `SunIntensity`/`CurrentTime`/`Pora` (read-only).
2. Aby DOJŚĆ do nocy: **NIE ustawiać `SunIntensity` ręcznie** (rozjazd z grafem). Przewinąć CZAS
   przez maszynerię zegara (`CurrentTime`/`TimeSpeed` BP) lub poczekać do Pora=Night, potem odczytać.
   Setter na SunIntensity = ryzyko — unikać.

---

## STOP
Trzy bloki zebrane. Zone-table świeży (18 stref, deterministyczny). Sun-curve i night-floor z
live-verified bazy 2026-06-19 + źródła C++ (dziś edytor offline → blok 3 live niewykonalny, fallback
zaznaczony). **Bug #1 (pik sezonowo stały) = decyzja architekta przed implementacją resolvera BaseTemp.**
Nic nie commitowane poza tym raportem.
