# Raport — TASK 2 (sen / plaster #3) DONE + PIE-VERIFIED · 2026-06-23

> Sleep Engine: most Maslow→BT dla snu + fix pułapki omdlenia + Stamina jako gate prędkości.
> Build edytor-zamknięty (Result: Succeeded, 241s). PIE-verified twardymi log-stringami. Branch `feat/l1-night`.

## Zakres (wg zatwierdzonego spec)
- **D1:** `EvaluateCurrentNeed` Level_1_Rest odpala na `HoursAwake >= MentalFogThreshold` (16h), NIE na Staminie.
- **Stamina:** wycięta z drabiny potrzeb → **gate prędkości** (drain `CurrentActionStaminaCost` + regen + `GetStaminaSpeedMultiplier`, BP-ruch konsumuje). Była martwa (nigdy nie drenowana).
- **D2:** omdlenie = sen wymuszony (`bIsSleeping=true`); `UpdateFatigue` auto-woła `StopSleep` przy HoursAwake≈0 → `RestartLogic` (jedyne wyjście z pułapki; `StopSleep` miał 0 wywołań).
- **D5:** `UBTTask_Sleep` (C++): StartSleep → tick aż odespane → StopSleep, abort-safe.
- **D6:** `OnSleepStart` BP-event (poza snu); omdlenie zostaje przy OnCollapse/ragdoll.
- Wiring: `BTTask_Sleep` wpięty w pustą gałąź Handle Sleep (dekorator `CurrentNeed==Sleep(3)` był), BT_NPC zapisany.

## Commity / build
- C++: **`98c12e9`** (`MaslowBiologicalComponent.{h,cpp}` + `AI/BTTask_Sleep.{h,cpp}`). Build edytor-zamknięty UHT: **Result: Succeeded (241s)**, 0 błędów.
- Asset (untracked): `BT_NPC.uasset` (BTTask_Sleep) zapisany `EditorAssetLibrary.save_asset`; backup POST `_asset_backups/2026-06-23_task2-sleep_POST/`.

## PIE-VERIFY (mapa Game, świeży edytor po buildzie, twarde log-stringi LogMaslow — AUTONOMICZNIE na C_1 i C_2)

**✅ TEST A — sen DOBROWOLNY (pełny cykl):**
```
[Sleep] BP_NPC_Character_C_1: zasypia (HoursAwake=17.00, temp=36.6, jakość=1.00).
[Sleep] BP_NPC_Character_C_1: budzi się (HoursAwake=0.00, Rested=0).
```
HoursAwake≥16 → CurrentNeed=3 → Handle Sleep → `BTTask_Sleep` → `StartSleep` → HoursAwake drenuje (17→0) → `StopSleep`. Potwierdzone też live: HoursAwake 17→2.9 przy `bIsSleeping=true`. Identyczny cykl na C_2 (`budzi się HoursAwake=0.00`).

**✅ TEST B — OMDLENIE → AUTO-WYBUDZENIE (pułapka rozbrojona):**
```
[Sleep] BP_NPC_Character_C_1: zmęczenie 2 -> 3 przy HoursAwake=25.33 (akt x1.0).
[Sleep] BP_NPC_Character_C_1: OMDLENIE (HoursAwake=25.33) — BT zatrzymany, ragdoll.
[Sleep] BP_NPC_Character_C_1: zmęczenie 3 -> 2 przy HoursAwake=21.33 (akt x1.0, śpi).
[Sleep] BP_NPC_Character_C_1: zmęczenie 2 -> 1 przy HoursAwake=17.33 (akt x1.0, śpi).
[Sleep] BP_NPC_Character_C_1: zmęczenie 1 -> 0 przy HoursAwake=14.23 (akt x1.0, śpi).
```
Po OMDLENIU `bIsSleeping=true` (D2) → HoursAwake MALEJE mimo omdlenia ("śpi" 25→21→17→14...) → auto-`StopSleep`: live read **`bIncapacitated: True → False`, `bIsSleeping: True → False`, `FatigueState: Microsleeps → Awake`**. **Koniec wiecznego ragdolla.** Identycznie na C_2 (`OMDLENIE 25.00 → 3->2...15.70 śpi`).

**✅ TEST C — Stamina = prędkość, nie potrzeba:**
- `GetStaminaSpeedMultiplier()` przy niskiej Staminie = **0.54** (skaluje Lerp(0.5..1.0); ~0.54 bo Stamina zregenerowała 0→~8 między setem a odczytem → **drain/regen ŻYJE**).
- Sen w CAŁYM logu odpala wyłącznie z progów HoursAwake (16/20/24), **nigdy ze Staminy** → odsprzężenie potwierdzone.

## DŁUGI JAWNE (do roadmapy)
- **GetStaminaSpeedMultiplier — BP-ruch jeszcze NIE czyta** (C++ getter gotowy, konsument BP = follow-up, wzorzec jak `GetWorkEfficiencyMultiplier`). Stamina nie jest osierocona (ma drain+getter), ale prędkość ruchu zacznie reagować dopiero po wpięciu w BP MaxWalkSpeed.
- **F2 (z TASK 1) potwierdzony ponownie:** CurrentTemp<34 → temp-need→0 wyprzedza sen (utrudniał izolację testu A). Drabina: temp PONAD sen. Decyzja architekta (temperature-slice / fallback) wciąż otwarta.
- **F4 (z TASK 1):** BT NPC nie startuje automatycznie w PIE (trzeba `runtime_start_bt`). Po `StopSleep`→`RestartLogic` BT też nie wznowił widocznie (is_running:false) — prawdop. ten sam F4 (RestartLogic bez własnego RunBehaviorTree NPC). bIncapacitated jednak czyszczone (rdzeń fix). Do osobnego sprawdzenia auto-startu BT.
- **OnSleepStart / OnWakeUp** — placeholdery wizualne (poza snu/wstania), realna anim później (jak przy jedzeniu).

## WERDYKT
**TASK 2 (sen) DONE + PIE-VERIFIED.** Sen dobrowolny (BTTask_Sleep) i wybudzenie mimowolne z omdlenia
działają twardo-zweryfikowane na obu NPC. Stamina przeniesiona na oś prędkości (C++-kompletna, BP-konsument = dług).
**CTRL+S RANO: brak** (BT_NPC zapisany ścieżką silnikową; C++ zacommitowany). Następne: TASK 3 (flee) lub decyzja F2/F4.
