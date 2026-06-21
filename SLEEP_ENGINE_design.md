# SILNIK SNU (L1-06/07) — DESIGN GATE
> Zatwierdzony projekt do implementacji. C++ liczy (timer, progi, sprzężenie temp),
> Blueprint pokazuje (ragdoll, mikrosen, przecieranie oczu) przez BlueprintImplementableEvent.
> Oparty na realnej biologii: model dwuprocesowy (Borbély 1982), adenozyna = zużyty ATP.
> Decyzje: D1=skalowane aktywnością, D2=mocny most do temperatury, D3=głód odłożony.

═══════════════════════════════════════════════════════════
# RDZEŃ — HoursAwake jako adenozyna
═══════════════════════════════════════════════════════════
Pole HoursAwake JUŻ ISTNIEJE w UMaslowBiologicalComponent (getter gotowy, nigdy nie rośnie).
To zadanie = zbudować SILNIK, który je inkrementuje + skutki.

## Narastanie (D1: skalowane aktywnością)
Na ISTNIEJĄCYM timerze metabolizmu (10s tick — NIE nowy timer, NIE Event Tick):
```
HoursAwake += AwakeRatePerTick × ActivityMultiplier
```
- ActivityMultiplier = ten sam, którego używa metabolizm (SetCurrentActionMultiplier:
  Idle 1.0 / Work ~3.0 / Combat 5.0). REUŻYWAMY, nie tworzymy nowego.
- AwakeRatePerTick: dobrać tak, by przy Idle (1.0) NPC osiągał próg mgły (16h) i omdlenia
  (24h) w sensownym czasie gry. Przy 10s ticku i skali "1 tick = X godzin gry" —
  Claude Code wylicza X z docelowego tempa, zapisuje w MECHANICS.
- Efekt: drwal (Work) męczy się szybciej niż NPC odpoczywający. Emergentny realizm.

## Progi skutków (z MECHANICS [✓approved])
| HoursAwake | Skutek | Warstwa |
|---|---|---|
| ≥ 16h | Mgła umysłowa: −30% wydajności pracy | C++ flag, BP czyta |
| ≥ 20h | Mikrosny: NPC zamiera na ~1-2s losowo | C++ trigger, BP animacja |
| ≥ 24h | Omdlenie: ragdoll, bezbronny | C++ trigger, BP ragdoll |

## Reset — sen w Safe Zone
Gdy NPC śpi (stan SPI, ustawiany przez BT/akcję — NIE w tym zadaniu, tylko HOOK):
```
HoursAwake -= SleepRecoveryPerTick × TempQualityMultiplier   (do 0)
```
- Po pełnym przespaniu (HoursAwake → 0 w dobrych warunkach): buff "Rested" +20% praca/Eureka.
- TempQualityMultiplier ← patrz most temperatury niżej.
- 🔵 HOOK: sam stan "śpi" (kiedy NPC kładzie się spać) to decyzja BT/L3 (Safe Zone) —
  NIE w tym zadaniu. To zadanie buduje SILNIK (narastanie + reset + skutki + temp),
  wystawia funkcje StartSleep()/StopSleep() albo bIsSleeping flagę dla BT.

═══════════════════════════════════════════════════════════
# MOST TEMPERATURY (D2: MOCNY — sen na zimnie = ryzyko śmierci)
═══════════════════════════════════════════════════════════
Biologia: podczas snu ciało NIEMAL PRZESTAJE regulować temperaturę (brak drżenia/pocenia).
Śpiący jest DUŻO bardziej narażony na temp otoczenia niż czuwający.

## Dwa efekty temperatury na sen:

### 1. Jakość snu (TempQualityMultiplier — wpływa na reset)
Czyta ISTNIEJĄCE CurrentTemp z MaslowBiologicalComponent.
- Komfort (np. 15-24°C): TempQualityMultiplier = 1.0 → pełna regeneracja + buff Rested.
- Poza komfortem (za zimno <15°C / za gorąco >24°C): mult < 1.0 → wolniejszy reset,
  brak buffa Rested (sen "płytki").
- Progi komfortu: dobrać, zapisać w MECHANICS jako [approved] (możemy dostroić).

### 2. Utrata termoregulacji we śnie (ŚMIERTELNE — to jest "mocny" most)
Gdy bIsSleeping == true:
```
// Śpiący traci zdolność termoregulacji → kara zimna AMPLIFIKOWANA
if (bIsSleeping && CurrentTemp < ColdThreshold)   // ColdThreshold = istn. 10°C
{
    // ISTNIEJĄCY cold ×2 kcal burn staje się np. ×3-4 we śnie (brak drżenia)
    // + jeśli CurrentTemp ≤ CriticalTempThreshold (34°C) → hipotermia postępuje
    //   SZYBCIEJ we śnie niż na jawie (śpiący się nie rusza, nie szuka ciepła)
}
```
- Efekt rozgrywkowy: NPC, który zaśnie na mrozie BEZ ognia/schronienia, może UMRZEĆ we śnie.
- To DOMYKA pętlę Caldreth: stoki popiołu (ciepłe od wulkanu w nocy) = schronienie;
  ogień (Eureka) = ratunek; Safe Zone z ogniskiem = bezpieczny sen.
- 🔴 WAŻNE: to MUSI mieć fail-safe — NPC w panice/bez wyboru nie powinien instant-umierać.
  Hipotermia postępuje, daje czas na reakcję (obudzenie?), nie zabija w 1 tick.

═══════════════════════════════════════════════════════════
# PODZIAŁ C++ / BLUEPRINT (reguła projektu)
═══════════════════════════════════════════════════════════
## C++ (UMaslowBiologicalComponent — ROZSZERZAMY, nie nowy komponent):
- Inkrementacja HoursAwake na istniejącym timerze (× ActivityMultiplier).
- Wyliczanie progów (16/20/24h) → flagi/enum stanu zmęczenia (np. EFatigueState).
- TempQualityMultiplier z CurrentTemp.
- Sprzężenie sen+zimno (amplifikacja kary zimna we śnie).
- StartSleep()/StopSleep() lub SetIsSleeping(bool) — API dla BT.
- Reset HoursAwake przy śnie + buff Rested.
- Gettery: GetFatigueState(), IsInMentalFog(), GetWorkEfficiencyMultiplier() (uwzględnia mgłę).
- UE_LOG na przejściach progów (mgła/mikrosen/omdlenie/przebudzenie).

## Blueprint (przez BlueprintImplementableEvent — C++ woła, BP rysuje):
- OnMentalFogStart/End — wizualny feedback (np. desaturacja, ikona).
- OnMicrosleep — animacja "zamarcia" na 1-2s.
- OnPassout — ragdoll, NPC pada.
- OnWakeUp / OnRested — przecieranie oczu, ikona buffa.
Silnik C++ NIE WIE nic o wizualu — tylko woła eventy.

═══════════════════════════════════════════════════════════
# CO TO ZADANIE ROBI / CZEGO NIE ROBI
═══════════════════════════════════════════════════════════
✅ ROBI: silnik narastania zmęczenia, progi, skutki (flagi+BP eventy), sprzężenie z temp,
   API dla snu (StartSleep/StopSleep), reset+Rested, ożywia martwy pasek "Awake" w HUD.
❌ NIE ROBI (HOOKI na przyszłość):
- Decyzja KIEDY NPC idzie spać (to BT/L3 Safe Zone) — wystawiamy tylko API.
- Most do głodu (leptyna/grelina, D3) — ODŁOŻONY.
- "Gorszy osąd" we mgle (mylenie sojusznik/wróg) — HOOK na OCEAN/BT, nie teraz.
  (mgła = na razie tylko −30% praca; gettera GetWorkEfficiencyMultiplier użyje crafting.)
- Warty nocne, koszmary — haczyki w DESIGN, nie teraz.

═══════════════════════════════════════════════════════════
# WERYFIKACJA (jak sprawdzić że działa)
═══════════════════════════════════════════════════════════
TEST scenariusze (Claude Code ustawia przez istniejące API):
1. Wymuś HoursAwake=16h → sprawdź flag mgły + GetWorkEfficiencyMultiplier=0.7.
2. Wymuś HoursAwake=24h → sprawdź OnPassout zawołany.
3. Idle vs Combat przez N ticków → Combat rośnie szybciej (D1 działa).
4. bIsSleeping + CurrentTemp=5°C → hipotermia postępuje (most temp), ale NIE instant-death.
5. Sen w komforcie → HoursAwake spada, Rested buff po przebudzeniu.

WARTOŚCI do zapisania w MECHANICS po implementacji:
AwakeRatePerTick, skala "1 tick = X h gry", progi komfortu temp, SleepRecoveryPerTick,
amplifikacja zimna we śnie, TempQualityMultiplier krzywa.
