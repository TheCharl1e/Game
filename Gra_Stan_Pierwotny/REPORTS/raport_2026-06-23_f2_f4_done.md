# Raport — F2 (temp gasi potrzeby) + F4 (BT-autostart) DONE · 2026-06-23

> Dwa długi z TASK 1/2 domknięte. F2 = realny fix C++ (build+PIE). F4 = false alarm (recon obalił). Branch `feat/l1-night`.

## F2 — niezaimplementowane potrzeby NIE gaszą już niższych ✅ FIXED

**Bug:** `EvaluateCurrentNeed` zwracał `Level_1_Temperature` (gdy CurrentTemp≤CriticalTempThreshold), a
`GetActionableNeed` mapował go na **0 (None)**. Temperatura stoi PONAD Hydration/Rest/Nutrition w drabinie →
**zmarznięty NPC zwracał need=0 i marzł bezczynnie** zamiast jeść/pić/spać. Nic nie logowało → cicha pułapka.

**Recon:** `EvaluateCurrentNeed` ma JEDNEGO realnego konsumenta (`GetActionableNeed`); `Level_1_Temperature`
to jedyny realny swallow (`Level_2_Safety` nigdy nie zwracany — martwy case). → czysty, bezdryftowy fix.

**Fix (commit `e8460af`, build edytor-zamknięty: Succeeded 53s):** `EvaluateCurrentNeed(bool bActionableOnly=false)`.
Gdy `true`, POMIJA niezaimplementowane poziomy (Temperature) i schodzi do następnej ACTIONABLE potrzeby.
`GetActionableNeed` woła z `true`. Pełna drabina `EvaluateCurrentNeed()` (bez flagi) wciąż zwraca Temperature
(HUD/przyszłość). Hipotermia C++ (cold-burn/obrażenia) działa niezależnie od tej gałęzi.

**PIE-VERIFY (twarde, NPC BP_NPC_Character_C_1):**
- A/B bezpośredni: `EvaluateCurrentNeed()` = **Level_1_Temperature** (pełna drabina widzi zimno) vs
  `EvaluateCurrentNeed(bActionableOnly=true)` = **Satisfied** (pomija Temperature). Flaga skutecznie przeskakuje poziom.
- **End-to-end:** NPC zimny (CurrentTemp 20→28.9, ≤34 = temp aktywne) + zmęczony (HoursAwake 18) →
  `GetActionableNeed`→Sleep → **`active_node = BTTask_Sleep`**, HoursAwake spadło **18 → 9.8**. **Zmarznięty NPC
  ZASNĄŁ zamiast marznąć bezczynnie.** Przed fixem: need=0 → idle → HoursAwake rośnie → eventualny collapse.

## F4 — BT-autostart ✅ FALSE ALARM (bez fixu)

**Recon (BP_NPC_AI EventGraph):** wiring jest podręcznikowo poprawny:
`Zdarzenie On Possess → UseBlackboard(BB_NPC) → RunBehaviorTree(BTAsset=BT_NPC) → ...`. Kontroler woła BT na possess.

**Empiryczny test (świeży PIE, BEZ ręcznego startu):** `runtime_get_bt_state` → **`is_running:true`, `tree_asset=BT_NPC`**.
BT auto-startuje poprawnie. Złapany live `active_node = "Sleep (Maslow)" / BTTask_Sleep` autonomicznie.

**Wniosek:** wcześniejsze `is_running:false` (TASK 1/2) = **odczyty PRZED zakończeniem possession** (timing), nie
bug. F4 NIE wymaga fixu. **To wzmacnia TASK 1/2: most i sen działają w PIE autonomicznie, bez `runtime_start_bt`.**

**Sub-uwaga (post-collapse resume):** nie udało się wymusić czystego collapse do snapshotu `is_running`-resume,
bo **system robustnie wybiera sen DOBROWOLNY** zanim dojdzie do omdlenia (HoursAwake≥16 → BTTask_Sleep drenuje
HoursAwake poniżej 24 zanim collapse trafi; thresholdy resetują się — TECH-10). Collapse to rzadki safety-net;
jego recovery (`bIncapacitated` True→False) już dowiedziony w TASK 2. `RestartLogic` = engine-standard, BT auto-runs.

## WERDYKT
**F2 FIXED + PIE-verified** (commit `e8460af`); **F4 RESOLVED** (false alarm, recon+empiria). Oba odblokowują
TASK 3 (panika/flee): panika to wyższy priorytet, musi przerywać żywy BT — auto-start (F4) potwierdzony, a
brak swallow (F2) zapewnia, że niższe potrzeby nie znikają pod nieaktywnymi wyższymi.
**CTRL+S rano: brak.** Pamięć `getactionableneed-temp-safety-swallow-needs` → oznaczona FIXED.
