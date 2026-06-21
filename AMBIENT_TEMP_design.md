# AMBIENT TEMP — DESIGN GATE (temperatura otoczenia, rdzeń strefowy)
> Zatwierdzony projekt do implementacji (decyzje architekta + Szymona, 2026-06-19).
> ROZSZERZA UMaslowBiologicalComponent + dotyka ACaldrethZone/GetZoneAtLocation (cache stref).
> NIE nowy komponent, NIE Event Tick — wszystko na istniejącym timerze metabolizmu.
>
> KONTEKST: diagnoza cold-burn (ETAP 2) wykazała, że CAŁY podsystem temperatury był MARTWY —
> CurrentTemp=36.6 (temp CIAŁA) ustawiana RAZ w konstruktorze (L48), nic jej nie zmienia.
> Trzy systemy czekają na wejście temperatury OTOCZENIA: spalanie tłuszczu na zimnie (głód),
> priorytet Level_1_Temperature (przetrwanie L1), jakość snu (sen na zimnie = śmierć).
> AmbientTemp dostarcza to wejście → ożywia wszystkie trzy.

═══════════════════════════════════════════════════════════
# DECYZJE PROJEKTOWE (zatwierdzone — NIE zmieniać bez architekta)
═══════════════════════════════════════════════════════════
| # | Decyzja | Wartość |
|---|---|---|
| D-SRC | Źródło temp otoczenia | **Strefa Caldreth (baza) + doba (modyfikator)** — KOMBINACJA |
| D-COUPLE | Otoczenie ↔ ciało | **SPRZĘŻENIE** — otoczenie ciągnie temp ciała ku sobie (hipotermia realna) |
| D-STAGE | Realizacja | **Rdzeń strefowy NAJPIERW, doba jako druga warstwa** (ten etap = TYLKO strefa) |

🔴 ZAKRES TEGO ETAPU: rdzeń strefowy + sprzężenie otoczenie→ciało + cache stref.
   NIE obejmuje: warstwy doby (BP_DayNightCycle offset) — to NASTĘPNY etap (AmbientTemp-Doba).
   Hook na dobę zostawiamy w kodzie (offset = 0 do czasu warstwy doby).

═══════════════════════════════════════════════════════════
# 🔴 ZALEŻNOŚĆ KRYTYCZNA #1 — CACHE STREF (perf, rozwiązać PIERWSZE)
═══════════════════════════════════════════════════════════
PROBLEM: GetZoneAtLocation używa TActorIterator (przeczesuje wszystkich aktorów stref).
Wołane co tick metabolizmu × 500 NPC = zabójcze dla wydajności. To jest "pierwszy konsument",
na który odłożono perf-TODO przy imporcie map (ETAP 5).

ROZWIĄZANIE (do potwierdzenia przez Claude Code wobec realnego ACaldrethZone):
- NIE wołać GetZoneAtLocation co tick per NPC.
- Opcja A (preferowana): NPC cache'uje swoją CurrentZone + odświeża RZADKO — co N ticków
  ALBO gdy przemieścił się o > próg dystansu od ostatniego sprawdzenia (most NPC stoi/pracuje
  w jednym miejscu przez wiele ticków → strefa się nie zmienia).
- Opcja B (jeśli zone-lookup i tak drogie): zbuduj przestrzenny indeks stref RAZ na BeginPlay
  (np. grid/AABB stref), lookup O(1) zamiast O(aktorów). Cięższe, ale skalowalne.
- 🔑 Claude Code: ZMIERZ realny koszt GetZoneAtLocation (ile stref na CaldrethMap, czy TActorIterator
  faktycznie boli przy obecnej liczbie) ZANIM przeprojektujesz. Może opcja A wystarczy.

KOSZT PAMIĘCI (opcja A): 1× TWeakObjectPtr<ACaldrethZone> CurrentZone + 1× FVector LastZoneCheckPos
per NPC = ~24B × 500 = ~12KB. Znikome.

🔴 Bez rozwiązania cache — NIE wpinać temp per-tick. Lepiej etap stanie, niż zabije perf przy 500 NPC.

═══════════════════════════════════════════════════════════
# CZĘŚĆ A — STREFA NIESIE TEMPERATURĘ (FZoneDef.BaseTemp)
═══════════════════════════════════════════════════════════
Każdy biom ma bazową temperaturę otoczenia. Dodajemy pole do FZoneDef (DataTable row, data-driven).

## Nowe pole w FZoneDef (Source/.../Map/, struct row DT_ZoneDefs):
```cpp
/** Bazowa temperatura otoczenia tej strefy (°C). Modyfikowana porą doby (warstwa doby = next etap). */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Caldreth|Climate")
float BaseTemp = 20.0f;
```

## Wartości do DT_ZoneDefs (proponowane — dostroić; zgodne z geologią wyspy wulkanicznej):
| Biom (EZoneType) | BaseTemp °C | Uzasadnienie |
|---|---|---|
| Lava | 80 | lawa — śmiertelnie gorąco |
| Caldera | 45 | krater wulkanu — bardzo gorąco |
| AshSlope | 28 | stok popiołu — ciepły (wulkan grzeje od spodu) |
| Desert | 35 | dzień gorący (doba mocno zmodyfikuje na noc) |
| Savanna | 26 | ciepło |
| Grassland | 20 | umiarkowanie |
| SlopeForest | 18 | las, cień |
| Oasis | 22 | ciepło, wilgotno |
| Beach | 19 | nadmorsko |
| River | 14 | woda chłodzi |
| Mountain | 4 | wysoko, zimno |
| Ocean | 8 | woda — zimna |

🔑 Claude Code: potwierdź realną listę EZoneType (12 biomów wg pamięci) wobec kodu; dostrój jeśli inna.

## Odczyt w komponencie (z cache, NIE co tick raw):
```cpp
float UMaslowBiologicalComponent::GetZoneBaseTemp() const
{
    // używa CACHE'owanej CurrentZone (patrz zależność #1), NIE GetZoneAtLocation co tick
    if (CurrentZone.IsValid())
    {
        return CurrentZone->GetZoneDef().BaseTemp;   // dokładna ścieżka do potwierdzenia
    }
    return DefaultAmbientTemp;   // fallback gdy poza strefą (np. 20°C)
}
```

═══════════════════════════════════════════════════════════
# CZĘŚĆ B — AmbientTemp + HOOK NA DOBĘ
═══════════════════════════════════════════════════════════
```cpp
/** Temperatura OTOCZENIA NPC (°C) — baza strefy + offset doby. Liczona na timerze metabolizmu. */
UPROPERTY(BlueprintReadOnly, Category="Biology|Climate")
float AmbientTemp = 20.0f;

/** Offset doby (°C) — 0 do czasu warstwy doby (next etap). HOOK. */
UPROPERTY(BlueprintReadOnly, Category="Biology|Climate")
float DayNightTempOffset = 0.0f;

/** Fallback temp otoczenia gdy NPC poza wszystkimi strefami. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biology|Climate")
float DefaultAmbientTemp = 20.0f;
```

Aktualizacja w ProcessMetabolism (raz na tick, PRZED użyciem temp w spalaniu):
```cpp
// AmbientTemp = baza strefy + offset doby (offset=0 do warstwy doby)
AmbientTemp = GetZoneBaseTemp() + DayNightTempOffset;
```

═══════════════════════════════════════════════════════════
# CZĘŚĆ C — SPRZĘŻENIE OTOCZENIE → CIAŁO (hipotermia realna)
═══════════════════════════════════════════════════════════
D-COUPLE: otoczenie CIĄGNIE temp ciała ku sobie. Zimne otoczenie → ciało stygnie → hipotermia.
Ciepłe otoczenie / ogień → ciało się ogrzewa z powrotem ku 36.6.

## Model (na timerze metabolizmu):
```cpp
// Ciało dąży do równowagi z otoczeniem, ale POWOLI (termoregulacja opóźnia).
// Współczynnik zależy od izolacji (ubranie/ogień = wolniejsze stygnięcie — HOOK na ekwipunek).
const float TempDelta = AmbientTemp - CurrentTemp;   // ujemny gdy otoczenie zimniejsze niż ciało
const float Rate = (TempDelta < 0.0f) ? BodyCoolingRate : BodyWarmingRate;
CurrentTemp += TempDelta * Rate * InsulationFactor;   // InsulationFactor: 1.0 brak / <1.0 ubranie/ogień
CurrentTemp = FMath::Clamp(CurrentTemp, MinBodyTemp, MaxBodyTemp);   // np. [25, 42]
```

## Nowe pola:
```cpp
/** Tempo stygnięcia ciała ku otoczeniu (część delty/tick). [tune] np. 0.05 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biology|Climate")
float BodyCoolingRate = 0.05f;

/** Tempo ogrzewania ciała ku otoczeniu/ognisku (zwykle szybsze niż stygnięcie — aktywna termoregulacja). [tune] */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biology|Climate")
float BodyWarmingRate = 0.08f;

/** Izolacja: 1.0 = brak (pełne stygnięcie), <1.0 = ubranie/ogień spowalnia. HOOK na ekwipunek/Eureka ogień. */
UPROPERTY(BlueprintReadWrite, Category="Biology|Climate")
float InsulationFactor = 1.0f;

/** Dolny/górny klamp temp ciała (°C). Poniżej dolnego = hipotermia (śmierć), powyżej = hipertermia. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biology|Climate")
float MinBodyTemp = 25.0f;
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biology|Climate")
float MaxBodyTemp = 42.0f;
```

## 🔴 SPRZĘŻENIE ZE SNEM (most temp z ETAP 2/3, teraz AKTYWNY):
We śnie termoregulacja niemal wyłączona → ciało stygnie SZYBCIEJ (brak drżenia, brak aktywnego ogrzewania):
```cpp
const float EffCoolingRate = bIsSleeping ? (BodyCoolingRate * ColdSleepMultiplier) : BodyCoolingRate;
```
- ColdSleepMultiplier (=3.5, już w kodzie z ETAP 2, było DESIGN-ONLY) staje się AKTYWNY.
- Efekt: NPC śpiący w zimnej strefie (Ocean 8°C / Mountain 4°C) bez ognia → ciało spada → hipotermia → śmierć.
- 🛡️ FAIL-SAFE (zasada projektu): hipotermia STOPNIOWA przez kaskadę (CurrentTemp spada przez wiele
  ticków → dopiero przy MinBodyTemp/CriticalTempThreshold śmierć), NIE instant. NPC ma czas się obudzić
  (przyszły hook: auto-wake przy krytycznej temp we śnie?) lub zginąć powoli. Patrz EvaluateCurrentNeed L534.

═══════════════════════════════════════════════════════════
# CZĘŚĆ D — ODBLOKOWANIE MARTWYCH SYSTEMÓW (teraz CurrentTemp żyje)
═══════════════════════════════════════════════════════════
Skoro CurrentTemp (ciało) realnie się zmienia, trzy martwe gałęzie ożywają — SPRAWDŹ każdą:

1. **Spalanie na zimnie** (ProcessMetabolism ~L129): `if (CurrentTemp < 10) → ×2 fat burn`.
   Teraz odpali, gdy ciało wystygnie <10°C. Ale UWAGA: próg 10°C dla CIAŁA to już skrajna hipotermia.
   🔑 ROZWAŻ: spalanie powinno rosnąć, gdy OTOCZENIE jest zimne (organizm grzeje), nie dopiero gdy ciało
   spadło do 10. Czyli ten warunek może powinien czytać AmbientTemp < próg, nie CurrentTemp < 10.
   → DO DECYZJI w implementacji: spalanie zależy od OTOCZENIA (zimno = grzeję = palę tłuszcz), a
   hipotermia (CurrentTemp niskie) to OSOBNY, cięższy skutek. Claude Code: zaproponuj, zatwierdzę.

2. **Priorytet zamarznięcia** (EvaluateCurrentNeed L534): `if (CurrentTemp <= CriticalTempThreshold) → Level_1_Temperature`.
   Teraz odpali, gdy ciało spadnie do krytycznej. To jest poprawne — "zamarzam, ratuj się" = priorytet Maslowa.
   Potwierdź CriticalTempThreshold (wg pamięci 34°C) — to ma sens dla CIAŁA (hipotermia zaczyna się ~35°C).

3. **Jakość snu** (GetTempQualityMultiplier — zahardkodowany 1.0 w ETAP 2):
   ODBLOKUJ zakomentowany kod, ale ZAMIEŃ CurrentTemp → AmbientTemp (komfort 15-24 to OTOCZENIE!):
   ```cpp
   float UMaslowBiologicalComponent::GetTempQualityMultiplier() const
   {
       if (AmbientTemp >= ComfortTempMin && AmbientTemp <= ComfortTempMax) return 1.0f;
       const float Dist = (AmbientTemp < ComfortTempMin) ? (ComfortTempMin - AmbientTemp)
                                                          : (AmbientTemp - ComfortTempMax);
       return FMath::Clamp(1.0f - Dist * 0.075f, 0.25f, 1.0f);
   }
   ```
   To był DOKŁADNIE bug z ETAP 2 (liczył komfort z temp ciała 36.6 → 0.25). Teraz z AmbientTemp = poprawne.

═══════════════════════════════════════════════════════════
# PODZIAŁ C++ / BLUEPRINT
═══════════════════════════════════════════════════════════
## C++ (UMaslowBiologicalComponent + FZoneDef + cache w komponencie):
- FZoneDef.BaseTemp (data-driven, DT_ZoneDefs).
- Cache CurrentZone (rozwiązanie perf #1).
- AmbientTemp = GetZoneBaseTemp() + DayNightTempOffset (offset=0 ten etap).
- Sprzężenie otoczenie→ciało (CurrentTemp dąży do AmbientTemp, sen amplifikuje stygnięcie).
- Odblokowanie 3 systemów (spalanie/priorytet/jakość snu — z AmbientTemp gdzie trzeba).

## Blueprint (BlueprintImplementableEvent — opcjonalne, jeśli chcesz wizual):
- OnHypothermiaWarning() — gdy CurrentTemp spada krytycznie (dreszcze, sine usta — wizual).
- OnBodyTempNormalized() — powrót do komfortu.
(Niewymagane w tym etapie — czysta mechanika wystarczy do verify. Eventy = nice-to-have.)

═══════════════════════════════════════════════════════════
# WARTOŚCI DO MECHANICS (po implementacji)
═══════════════════════════════════════════════════════════
| Zmienna | Wartość proponowana | Status |
|---|---|---|
| FZoneDef.BaseTemp (per biom) | patrz tabela części A | [TBD→tune] |
| BodyCoolingRate | 0.05 | [TBD→tune] |
| BodyWarmingRate | 0.08 | [TBD→tune] |
| MinBodyTemp / MaxBodyTemp | 25 / 42 | [TBD→tune] |
| DefaultAmbientTemp | 20 | [TBD→tune] |
| InsulationFactor (brak izolacji) | 1.0 | [LOCKED, hook ekwipunek] |
| ComfortTempMin/Max (z ETAP 2) | 15 / 24 | [approved, teraz z AmbientTemp] |
| ColdSleepMultiplier (z ETAP 2) | 3.5 | [approved, teraz AKTYWNY] |
| CriticalTempThreshold (istn.) | ~34 | [potwierdzić wobec kodu] |

═══════════════════════════════════════════════════════════
# WERYFIKACJA (Claude Code — twarde dane, ROZBITA na bloki)
═══════════════════════════════════════════════════════════
Po buildzie (edytor zamknięty → UHT), PIE + log z dysku + Monolith live:

BLOK 1 — strefa niesie temp:
  NPC w strefie ciepłej (np. AshSlope 28) → AmbientTemp≈28. NPC w Ocean → AmbientTemp≈8.
  Przenieś NPC między strefami → AmbientTemp się zmienia. Cache działa (nie woła iteratora co tick — sprawdź perf/log).

BLOK 2 — sprzężenie otoczenie→ciało:
  NPC w zimnej strefie (Ocean 8) na jawie → CurrentTemp powoli spada z 36.6 ku ~8 (ale wolno, klamp 25).
  NPC wraca do ciepła → CurrentTemp rośnie z powrotem. Twarde liczby: pokaż trend CurrentTemp przez N ticków.

BLOK 3 — sen na zimnie = śmierć (STOPNIOWO):
  StartSleep() w Ocean 8°C → CurrentTemp spada SZYBCIEJ (×ColdSleepMultiplier) → dochodzi do
  CriticalTempThreshold → Level_1_Temperature priorytet ODPALA (live: EvaluateCurrentNeed) →
  ostatecznie śmierć. 🔴 KLUCZOWE: śmierć STOPNIOWA (wiele ticków), NIE instant. Pokaż timeline temp.

BLOK 4 — jakość snu z AmbientTemp:
  GetTempQualityMultiplier() w komforcie (Grassland 20) = 1.0; w Ocean (8) < 1.0 (nie 0.25-od-ciała-bug).
  StopSleep w komforcie → Rested; StopSleep w zimnie → brak Rested (sen płytki).

BLOK 5 — odblokowane systemy:
  Potwierdź że spalanie na zimnie + priorytet zamarznięcia realnie odpalają (były martwe od początku).
  Live: TemperatureKcalMultiplier > 1 w zimnie (był stały 1), Level_1_Temperature w BB gdy krytycznie.

WSZYSTKO twarde liczby z logu/live, zero placeholderów. Format logu jak w kodzie.

═══════════════════════════════════════════════════════════
# CO TEN ETAP ROBI / CZEGO NIE
═══════════════════════════════════════════════════════════
✅ ROBI: FZoneDef.BaseTemp, cache stref, AmbientTemp (strefa+hook doby), sprzężenie otoczenie→ciało,
   odblokowanie 3 martwych systemów (spalanie/priorytet/jakość snu), aktywacja ColdSleepMultiplier,
   hipotermia stopniowa (fail-safe).
❌ NIE ROBI (NASTĘPNE etapy):
- Warstwa DOBY (BP_DayNightCycle → DayNightTempOffset) → osobny etap AmbientTemp-Doba (hook=0 teraz).
  ⚠️ Wymaga postawienia BP_DayNightCycle na CaldrethMap (nie istnieje — sen też leci fallbackiem).
- Izolacja od ekwipunku/ognia (InsulationFactor z ubrania/ogniska) → hook, gdy ekwipunek dojrzeje.
- Auto-wake przy krytycznej temp we śnie → przyszły hook (teraz: śmierć stopniowa, gracz/BT reaguje).
- Hipertermia (przegrzanie w Lava/Caldera) → mechanizm symetryczny, ale skup się na zimnie najpierw.

═══════════════════════════════════════════════════════════
# 🔑 INSTRUKCJA DLA CLAUDE CODE — POTWIERDŹ PRZED KODOWANIEM
═══════════════════════════════════════════════════════════
Ten DESIGN GATE pisany BEZ dostępu do żywego kodu komponentu/ACaldrethZone. ZANIM kodujesz:
1. Przeczytaj realne: ProcessMetabolism (gdzie temp), EvaluateCurrentNeed (L534), FZoneDef,
   ACaldrethZone::GetZoneAtLocation (jak działa, koszt), GetTempQualityMultiplier (zakomentowany kod z ETAP 2).
2. POTWIERDŹ ścieżki: jak komponent dostanie ACaldrethZone (GetWorld → iterator? subsystem? referencja?),
   dokładna nazwa GetZoneDef/BaseTemp, realna lista EZoneType, realny CriticalTempThreshold.
3. ZMIERZ koszt GetZoneAtLocation — czy opcja A (cache rzadki) wystarczy, czy trzeba indeksu (opcja B).
4. Jeśli coś się nie zgadza z tym docem — FLAGUJ przed kodowaniem (jak przy diagnozie cold-burn), nie zgaduj.
5. Patch, nie regeneracja. Most temp z ETAP 2 (ColdSleepMultiplier) AKTYWUJ, nie pisz od nowa.
