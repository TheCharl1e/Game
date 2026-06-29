# Raport — TASK 2 (sen / plaster #3) RECON + GATE-REQUEST · 2026-06-23

> Recon przed kodem (Sleep Engine = defining mechanic; CLAUDE.md: „2 bugi złapane TYLKO twardym PIE").
> **STOP: nie buduję bez gate'u.** Poniżej co istnieje + decyzje projektowe, których nie wolno mi zgadnąć.

## Co JUŻ istnieje (C++ silnik snu — bogaty, BT-ready)
`UMaslowBiologicalComponent`:
- **Drabina zmęczenia** (`ProcessFatigue`, cpp:295-368): `HoursAwake` rośnie na jawie
  (`+=AwakeRatePerTick*ActionKcalMult`), MALEJE we śnie (`-=SleepRecoveryPerTick*TempQuality`). Progi:
  MentalFog 16h → Microsleep 20h → **Collapse 24h**.
- **Hooki API dla BT (BlueprintCallable):** `StartSleep()` (cpp:497) i `StopSleep()` (cpp:509).
  Nagłówek wprost: *„HOOK — decyzja KIEDY to BT/L3 (Safe Zone), nie ten silnik."* `StopSleep`: pełny sen
  w komforcie (HoursAwake→0) → buff `bIsRested`; **znosi omdlenie + `Brain->RestartLogic()` (wznawia BT).**
- **Omdlenie** (cpp:351-368): wejście w Collapsed → `bIncapacitated=true` + `Brain->StopLogic("Collapsed")`
  (zatrzymuje BT) + `OnCollapse()` (BP ragdoll).
- **Mikrosen** (random na jawie ≥20h → `OnMicrosleep`/timer), **Rested-buff** (`RestedWorkBonus`).
- BP-eventy gotowe: `OnCollapse`, `OnWakeUp`, `OnMicrosleep/End`, `OnMentalFog/End`.

## 🔴 Dwa twarde problemy (potwierdzone grep/źródło)

**P1 — PUŁAPKA OMDLENIA bez wyjścia.** `StartSleep`/`StopSleep` mają **ZERO wywołań w całym Source/**
(grep: tylko definicje + deklaracje). Omdlenie robi `StopLogic` → BT martwy → jedyne wybudzenie to
`StopSleep`→`RestartLogic`, a `StopSleep` nikt nie woła → **NPC omdlewa NA ZAWSZE.** To rdzeń „fix pułapki omdlenia".

**P2 — ZGRZYT OSI (need vs silnik).** Potrzeba „Sen" w drabinie = `Level_1_Rest` = `CurrentStamina<=CriticalStaminaThreshold`
(cpp:712). Ale sen regeneruje `HoursAwake`, NIE `CurrentStamina`. `CurrentStamina` rośnie WYŁĄCZNIE od jedzenia
(`+MoraleBonus`, cpp:656). → **Spanie NIE gasi potrzeby snu** (potrzeba=stamina, sen=HoursAwake — dwie różne osie).
NPC ze Staminą<15 spałby w nieskończoność, bo HoursAwake spada do 0, a Stamina się nie rusza → need wciąż Sleep.

## Czego NIE MA
- Żadnego `BTTask_Sleep` (lista tasków: Check/Drink/Eat/FindWater/FindFood/Searching).
- Gałąź Handle Sleep w BT_NPC = PUSTA (dekorator `CurrentNeed==Sleep(3)` jest, 0 dzieci — export_bt_spec).
- Żadnej animacji/pozy snu (są tylko hooki BP-event).

## DECYZJE PROJEKTOWE (gate — proszę o rozstrzygnięcie, z moją rekomendacją)

**D1 (FORK — P2 oś potrzeby).** Co reprezentuje potrzeba „Sen" i co ją gasi?
 - (a) **REKOMENDACJA:** zmienić trigger `Level_1_Rest` na **HoursAwake-based** (np. `HoursAwake>=MentalFogThreshold`),
   bo to oś, którą sen realnie regeneruje. Spójne, jednoosiowe. (Stamina zostaje osobnym torem — zmęczenie-akcją.)
 - (b) Zostawić trigger na Staminie, ale `StartSleep`/sen ma TEŻ regenerować Staminę. Dwa tory zbiegają się we śnie.
 - To rozstrzyga, kiedy NPC idzie spać i kiedy się budzi. **Bez tego pętla snu nie domyka się poprawnie.**

**D2 (FORK — P1 architektura wybudzenia z omdlenia).** Kto wybudza omdlałego (BT martwy po StopLogic)?
 - (a) **REKOMENDACJA: C++ posiada wybudzenie MIMOWOLNE.** Omdlenie = stan, w którym `ProcessFatigue` dalej
   obniża HoursAwake (sen wymuszony), a gdy HoursAwake spadnie poniżej progu wybudzenia → C++ sam woła
   `StopSleep()` (→RestartLogic). BT posiada tylko sen DOBROWOLNY (Handle Sleep). Czysty rozdział: wola vs omdlenie.
 - (b) Omdlenie NIE robi StopLogic — zamiast tego wymusza w BT gałąź „sen wymuszony" (BT żyje, Handle Sleep biegnie incapacitated).
 - (a) jest bezpieczniejsze (nie polega na żywym BT w stanie ragdoll). Wymaga małej zmiany C++ w collapse/ProcessFatigue.

**D3 (zakres) — gdzie NPC śpi?** REKOMENDACJA: **w miejscu** (minimal). Safe Zone/shelter (L3-07) = osobny task później.
**D4 (wybudzenie dobrowolne):** REKOMENDACJA: `HoursAwake<=ε` (pełny sen) → `StopSleep`.
**D5 (BTTask_Sleep):** REKOMENDACJA: **C++ `UBTTask_Sleep`** (pętla „czekaj aż HoursAwake niskie" czystsza w C++
niż w BP; spójne z mostem). Alternatywa: BP (spójne z Eat/Drink). **Twoja preferencja?**
**D6 (anim):** placeholder pozy snu (jak przy jedzeniu) via nowy BP-event/`OnWakeUp` — później realna.

## STOP
Recon gotowy. **Nie buduję, póki nie dostanę D1+D2 (forki) i preferencji D5.** D3/D4/D6 mam rekomendacje —
zatwierdź lub zmień. Po gate: C++ (build edytor-zamknięty) + BTTask_Sleep + wypełnienie Handle Sleep + PIE-verify
(twarde: NPC zmęczony→StartSleep→HoursAwake maleje→StopSleep→wstaje; ORAZ omdlenie→auto-wybudzenie, NIE wieczny ragdoll).
