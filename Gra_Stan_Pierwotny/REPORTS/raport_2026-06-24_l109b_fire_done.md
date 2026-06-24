# Raport — L1-09b fire / źródło ciepła DONE + PIE-VERIFIED · 2026-06-24

> Falloff `(1−d/r)²` (huddle), Warmth=25, Radius=600, suma z clampem ≤+40 (decyzje dyrektora).
> Build edytor-zamknięty (Succeeded). Branch `feat/l1-09b-fire`.

## Architektura (wzorzec rejestru jak L3-01, perf ×500)
- **`UHeatSourceRegistrySubsystem` (UWorldSubsystem):** `RegisterSource`/`UnregisterSource` + `GetRadiantHeatAt(loc)`
  = Σ `Warmth × (1−d/Radius)²` po zapalonych źródłach w zasięgu, **clamp ≤ `MaxRadiantHeat=40`**. Zero tick.
- **`UHeatSourceComponent`** (Warmth=25, Radius=600, `bIsLit`): rejestruje się w BeginPlay, wyrejestrowuje w EndPlay
  (jak `NPCIdentityComponent`). Placeable na dowolnym BP.
- **Maslow:** co kadencję `AmbientTemp += GetRadiantHeatAt(ownerLoc)` — ogień podnosi LOKALNY AmbientTemp,
  istniejące reżimy termoregulacji robią resztę (zimny NPC przy ogniu → AmbientTemp w górę → grzeje się). Zero nowego reżimu.
- **Perf:** ogniska rzadkie (~10-50/wioska) → tani odczyt ×500, bez spatial-grid (flaga: dodać gdyby źródeł były tysiące).

## Commity / build / assety
- C++: **`4a9ef0e`** (`Temperature/HeatSourceRegistrySubsystem.{h,cpp}` + `HeatSourceComponent.{h,cpp}` + Maslow). Build Succeeded.
- **`BP_HeatSource`** (Actor + HeatSourceComponent) utworzony do testu/placementu — zapisany ścieżką silnikową (nowy asset, bez RF_Transient defaultów).

## PIE-VERIFY (izolacja RadiantHeat przez DWA NPC: C_1 przy ogniu, C_2 daleko, ta sama strefa → `C_1−C_2` = czysty RadiantHeat)
Parytet bazy potwierdzony (oba bez ognia: AmbientTemp 17.01 = 17.01). C_1 zamrożony (BT stop, by nie wędrował).

| Test | Setup | RadiantHeat (C_1−C_2) | Oczekiwane |
|---|---|---:|---|
| **Magnituda d=0** | Fire1 na C_1 | **+24.95** | +25 = 25×(1−0)² ✓ |
| **Falloff d=300** | Fire1 300uu od C_1 | **+6.27** | +6.25 = 25×(1−0.5)² ✓ (huddle: ½r = 25% ciepła) |
| **CLAMP (dwa ogniska)** | Fire1+Fire2 na C_1 | **+40.0** | raw 2×25=50 → **clamp +40** ✓ (stos nie ugotuje NPC) |

→ Falloff `(1−d/r)²` „huddle" potwierdzony liczbowo (przy połowie zasięgu tylko ¼ ciepła — trzeba podejść blisko).
Clamp wchodzi przy nakładających się ogniskach (Twój kluczowy test). End-to-end: zimny NPC przy ognisku grzeje się.

## DŁUGI / dalej
- Spatial-grid dla rejestru gdyby źródeł były tysiące (dziś rzadkie → niepotrzebne).
- BP_HeatSource = goły Actor + komponent (placeholder) — realne ognisko (mesh/Niagara/światło + paliwo/gaszenie via `bIsLit`) = wizual/gameplay później.
- Bilans „komfort dla grupy" przy Warmth=25/Radius=600: realny komfort (AmbientTemp≥15) bardzo blisko ognia; reszta zasięgu = „nie zamarza, ale zimno". Dostroisz Warmth/Radius gdy zobaczysz w grze.

## WERDYKT
**L1-09b DONE + PIE-verified.** Ognisko podnosi lokalny AmbientTemp (huddle (1−d/r)², clamp ≤40, suma).
**L1-09 (temperatura: fire + clothing) DOMKNIĘTE** — clothing (L1-09a) + fire (L1-09b) gotowe. CTRL+S RANO: brak (C++ + nowy BP zapisane).
