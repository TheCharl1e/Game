# C3 — Klimat (temperatura — strona świata)

**Oś:** C (świat) · **Warstwa:** C3 · **Status:** ✅ · **Migrowano:** ETAP 2 (2026-07-01). Import: C3 zna tylko C0-C1 (czas, geografia).

> Odczuwanie temperatury przez ciało → [[L1_temperature]] (aspekt: odczuwanie).

Świat wystawia temperaturę OTOCZENIA, którą ciało NPC czyta przez interfejs. Źródło temperatury = **strefa Caldreth (baza) + doba (modyfikator)**. Ten etap realizuje TYLKO rdzeń strefowy; warstwa doby to następny etap (offset = 0 do tego czasu).

## AmbientTemp = BaseTemp(strefa) + offset doby

Temperatura otoczenia NPC (°C) liczona na timerze metabolizmu (nie Event Tick), PRZED użyciem w spalaniu:

```cpp
// AmbientTemp = baza strefy + offset doby (offset=0 do warstwy doby)
AmbientTemp = GetZoneBaseTemp() + DayNightTempOffset;
```

Pola (na UMaslowBiologicalComponent):
- `AmbientTemp = 20.0f` — temperatura otoczenia NPC (°C), BlueprintReadOnly.
- `DayNightTempOffset = 0.0f` — offset doby (°C); HOOK, 0 do czasu warstwy doby (SunFactor / BP_DayNightCycle → [[C0_clocks]]).
- `DefaultAmbientTemp = 20.0f` — fallback, gdy NPC poza wszystkimi strefami.

## BaseTemp per biom (FZoneDef, data-driven — DT_ZoneDefs)

Każdy biom niesie bazową temperaturę otoczenia. Nowe pole w FZoneDef (struct row DT_ZoneDefs):

```cpp
/** Bazowa temperatura otoczenia tej strefy (°C). Modyfikowana porą doby (warstwa doby = next etap). */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Caldreth|Climate")
float BaseTemp = 20.0f;
```

Wartości (12 biomów EZoneType, zgodne z geologią wyspy wulkanicznej):

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

## Cache strefy (perf — rozwiązać PIERWSZE)

`GetZoneAtLocation` używa TActorIterator (przeczesuje wszystkich aktorów stref). Wołane co tick × 500 NPC = zabójcze. NIE wołać co tick raw.

- Opcja A (preferowana): NPC cache'uje `CurrentZone` (TWeakObjectPtr<ACaldrethZone>) + odświeża RZADKO — co N ticków ALBO gdy przemieścił się o > próg dystansu (ZoneRecheckDistance) od `LastZoneCheckPos` (most NPC stoi/pracuje w jednym miejscu → strefa się nie zmienia). RefreshZoneCache odświeża cache.
- Opcja B (jeśli lookup i tak drogi): przestrzenny indeks stref RAZ na BeginPlay (grid/AABB), lookup O(1) zamiast O(aktorów). Cięższe, ale skalowalne.
- Koszt pamięci (A): 1× TWeakObjectPtr<ACaldrethZone> + 1× FVector LastZoneCheckPos per NPC ≈ 24B × 500 ≈ 12KB. Znikome.

Odczyt bazy strefy z cache (NIE co tick raw):

```cpp
float UMaslowBiologicalComponent::GetZoneBaseTemp() const
{
    if (CurrentZone.IsValid())
    {
        return CurrentZone->GetZoneDef().BaseTemp;   // ścieżka do potwierdzenia
    }
    return DefaultAmbientTemp;   // fallback poza strefą (np. 20°C)
}
```

🔴 Bez rozwiązania cache — NIE wpinać temp per-tick. Lepiej etap stanie, niż zabije perf przy 500 NPC.

**Źródło:** AMBIENT_TEMP_design.md (aspekt: świat) (Game_git)
