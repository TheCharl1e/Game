# Raport — AmbientTemp ETAP A: RECON · 2026-06-19

> RECON only. Zero kodu produkcyjnego: nie dodano pól do FZoneDef, nie napisano
> resolvera, nie ruszono nagłówków. Nic nie commitowane.

## Metoda
1. **Zone table** — `python Gra_Stan_Pierwotny/Scripts/zone_table_from_npz.py`
   replikuje flood-fill z `CaldrethImportLibrary.cpp` (4-connected, MinRegionPixels=8,
   bSkipOcean=false, skip biome>11) na `MapData/caldreth_data.npz`. Daje **dokładnie 18 stref**
   (zgodne z ETAP 5 "zweryfikowane 18 stref"). Elewacja z `npz['elevation']` (float32, 0..1).
2. **Sun curve** — inwentaryzacja zmiennych + node'ów z `Content/DocelowaGra/WORLD/BP_DayNightCycle.uasset`
   (string scan). Dokładne wiring tylko przez live `blueprint_query get_graph_data` (edytor offline).
3. **Night floor** — Monolith MCP offline (edytor nie działa). Użyto wartości z DYSKU z
   poprzedniego zweryfikowanego raportu (`raport_2026-06-19_warstwa_doby.md`).

---

## BLOK 1 — ZONE TABLE (18 stref, z caldreth_data.npz)

Elewacja **znormalizowana 0..1** (== `elev/elev.max()` z mapgen). `sea_level=0.200`.
Grid 512×512 → area = liczba komórek (1 komórka ≈ (WorldSizeUU/512)² uu; WorldSizeUU=1e6 → ~1953 uu/komórka).

| id | biome        | area (komórki) | e_mean | e_min | e_max | cx    | cy    |
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

Per-biome: OCEAN×2, BEACH×2, SAVANNA×3, DESERT×2, GRASSLAND×2, SLOPE_FOREST×1,
MOUNTAIN×1, ASH_SLOPE×1, CALDERA×1, RIVER×1, LAVA×1, OASIS×1 = **18**.

⚠️ **Uwaga do lapse-rate:** plik NIE ma metrów — elewacja jest 0..1 (ułamek wysokości szczytu).
Do BaseTemp per strefa = biome_base + lapse(elev) trzeba przyjąć skalar "wysokość szczytu w m".
⚠️ Strefy o tym samym biomie mają RÓŻNE id (osobne komponenty spójne) — np. CALDERA to id 9
(e_mean 0.963), MOUNTAIN id 7 (0.767), ASH_SLOPE id 8 (0.885). To one będą najzimniejsze po lapse.
⚠️ Centroid AABB-overlap (Mountain centroid wpada w mniejszą AshSlope) — znany artefakt z ETAP doby.

---

## BLOK 2 — SUNINTENSITY CURVE

**Plik:** `/Game/DocelowaGra/WORLD/BP_DayNightCycle` (`Content/DocelowaGra/WORLD/BP_DayNightCycle.uasset`)
— to JEDYNY żywy zegar na mapie (drugi, `AI_NPC/BP_DayNightCycle`, jest martwy: 0 ref mapy).
**Funkcje grafu:** `THE SUN` i `TIME` (entry pointy: `THE SUN.K2Node_FunctionEntry_0`, `TIME.K2Node_FunctionEntry_0`).

**Zmienne związane ze słońcem (z uasset):** `SunPitch`, `SunIntensity`, `MaxSunIntensity`,
`MaxElevationSun`, `SunYaw`, `SunRise`, `SunSet`, `SunRiseNormalized`, `SunSetNormalized`,
`SunYawAtSunRise`, `SunYawAtSunSet`, `TimeNormalized`, `DayLenght`, `DayOfYear`, `CurrentTime`,
`Pora`, `Season`/`Seasons`, `DawnStart`, `DawnEnd`. (Księżyc: `MoonPitch/MoonIntensity/MaxMoonIntensity/MaxElevationMoon`.)

**Node'y matematyczne obecne w grafie:** `CallFunc_Sin_ReturnValue`, `CallFunc_Cos_ReturnValue`,
`CallFunc_Lerp_ReturnValue`, `ClampMin`, `ClampMax`, `SetIntensity` (`SetIntensity_NewIntensity`),
`MakeRotator_Pitch` (SunPitch → rotacja DirectionalLight). → krzywa = **sin/clamp**, nie liniowa.

**Model POTWIERDZONY (C++ + zweryfikowany runtime, NIE inferencja):**
- `SunFactor = Clamp(SunIntensity / MaxSunIntensity, 0..1)` — `MaslowBiologicalComponent.cpp:466`.
- **MaxSunIntensity = 100**, stała "cap" (raport warstwy doby: "MaxSunIntensity=100 const").
- Noc: **SunIntensity = 0.0 DOKŁADNIE** przy Pora=Night (zero resztki księżycowej) → SunFactor=0.
- Południe (live): SunFactor ≈ 0.917 (offset=+7.33 przy Amp=8 → 7.33 = 8·(0.917−0.5)·2 → SunFactor≈0.958; w raporcie +7.33).
- SunIntensity idzie za REALNĄ elewacją słońca; `MaxElevationSun` zależy od sezonu (`DayOfYear`)
  → niższy łuk zimą → niższy sin → niższe SunIntensity w południe = "sezon za darmo".

**Kształt (inferencja z node'ów, do potwierdzenia live):**
`SunIntensity ≈ MaxSunIntensity · ClampMin( sin(elewacja słońca), 0 )`, gdzie elewacja rośnie/maleje
między `SunRiseNormalized`..`SunSetNormalized` z pikiem ograniczonym przez `MaxElevationSun` (sezonowy).

🔴 **Dokładne wiring wzoru = live `blueprint_query get_graph_data` na funkcji "THE SUN"** (edytor offline teraz).
🔴 **"Ile referencji MaxSunIntensity w grafie"** — niepoliczalne ze string-scanu; wymaga live
`blueprint_query find_variable_references {var: MaxSunIntensity}`. Potwierdzone: jest stałą cap = 100,
czytaną przez C++ i użytą w SetIntensity słońca.
(DESIGN_how_it_works.md §"Słońce sezonowe" opisuje ASPIRACYJNY model astronomiczny
δ≈23.44°·sin(360°·(dzień−81)/365) — oznaczony "🔨 do przebudowy", to NIE jest pewne że to obecny graf.)

---

## BLOK 3 — NIGHT FLOOR SUNINTENSITY (worst-case zimny offset)

🔴 **Live read NIEMOŻLIWY teraz:** `monolith_status` → "Unreal Editor not running"
(Monolith MCP offline, port niedostępny). NIE wołałem setterów na ślepo (zgodnie z zadaniem).

**Wartość z DYSKU (poprzedni zweryfikowany runtime, `raport_2026-06-19_warstwa_doby.md`, test "night→0"):**
- **SunIntensity = 0.0 DOKŁADNIE** (zero resztki księżycowej)
- **Pora = Night**
- Skutek: SunFactor = 0 → `DayNightTempOffset = 8·(0−0.5)·2 = −8.0` °C (worst-case zimny offset)
- Live AmbientTemp w tym teście: AshSlope 28 − 8 = **20**; Mountain ambient = **−4.0** → hipotermia na jawie.

**Jak zrobić świeży live read (gdy edytor wstanie):**
1. `editor_query pie_get_object_properties` na aktorze `BP_DayNightCycle` w PIE →
   odczyt `SunIntensity`, `CurrentTime`, `Pora` (read-only, bezpieczne).
2. Aby DOJŚĆ do nocy: NIE ustawiać `SunIntensity` ręcznie. Zamiast tego przewinąć CZAS
   przez maszynerię zegara (`TimeSpeed`/`CurrentTime` BP) albo poczekać do Pora=Night,
   a potem odczytać `SunIntensity` jak wyżej. Setter na SunIntensity = ryzyko (rozjazd z grafem) — unikać.

---

---

## AKTUALIZACJA LIVE (edytor + PIE, Monolith port 9316) — 2026-06-19

Edytor wstał → dociągnięto wszystkie trzy live'y. Wzory z grafu (nie inferencja).

### Wzór SunIntensity — POPRAWIONY (był błędnie przypisany do "THE SUN")
- **`THE SUN`** liczy TYLKO geometrię słońca (obrót DirectionalLight), NIE intensywność:
  - `T2_sun = (CurrentTime − DawnStart) / (DustEnd − DawnStart)`
  - `SunPitch = −sin(T2_sun · π) · MaxElevationSun`
  - `SunYaw = Lerp(SunYawAtSunRise, SunYawAtSunSet, T2_sun)`
- **`THE ATMOSPHERE`** liczy SunIntensity w 4 fazach doby (cap = `MaxSunIntensity` = **100**):
  - **Noc:** `SunIntensity = 0` (literał) ← night floor
  - **Świt** [DawnStart..DawnEnd]: `Lerp(0, 25, T)`, T=(t−DawnStart)/(DawnEnd−DawnStart) → 0→25 liniowo
  - **Dzień** [DawnEnd..DustStart]: `Lerp(25, 100, sin(π·T))`, T=(t−DawnEnd)/(DustStart−DawnEnd) → **pik 100 w południe słoneczne**
  - **Zmierzch** [DustStart..DustEnd]: `Lerp(25, 0, T)`, T=(t−DustStart)/(DustEnd−DustStart) → 25→0 liniowo
- `MaxSunIntensity` = **100**, literał ustawiany RAZ w EventGraph BeginPlay (CDO=0, więc tylko runtime).
  `find_variable_references` zwraca 0 ref dla SunIntensity i MaxSunIntensity = **BŁĄD narzędzia** — `search_nodes`
  pokazuje 27 trafień (init w EventGraph, obliczenia w THE ATMOSPHERE). Używać search_nodes, nie find_variable_references.

### Live PIE (BP_DayNightCycle_C_1, odczyt obiektu)
| prop | wartość |
|---|---|
| CurrentTime | 0.4446 (~00:27) |
| **Pora** | **Night** |
| **SunIntensity** | **0** (DOKŁADNIE) ← night floor potwierdzony live |
| **MaxSunIntensity** | **100** ← potwierdzony live |
| SunPitch | −0.375 (pod horyzontem) |
| MaxElevationSun | 37.5 (DayOfYear=102, Season=NewEnumerator2) |
| DawnStart / DawnEnd | 6.75 / 7.25 |
| DustStart / DustEnd | 18.5 / 19.0 |
| DayLenght | 11.75 |
| TimeSpeed | 0.16667 |

EventGraph BeginPlay literały: TIME_(X)=M=6.0 → TimeSpeed=24/(6·24)=0.1667; CurrentTime init=10.0;
MaxSunIntensity=100; MaxMoonIntensity=2.0; MaxElevationMoon=28.0; SkyInesityDay=2.0; SkyInesityNight=0.05; DayOfYear init=100.

### 🔴 DRIFT — koperta sezonowa SunIntensity jest INNA niż zakłada projekt
Komentarz C++ (`MaslowBiologicalComponent.cpp:453-454`) i MECHANICS twierdzą: *"SunFactor niesie SEZON za
darmo (zima w południe = niższe SunIntensity)"*. **Graf temu przeczy:**
- Pik dzienny = `Lerp(25, 100, sin(π·0.5)) = 100` w KAŻDE południe słoneczne, niezależnie od sezonu.
  → **noon SunFactor = 1.0 cały rok** → noon DayNightTempOffset ≈ +Amp ZAWSZE.
- `MaxElevationSun` (sezonowy, tu 37.5) wpływa TYLKO na SunPitch (wizualny łuk słońca w THE SUN),
  **NIE na magnitudę SunIntensity**. MaxSunIntensity stoi na 100 cały rok (0 setterów poza BeginPlay).
- Sezon zmienia DŁUGOŚĆ DNIA (DawnStart/DawnEnd/DustStart/DustEnd ustawiane sezonowo w `THE CALLENDAR`)
  → przesuwa OKNO ciepła i jego długość, NIE amplitudę piku.
- Wniosek dla AmbientTemp: offset doby waha się −Amp (noc) ↔ +Amp (południe) z amplitudą ~stałą sezonowo;
  sygnał sezonowy jest w DŁUGOŚCI/timing okna ciepła, nie w niższym piku zimą.
- Jeśli projekt chce "zimniejszych zim w południe", musi to przyjść SKĄDINĄD: sezonowy GetZoneBaseTemp,
  sezonowa modulacja DayNightAmplitude/MaxSunIntensity, albo użycie MaxElevationSun zamiast SunIntensity.
- (Wartości +7.33 z poprzedniego raportu = pomiar tuż obok południa słonecznego, nie efekt sezonu.)

### Do koperty sezonowej dnia (jeśli potrzebne)
Sezonowy harmonogram DawnStart/DawnEnd/DustStart/DustEnd + MaxElevationSun(DayOfYear) siedzi w funkcji
`THE CALLENDAR` (146 węzłów) — nie wyciągnięto w tym etapie (duże). Tam jest krzywa długości dnia per sezon.

---

## THE CALLENDAR — sezonowy harmonogram dnia (2026-06-19)

Rok = **364 dni**, 4 sezony wybierane progami DayOfYear (łańcuch 3× Branch `DayOfYear < X`).
Każdy sezon miał interpolować harmonogram `Lerp(A, B, alpha=T)`, gdzie `T = (DayOfYear − start)/długość`.

### 🔴 BUG #2 — interpolacja sezonowa MARTWA (dzielenie całkowite)
`Set T` używa **`Divide_IntInt`** (int/int), wynik konwertowany `To Float(Integer)` → podawany jako alpha Lerpów.
Ponieważ licznik (0..długość−1) < mianownik przez CAŁY sezon, `T = 0` zawsze → **każdy Lerp zwraca A**
(wartość początku sezonu). Endpoint B nie jest nigdy osiągany. Efekt: harmonogram to **4 stałe stopnie**
z twardymi skokami na granicach DoY (84 / 168 / 252 / 0), zero płynnego przejścia.
Potwierdzenie live: DoY=102 (Enum2) → dokładnie wartości A (MaxElevationSun=37.5, DawnStart=6.75,
DustStart=18.5, DayLenght=11.75). Gdyby T był float, byłoby ~42.0 / 6.16 / ... — nie jest.

### Efektywny harmonogram (wartości A — to co realnie działa)
DoY-order roku: Enum1 → Enum2 → Enum3 → Enum0 (potem zawija). `DayLenght = DustStart − DawnStart`.

| Sezon (enum) | DoY | MaxElevSun | DawnStart | DawnEnd | DustStart | DustEnd | DayLenght | YawRise | YawSet | MaxElevMoon |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Enum1 (wiosna?)   | 0–83    | 14.5 | 6.75 | 7.25 | 18.5 | 19.0 | **11.75** | 103 | 257 | 25 |
| Enum2 (lato wcz.?)| 84–167  | 37.5 | 6.75 | 7.25 | 18.5 | 19.0 | **11.75** | 54  | 306 | 35 |
| Enum3 (LATO peak) | 168–251 | 61.2 | 5.5  | 6.0  | 20.0 | 20.5 | **14.5**  | 75  | 285 | 25 |
| Enum0 (ZIMA)      | 252–363 | 37.5 | 7.5  | 8.0  | 15.5 | 16.0 | **8.0**   | 124 | 236 | 15 |

Intencja projektanta (endpointy B, NIEaktywne przez bug): Enum1 DawnStart 6.75→4.0, DustStart 18.5→21.0
(dzień miał rosnąć do ~17h w pełni lata), MaxElevationSun 14.5→37.5→61.2→37.5→14.5 (płynna sinusoida elewacji).

### Skutek dla AmbientTemp — koperta sezonowa
Łącząc z THE ATMOSPHERE (pik SunIntensity = 100 co dzień):
- **Amplituda offsetu doby = ±DayNightAmplitude niezależnie od sezonu** (noon SunFactor=1.0, noc=0 cały rok).
- Jedyny realny sygnał sezonowy = **długość dnia** (szerokość okna ciepła): **lato 14.5 h ↔ zima 8.0 h**
  (z twardymi skokami między 4 sezonami, bez płynnego przejścia — bug #2).
- „Zimno zimy" = WIĘCEJ godzin w okolicy −Amp na dobę (zima ~16 h ciemna, lato ~9.5 h), NIE niższy pik.
- Do analitycznej koperty: `DayNightTempOffset(t) = Amp·(2·SunFactor(t) − 1)`, gdzie SunFactor(t):
  0 (noc) → ramp liniowy (świt) → `Lerp(0.25, 1.0, sin(π·T_day))` (dzień, pik 1.0) → ramp (zmierzch) → 0,
  a granice świtu/zmierzchu bierzesz z tabeli per sezon. Średnia dobowa offsetu jest funkcją DayLenght.

🔴 **Dwa bugi do decyzji architekta (NIE naprawiam w recon):**
1. SunIntensity peak sezonowo stały (MaxSunIntensity=100 const) — „sezon za darmo" nie istnieje.
2. THE CALLENDAR `Divide_IntInt` zabija płynną interpolację sezonową (4 skokowe stopnie).
Oba dotyczą tego, jak/czy AmbientTemp ma nieść sezon. Wymaga gate'u zanim cokolwiek ruszę.

---

## FIX BUG #2 — przywrócona interpolacja sezonowa (2026-06-19, na polecenie usera)

**Przyczyna:** `Set T = Divide_IntInt` (int/int) + zmienna `T` typu int32 → T=0 przez cały sezon → wszystkie
sezonowe Lerpy zwracały endpoint A. Naprawa „zmień na float" wymagała ominięcia int-owego `T` (zmiana typu
zmiennej rozsypałaby auto-casty), więc zastosowano **bypass**.

**Co zrobiono (Monolith, graf THE CALLENDAR, BP_DayNightCycle WORLD):**
- Dodano 4 węzły `Divide_DoubleDouble` (po jednym na sezon): CF_28, CF_29, CF_30, CF_31.
  - CF_28 = DayOfYear / 84.0 (sezon Enum1)
  - CF_29 = (DayOfYear−84) / 84.0 (Enum2)
  - CF_30 = (DayOfYear−168) / 84.0 (Enum3)
  - CF_31 = (DayOfYear−252) / 112.0 (Enum0)
  - Numerator int → A: Monolith auto-wstawił `Conv_IntToDouble` (poprawne dzielenie zmiennoprzecinkowe).
- Przepięto 22 połączenia: alfy wszystkich sezonowych Lerpów (MaxElevationSun, SunYaw) + węzły `MultiplyByPi`
  (CF_23, CF_2 → karmią `Sin` dla krzywej długości dnia) na nowe dzielenia float.
- Stary łańcuch (Divide_IntInt → Set T(int) → Conv_IntToDouble) pozostał martwy/nieszkodliwy (`T` teraz
  vestigial; kompilator pomija osierocone Conv). NIE zmieniano typu zmiennej T.
- `compile_blueprint` → **UpToDate, 0 błędów, 0 ostrzeżeń**.

**Odkrycie przy okazji:** harmonogram NIE jest jednolicie liniowy — długość dnia (DawnStart/DustStart) używa
`Lerp(A, B, sin(π·T))` (gładka krzywa przesilenia, pik w środku sezonu), a elewacja/yaw używa liniowego `T`.
Oba teraz płyną z naprawionego float-T. (Dodatkowo: oryginał był NIESPÓJNY — sezony Enum1/Enum3 miały
DawnStart/DustStart na liniowym T, Enum2/Enum0 na sin(π·T). Po naprawie wszystkie działają, choć krzywa
różni się typem między sezonami — do ewentualnego ujednolicenia osobno.)

**Live verify (PIE, DoY=100 = Enum2, T=0.19):**
| var | przed fixem | po fixie | oczekiwane |
|---|---|---|---|
| MaxElevationSun | 37.5 | **42.01** | Lerp(37.5,61.2,0.19)=42.01 ✓ |
| DawnStart | 6.75 | **5.20** | Lerp(6.75,4.0,sin(π·0.19)=0.562)=5.20 ✓ |
| DustStart | 18.5 | **19.91** | Lerp(18.5,21.0,0.562)=19.91 ✓ |
| DayLenght | 11.75 | **14.71** | 19.91−5.20 ✓ |
| SunYawAtSunRise | 54.0 (≈) | **57.81** | Lerp(54,74,0.19)=57.8 ✓ |

**Status zapisu:** ⏳ zmiana skompilowana w PAMIĘCI edytora i zweryfikowana w PIE, ale **NIEZAPISANA na dysk**.
Zgodnie z [[no-monolith-save-for-bp-defaults]] zapis BP robi user ręcznie (Ctrl+S) — NIE przez Monolith
(ryzyko RF_Transient ref-strip). Asset jest czysty w git → rollback = `git checkout` gdyby coś.

**Skutek dla AmbientTemp:** day-length sezonowy teraz realnie się zmienia (lato↔zima płynnie, nie 4 skoki).
Bug #1 (SunIntensity pik=100 cały rok) NADAL aktualny — to osobna decyzja.

## STOP
Bug #2 naprawiony w pamięci + live-verified. Czeka na ręczny zapis usera. Brak commita.
