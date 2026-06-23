# Raport — TASK 3 (panika/flee, plaster #4) DONE + PIE-VERIFIED · 2026-06-23

> Panika przerywa BT, gałąź „Zagrożenie" podpięta, flee = ucieczka OD zagrożenia. Generyczny threat
> (damage/predator/człowiek). Build edytor-zamknięty (Succeeded 77s). Branch `feat/l1-night`. Ożywia L3-02.

## Zakres (wg zatwierdzonego spec)
- **Generyczne zagrożenie:** `EThreatType {None,Damage,Predator,HostileHuman}` + `ThreatActor`/`ThreatLocation`/
  `ThreatExpiryTime` na Maslow. `SetThreat/ClearThreat/IsThreatActive/GetThreatLocation` (BlueprintCallable —
  percepcja predatora/wroga L0-04 użyje TEGO SAMEGO API). `ThreatMemoryDuration` (domyślnie 8s).
- **Damage-hook:** `BeginPlay` binduje `GetOwner()->OnTakeAnyDamage` → `HandleAnyDamage` → `SetThreat(causer, Damage)`.
  Generyczne: KAŻDE `ApplyDamage` (predator/człowiek/pułapka) staje się zagrożeniem; `BodyConditionComponent::ApplyDamage`
  (część-specyficzne, bez instigatora) celowo NIE odpala flee (od hipotermii się nie ucieka).
- **Threat = 3. wyzwalacz paniki:** `EvaluateCurrentNeed`: `bIsInPanic || CurrentHP<=CriticalHPThreshold ||
  IsThreatActive()` → `Level_0_FightOrFlight` → `GetActionableNeed=4 (Flee)`, najwyższy priorytet.
- **`UBTTask_Flee` (C++):** punkt = `PawnLoc + normalize(PawnLoc-ThreatLoc)*FleeDistance`, projekcja na navmesh,
  zapis BB `FleeLocation` (Vector) + `OnPanicFlee` BP-event. Po nim `BTTask_MoveTo(FleeLocation)`.
- **Wiring BT:** „Zagrożenie" przepięta jako **1. dziecko Selectora** (najwyższy priorytet), dekorator
  `BTDecorator_Blackboard CurrentNeed==Flee(4)` z **`FlowAbortMode=Both`** (przerywa niższą gałąź gdy panika,
  zwalnia gdy minie). Dzieci: `BTTask_Flee → BTTask_MoveTo(FleeLocation)`. Puste Widoczne/Dźwięk + sierota Selector skasowane.

## Commity / build
- C++: **`826b20e`** (`MaslowBiologicalComponent.{h,cpp}` + `AI/BTTask_Flee.{h,cpp}` + `Build.cs:+NavigationSystem`).
  Build edytor-zamknięty UHT: **Result: Succeeded (77s)**.
- Asset (untracked): `BT_NPC` (gałąź Flee), `BB_NPC` (+ key `FleeLocation` Vector) — zapisane `EditorAssetLibrary.save_asset`;
  backup POST `_asset_backups/2026-06-23_task3-flee_POST/`. Walidacja BT: valid, 0 sierot po sprzątaniu.

## PIE-VERIFY (mapa Game, świeży edytor po buildzie)

**✅ DAMAGE-HOOK → SetThreat (twardy log):**
```
[Flee] BP_NPC_Character_C_1: ZAGROŻENIE typ=1 od 'BP_NPC_Character_C_2' @ (8927,3831) — ucieka 8.0s.
```
`apply_damage(C_1, 12, causer=C_2)` → `OnTakeAnyDamage` → `SetThreat(C_2, Damage)`. **ThreatType=Damage,
ThreatLocation = pozycja causera.** Powtórzone 2× (różne pozycje C_2) — hook stabilny.

**✅ THREAT → FLEE (najwyższy priorytet):** `IsThreatActive=true` → `GetActionableNeed=4` (rzetelny pie_call).

**✅ GAŁĄŹ FLEE AKTYWNA + ucieczka OD zagrożenia:** `active_node = "Flee (away from threat)" (BTTask_Flee)`,
potem `active_node = "Move To"` (MoveTo wewnątrz gałęzi) — NPC biegnie do `FleeLocation`. FleeLocation
oddala od threatu: odległość NPC→threat **1021 → 2184 uu** (punkt dalej od zagrożenia; nav-projekcja
modyfikuje dokładny wektor na osiągalny — placeholder).

**✅ PRZERWANIE (FlowAbortMode):** w teście śpiącego NPC (CurrentNeed=3, sen aktywny) `apply_damage` →
`SetThreat` → bez restartu BT `FleeLocation` został zapisany (Flee przejął = **przerwał sen**) → po
wygaśnięciu okna threatu CurrentNeed wrócił do **Sleep(3)** (Flee zwolnił, NPC wznowił poprzednią potrzebę).

## DŁUGI / RYZYKA (flaguję)
- **Flee = PLACEHOLDER:** ucieka do nav-osiągalnego punktu „dalej od threatu", NIE do Safe Zone i bez drop-container.
  Realny L0-03 (DropContainer L2-12 + Safe Zone L3-07) + threat-detection wzrok/słuch L0-04 = osobne taski.
  **Predator/wrogi człowiek już OBSŁUŻENI architektonicznie**: oba przez to samo `SetThreat` (damage teraz; percepcja L0-04 = ten sam hak).
- **D4 — `BTTask_Eat` abort:** panika w trakcie jedzenia MOŻE zostawić `bIsEating` chwilowo (montaż gra do notify-StopEating /
  auto-stop przy sytości). Niezweryfikowane twardo (trudno utrzymać NPC w jedzeniu — glukoza odbija). Mała poprawka BP
  (StopEating-on-abort) = dług. `BTTask_Sleep` jest abort-safe (StopSleep ✓).
- **Dekorator enum-display:** Monolith ustawił `IntValue=4` (autorytatywne — Flee odpala), ale `CachedDescription`/`StringValue`
  pokazują „None" (nieodświeżony display, klasa TECH-12). Funkcjonalnie OK (PIE: gałąź odpala na 4); kosmetyka do poprawy w edytorze.
- **Nav-projekcja flee** na mapie Game bywa luźna (navmesh ograniczony) → wektor ucieczki przybliżony. Na CaldrethMap (nav OK) czyściej.

## WERDYKT
**TASK 3 (flee/panika) DONE + PIE-VERIFIED.** Damage-hook → generyczny threat → najwyższy priorytet →
przerwanie BT (FlowAbortMode=Both) → gałąź „Zagrożenie"/Flee → ucieczka od zagrożenia → powrót po wygaśnięciu.
**L3-02 panika OŻYWIONA** (dociera do BT). Predator/człowiek obsłużeni tym samym `SetThreat` (L0-04 podepnie percepcję).
**CTRL+S rano: brak** (asset zapisany ścieżką silnikową + backup). C++ zacommitowany.
