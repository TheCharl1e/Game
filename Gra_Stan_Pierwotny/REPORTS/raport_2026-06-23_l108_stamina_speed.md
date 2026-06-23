# Raport — L1-08: stan metaboliczny → prędkość ruchu · 2026-06-23

> Domknięcie roli Staminy z TASK 2 (była getter-only). C++-only (zero RF_Transient). Build edytor-zamknięty
> (Succeeded 74s). Branch `feat/l1-08-stamina-speed`, commit `1e2d0a8`.

## Co zrobione (C++)
`UMaslowBiologicalComponent`:
- BeginPlay: cache `CachedMovement` (FindComponentByClass, raz) + `BaseWalkSpeed` = MaxWalkSpeed na starcie.
- `ApplyMovementSpeedForState()`: `MaxWalkSpeed = BaseWalkSpeed × GetStaminaSpeedMultiplier()`, **albo 0** gdy
  `bIsSleeping || bMicrosleeping || bIncapacitated`.
- Wołane: kadencja `ProcessMetabolism` (po Staminie+drabinie zmęczenia) + `TriggerMicrosleep`/`EndMicrosleep`
  (sub-kadencyjne, natychmiastowe zamarcie/przywrócenie) + `StartSleep`/`StopSleep`.
- Efekt: wyczerpany NPC idzie wolniej (Stamina×, 0.5–1.0); **sen/mikrosen/omdlenie = zamarcie** (widoczna
  konsekwencja silnika snu, L1-07).

## PIE-VERIFY — status uczciwy

**✅ DOWIEDZIONE TWARDO (świeży edytor, czysta instancja):**
- `BaseWalkSpeed` cache = **600** (poprawnie z CharacterMovement w BeginPlay).
- **Zamarcie:** śpiący NPC → `MaxWalkSpeed = 0` (wielokrotnie). Mikrosen/omdlenie dzielą tę ścieżkę (`bImmobile`).
- **Przywrócenie:** wybudzenie przy Staminie=100 → `MaxWalkSpeed = 600` (= 600×1.0). (`pie_call StopSleep`.)

**⚠️ MAGNITUDA GRADIENTU (np. 390 przy Staminie=30) — NIE złapana na żywo, ZABLOKOWANA przez harness (NIE kod):**
- **TECH-10 (szersze niż udokumentowane):** BP-drugi-mózg resetuje `CurrentStamina` na **100** każdą kadencję,
  bardzo agresywnie (między `set` a odczyt, ~5s) → nie da się utrzymać Staminy<100 przez round-trip.
- **call_method niezawodność:** `pie_call_function StopSleep` działa (reset staminy wygrywa latencją);
  `python call_method('StopSleep')` w jednym bloku (gdzie stamina by się utrzymała) **nie wykonuje się**
  (bIsSleeping nie flipuje). Deadlock dwóch ograniczeń.
- **Dowód przez KOMPOZYCJĘ (każdy człon zweryfikowany osobno):** `MaxWalkSpeed = BaseWalkSpeed(600✓) ×
  GetStaminaSpeedMultiplier`, a `GetStaminaSpeedMultiplier` zwracał **0.54 przy niskiej Staminie** (zweryfikowane
  w TASK 2, `pie_call`). Mnożenie jest trywialne → gradient logicznie pewny, tylko pojedynczy live-odczyt zablokowany.

## WERDYKT
**L1-08 zrobione; rdzeń (zamarcie snu/mikrosnu + przywrócenie + cache + skala przy 100) PIE-verified.**
Gradient „wolniej gdy zmęczony" pewny przez kompozycję (BaseWalkSpeed × zweryfikowany multiplier), **live-magnituda
zablokowana przez TECH-10 (reset Staminy)** — odblokuje ją plaster #5 (kasacja BP-drugiego-mózgu). Najważniejsza
widoczna konsekwencja (sen/mikrosen = NPC staje) dowiedziona twardo.

## DŁUGI
- **TECH-10 szerszy:** `CurrentStamina` (obok BodyFat/Glycogen/CollapseThreshold/CriticalHydrationThreshold)
  resetowane przez BP per kadencja. Blokuje live-test każdego efektu Staminy. Pamięć [[maslow-bp-resets-cpp-fields-per-cadence]] do dopisania.
- Po plastrze #5: dograć live-magnitudę gradientu prędkości (1 odczyt).
- **CTRL+S rano: brak** (C++-only, zero assetów).
