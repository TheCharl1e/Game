# Raport — Warstwa doby (DayNightTempOffset) · 2026-06-19

## Cel
Ożywić ostatni „hook" rdzenia AmbientTemp: `DayNightTempOffset` był stałym 0.
Doprowadzić go do liczenia z zegara świata (BP_DayNightCycle) tak, by temperatura
otoczenia miała cykl dobowy **i** sezonowy — i by ekstremum dobowe (zimna noc w
górach) realnie odpalało hipotermię na jawie (regime 2 z etapu AmbientTemp).

## Co zrobiono
**Kod** (zacommitowany przez usera w `8dba663 "Day1ofgit"`: .h +10, .cpp +45;
ten etap = weryfikacja + dokumentacja):
- `UpdateDayNightTempOffset()` — woła `ProcessMetabolism` co tick PRZED złożeniem
  `AmbientTemp = GetZoneBaseTemp() + DayNightTempOffset`.
- Reflection helper `ReadActorDouble(AActor*, const TCHAR* PropName, double&)`
  (FDoubleProperty/FFloatProperty — BP „real" = double w UE5).
- `WorldClock` (TWeakObjectPtr<AActor>) — cache'owany RAZ w
  `ResolveAwakeRateFromWorldClock()` → **JEDEN zegar** wspólny dla snu (ETAP 1) i temp.
- Model proporcjonalny (zatwierdzony):
  `SunFactor = Clamp(SunIntensity / MaxSunIntensity, 0..1)`,
  `DayNightTempOffset = DayNightAmplitude × (SunFactor − 0.5) × 2` → noc −8, południe +8.

**Decyzja źródła (zatwierdzona):** `SunIntensity` (nie SunPitch/TimeNormalized) —
niesie SEZON ZA DARMO. MaxSunIntensity to stała cap (=100); SunIntensity podąża za
realną elewacją słońca → zima w południe = niższe SunIntensity = chłodniej.
Brak zegara → offset 0 (mapa bez BP_DayNightCycle).

## Zmienne / wartości
- `DayNightAmplitude = 8.0` (UPROPERTY, **[TBD→tune]** — amplituda °C: noc −8, południe +8)
- `DayNightTempOffset` — teraz LICZONY (był stały 0)
- `WorldClock` — cache zegara

## Build
PEŁNY UHT BUILD ZIELONY (DayNightAmplitude UPROPERTY, 68.2s, 2 pliki UHT, oba DLL).
Edytor zamknięty na czas builda.

## Runtime verify (twarde liczby — Monolith pie_get_object_properties + log na dysku, zero placeholderów)
| Test | Wynik |
|---|---|
| **night→0** | SunIntensity=0 DOKŁADNIE (Pora=Night) → ZERO resztki księżycowej → offset=−8, AmbientTemp=20 (AshSlope 28−8) |
| **południe** | offset=+7.33, AmbientTemp=35.33 (AshSlope 28+7.33), CurrentTemp=36.6 (regime 3) |
| **Mountain noc jawa (regime 2)** | teleport do czystego punktu Mountain (-140000,-140000) → `[Temp] HIPOTERMIA (CurrentTemp=32.79≤34.0, ambient=−4.0, śpi=0) — Level_1_Temperature`, CurrentTemp 36.6→32.79→29.57 na JAWIE |
| **cykl real-time** | offset −8 (noc) → +2.97 (świt) gdy słońce wstało; AmbientTemp Mountain wróciło 6.97 → NPC zaczął się odgrzewać (regime 3) |
| **sleep one-clock** | bez regresji: `[Sleep] AwakeRatePerTick dostrojony do zegara 'BP_DayNightCycle_C_1'` (dostrojony:1 / Brak zegara:0 — nie fallback) |

## Ustalenia
- BP_DayNightCycle stoi na CaldrethMap (WORLD/), MaxSunIntensity=100 const.
- **AABB-overlap stref** (Mountain centroid → wygrywa mniejsza AshSlope) = artefakt
  danych mapy. Obejście do testów: GetZoneTypeAtLocation grid-scan znalazł **31
  czystych punktów Mountain** → użyto (-140000,-140000). Do Moore-trace outline (perf bundle).
- Duplikat `AI_NPC/BP_DayNightCycle` = martwy (0 ref mapy) — do usunięcia (NIE na ślepo).

## Pliki
- Source/Stan_Pierwotny/MaslowBiologicalComponent.{h,cpp} (kod: 8dba663)
- Gra_Stan_Pierwotny/MECHANICS.md, CODE_REGISTRY.md, CHANGELOG.md, ROADMAP.md (L1-09 → 🔨)
- SLEEP_ENGINE_design.md

## Co dalej
- **Clean (pełny) rebuild na koniec** (user: „później jak to zakończymy to clean").
- L1-09 zostaje: fire (heat source) + clothing = Inventory equip izolacja (InsulationFactor jest, niewykorzystany).
- Usunąć martwy duplikat AI_NPC/BP_DayNightCycle.
- Tuning DayNightAmplitude + per-zone amplitude (przyszła notatka, NIE teraz).
