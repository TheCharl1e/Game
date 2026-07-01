# Raport — TASK 3 (panika/flee, plaster #4) RECON + GATE-REQUEST · 2026-06-23

> Recon przed kodem (flee/panika = defining mechanic; ożywia L3-02). **STOP: nie buduję bez gate'u.**

## Co JUŻ istnieje (C++ strona — gotowa)
- `EvaluatePanicRoll()` (cpp:795, z ProcessMetabolism) ustawia `bIsInPanic` (stochastyczny rzut Neuroticism×HP%, latch; wychodzi gdy HP% poza pasmem paniki).
- `EvaluateCurrentNeed`: `if (bIsInPanic || CurrentHP <= CriticalHPThreshold) return Level_0_FightOrFlight` — **NAJWYŻSZY priorytet**.
- `GetActionableNeed`: Level_0_FightOrFlight → **4 (Flee)**. Serwis pisze `CurrentNeed=4` do BB. **C++ produkuje Flee gotowe.**

## Czego NIE MA (cała strona BT + infra)
- Selector „Potrzeby" ma TYLKO Thirst(2)/Hunger(1)/Sleep(3) — **żadnej gałęzi Flee(4)**; wszystkie dekoratory `FlowAbortMode:None` (NIE przerywają biegnącej gałęzi).
- Gałąź „Zagrożenie" ODŁĄCZONA (`ParentNode:None`), bez dekoratora; dzieci „Widoczne zagrożenie"/„Dźwięk" PUSTE (to scaffolding DETEKCJI = L0-04, nie odpowiedź-flee).
- Brak `BTTask_Flee`, brak BP-eventu paniki (`OnPanic`/`OnFlee`), brak `DropContainer` (L2-12), brak Safe Zone (L3-07), brak aktora-zagrożenia (L0-04). **Flee nie ma DOKĄD uciekać.**

## 🔴 Konflikt z designem
L0-03 mówi: „drop loaded container, run to Safe Zone". **Wszystkie 3 zależności nie istnieją** (DropContainer/SafeZone/threat-detection). → flee MOŻE być teraz tylko **placeholderem** (ucieczka w losowe miejsce), realny flee po L0-03/04 + L3-07. Flaguję, nie udaję.

## SPEC TASK 3 (do zatwierdzenia, z rekomendacją)

**BT (Monolith) — rdzeń = PRZERWANIE + gałąź Flee:**
1. **D2 — INTERRUPT (sedno zadania):** nowa gałąź Flee = PIERWSZE dziecko Selectora (najwyższy priorytet),
   dekorator `CurrentNeed == Flee(4)` z **`FlowAbortMode = LowerPriority`** + observer „On Result Change".
   Gdy CurrentNeed flipnie na 4 (panika), dekorator **ABORTUJE biegnącą gałąź** (Thirst/Hunger/Sleep) i wchodzi w Flee. To „przerwij BT na bIsInPanic".
2. **D3 — reuse „Zagrożenie":** podpinam istniejącą gałąź „Zagrożenie" jako to pierwsze dziecko (zgodnie z poleceniem „podłącz gałąź Zagrożenie"), dokładam dekorator Flee(4)+abort, wstawiam task flee. Widoczne/Dźwięk zostają puste (L0-04). *(Alt: świeża gałąź „Handle Panic" + kasacja sieroty — powiedz, jeśli wolisz.)*

**C++ (build edytor-zamknięty) — minimalny task flee:**
3. **D1 — `UBTTask_Flee` (placeholder, bo brak destynacji):** odpala `OnPanicFlee` (nowy BP-event, wizual paniki) +
   MoveTo do losowego osiągalnego punktu w promieniu (`GetRandomReachablePointInRadius`, EQS-free) w pętli, aż abort.
   Gdy panika minie (bIsInPanic→false, HP wraca) → CurrentNeed→niższa potrzeba → dekorator abortuje Flee → NPC wraca do życia. **REKOMENDACJA.** *(Alt minimal: sam OnPanicFlee + Wait, zero ruchu.)*
4. **OnPanicFlee** = `BlueprintImplementableEvent` (jak OnCollapse) — placeholder wizualny.

**Długi/ryzyka (flaguję):**
- **D4 — abort-safety przerywanych tasków:** `BTTask_Sleep` jest abort-safe (StopSleep ✓). **`BTTask_Eat` (BP) — panika w trakcie jedzenia może zostawić `bIsEating` zawieszone** → trzeba dodać StopEating na abort (mała poprawka BP). Sprawdzę przy buildzie.
- Flee placeholder (losowy punkt) ≠ L0-03 (drop container + Safe Zone) — dług do L0-03/04/L3-07.

**PIE-verify (po build):** NPC je/śpi → wymuś panikę (`bIsInPanic=true` lub `CurrentHP≤25`) → **CurrentNeed=4 ABORTUJE biegnącą gałąź** → Flee aktywne (MoveTo losowy + OnPanicFlee) → panika mija → NPC wraca do poprzedniej potrzeby. Twardo: log abort + active_node=BTTask_Flee + powrót.

## STOP
**Nie buduję, póki nie zatwierdzisz D1 (placeholder-flee OK?) i D3 (reuse „Zagrożenie" vs świeża gałąź).** D2/D4 mam rekomendacje. Po „go": C++ BTTask_Flee+event → build → wiring gałęzi+dekorator-abort → PIE.
