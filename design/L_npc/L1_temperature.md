# L1 — Temperatura (odczuwanie)

**Oś:** L (NPC) · **Warstwa:** L1 · **Status:** ✅ · **Migrowano:** ETAP 2 (2026-07-01). Strzałka-w-dół: L1 zna tylko L0. Single-bridge BB = `BTService_MaslowBlackboardSync`.

> Temperatura otoczenia (BaseTemp/strefa/doba) → [[C3_climate]] (aspekt: świat), czytana przez interfejs.

Strona ciała: sprzężenie ciało ↔ otoczenie. Otoczenie CIĄGNIE temperaturę ciała (CurrentTemp) ku sobie — zimne otoczenie → ciało stygnie → hipotermia realna; ciepłe otoczenie / ogień → ciało ogrzewa się z powrotem ku BodySetpointTemp (36.6). Granica odczytu temperatury otoczenia = `GetFeltTemperature` (interfejs do C3).

## Model sprzężenia (na timerze metabolizmu)

Ciało dąży do równowagi z otoczeniem, ale POWOLI (termoregulacja opóźnia). Trzy reżimy: termoregulacja ku BodySetpointTemp, sen+zimno (amplifikacja stygnięcia), jawa+ekstremalne zimno (hipotermia).

```cpp
const float TempDelta = AmbientTemp - CurrentTemp;   // ujemny gdy otoczenie zimniejsze niż ciało
const float Rate = (TempDelta < 0.0f) ? BodyCoolingRate : BodyWarmingRate;
CurrentTemp += TempDelta * Rate * InsulationFactor;
CurrentTemp = FMath::Clamp(CurrentTemp, MinBodyTemp, MaxBodyTemp);   // np. [25, 42]
```

Pola:
- `BodyCoolingRate = 0.05f` — tempo stygnięcia ciała ku otoczeniu (część delty/tick).
- `BodyWarmingRate = 0.08f` — tempo ogrzewania (zwykle szybsze niż stygnięcie — aktywna termoregulacja).
- `MinBodyTemp = 25.0f` / `MaxBodyTemp = 42.0f` — klamp temp ciała; poniżej dolnego = hipotermia (śmierć), powyżej = hipertermia.

## Izolacja (InsulationFactor)

`InsulationFactor` (1.0 = brak, pełne stygnięcie; <1.0 = ubranie/ogień spowalnia). HOOK na ekwipunek (fat + clothing + fire). Wchodzi wprost w model sprzężenia jako mnożnik tempa. Fat→izolacja liczony w [[L1_hunger]] (jeden writer).

```cpp
UPROPERTY(BlueprintReadWrite, Category="Biology|Climate")
float InsulationFactor = 1.0f;   // HOOK: ekwipunek/Eureka ogień
```

## Sprzężenie ze snem + hipotermia stopniowa

We śnie termoregulacja niemal wyłączona → ciało stygnie SZYBCIEJ (brak drżenia, brak aktywnego ogrzewania):

```cpp
const float EffCoolingRate = bIsSleeping ? (BodyCoolingRate * ColdSleepMultiplier) : BodyCoolingRate;
```

- `ColdSleepMultiplier` (=3.5) staje się AKTYWNY. NPC śpiący w zimnej strefie (Ocean 8°C / Mountain 4°C) bez ognia → ciało spada → hipotermia → śmierć.
- 🛡️ FAIL-SAFE: hipotermia STOPNIOWA przez kaskadę (CurrentTemp spada przez wiele ticków → dopiero przy `MinBodyTemp` / `CriticalTempThreshold` śmierć), NIE instant. NPC ma czas się obudzić lub zginąć powoli.

## Odblokowanie martwych systemów (CurrentTemp realnie żyje)

1. **Spalanie na zimnie** (cold-burn): ×2 fat burn. DECYZJA: warunek powinien czytać `AmbientTemp < próg` (zimno otoczenia = organizm grzeje = pali tłuszcz), NIE `CurrentTemp < 10` (to już skrajna hipotermia). Hipotermia = OSOBNY, cięższy skutek.
2. **Priorytet zamarznięcia** (EvaluateCurrentNeed): `if (CurrentTemp <= CriticalTempThreshold) → Level_1_Temperature`. `CriticalTempThreshold` ~34°C (hipotermia zaczyna się ~35°C dla ciała).
3. **Jakość snu** (`GetTempQualityMultiplier`) — komfort 15-24 to OTOCZENIE, więc czyta `AmbientTemp` (nie CurrentTemp — to był bug z ETAP 2: komfort z temp ciała 36.6 → 0.25):

```cpp
float UMaslowBiologicalComponent::GetTempQualityMultiplier() const
{
    if (AmbientTemp >= ComfortTempMin && AmbientTemp <= ComfortTempMax) return 1.0f;
    const float Dist = (AmbientTemp < ComfortTempMin) ? (ComfortTempMin - AmbientTemp)
                                                       : (AmbientTemp - ComfortTempMax);
    return FMath::Clamp(1.0f - Dist * 0.075f, 0.25f, 1.0f);
}
```

Wartości powiązane: `ComfortTempMin/Max` = 15/24, `ColdSleepMultiplier` = 3.5 (teraz AKTYWNY), `CriticalTempThreshold` ~34 (potwierdzić wobec kodu). Jakość snu konsumowana przez [[L1_sleep]].

**Źródło:** AMBIENT_TEMP_design.md (aspekt: odczuwanie/ciało) (Game_git)
