# GŁÓD — WIZJA PROJEKTOWA (felt vs true, dwie warstwy, człowieczeństwo)
> Status: **WIZJA / RAMA** — NIE jest jeszcze specyfikacją do kodowania.
> Cel dokumentu: utrwalić pełny model głodu (biologia → mechanika), żeby plaster #2 mostu Maslow→BT
> dostał ŚWIADOME hooki, nie przypadkowe, a kolejne plastry miały gotowy obraz.
> Data szkicu: 2026-06-20. Autor: Architekt (rozmowa z dyrektorem).
> Ugruntowanie biologiczne (wzorzec projektu, jak Borbély 1982 dla snu): dwusygnałowy model głodu
> (grelina/leptyna) + Minnesota Starvation Experiment (Keys, 1944–45) dla psychologii skrajności.

═══════════════════════════════════════════════════════════
# RDZEŃ — JEDNO ZDANIE
═══════════════════════════════════════════════════════════
Głód to NIE jeden pasek. To DWA rozjechane systemy: **sygnał ODCZUWANY** (grelina — fale, wyuczony,
modulowany osobowością) i **stan PRAWDZIWY** (kaskada kataboliczna — realne wyczerpanie rezerw).
Warstwa 1 = głód odczuwany napędza zwykłe jedzenie. Warstwa 2 = prawda kaskady napędza skrajność
(kradzież, kanibalizm), a człowieczeństwo × OCEAN to rzut obronny przeciw instynktowi — który SŁABNIE
pod głodówką. Ta sama głodówka, inna reakcja — filozofia OCEAN przeniesiona z paniki na moralność.

═══════════════════════════════════════════════════════════
# BIOLOGIA (research — dlaczego dwa sygnały, nie jeden pasek)
═══════════════════════════════════════════════════════════
Człowiek nie ma „paska głodu". Ma dwa rozjechane systemy:

## Sygnał ODCZUWANY (krótkoterminowy) — grelina
- Hormon z żołądka, rośnie **falami** przed porami posiłków. Wyuczony i rytmiczny: czujesz głód o 13:00,
  bo zawsze jadłeś o 13:00 — nie dlatego, że nagle zabrakło paliwa.
- Głód przychodzi **napadami, nie rampą**: pang mija, gdy go zignorujesz, wraca silniejszy.
  → To inny KSZTAŁT niż liniowy spadek. Pasek głodu kłamie o tym, jak głód się czuje.

## Stan PRAWDZIWY (długoterminowy) — leptyna + kaskada
- Leptyna z tkanki tłuszczowej: sygnał długoterminowych rezerw. Mało tłuszczu → mało leptyny → uporczywy głód.
- Kaskada (JUŻ w kodzie, `EHungerPhase`): Glucose → Glycogen → FatBurn (0.5× tempa) →
  Autophagy (MaxHP −0.25×burn, TRWALE) → Death.

## 🔑 KLUCZOWY ROZJAZD (źródło całej głębi)
Te dwa sygnały potrafią się ROZJECHAĆ:
- Możesz CZUĆ głód (fala greliny) mając pełne rezerwy tłuszczu — felt > true.
- I — przerażające, prawdziwe — **w głębokiej ketozie/autofagii APETYT PRZYGASA**. Ciało zaczyna się
  zjadać, trwale niszczy mięśnie, a ODCZUWANY głód cichnie. Człowiek umiera, czując MNIEJ głodu niż
  dzień wcześniej — true rośnie, felt spada.
→ To jest dokładnie odłożony koncept **felt-need vs true-state** (patrz NPC_DEEPENING_concepts).
  Głód jest najlepszym miejscem, żeby go ożywić.

## Psychologia skrajności — Minnesota Starvation Experiment (Keys, 1944)
~36 ochotników-pacyfistów z zasadami, ~pół roku półgłodówki (~1570 kcal/dzień). Co się stało
(gotowy projekt zachowań, obserwacja, nie kontestowana neuro-teoria):
- **Obsesja jedzeniem** — myśli kurczyły się do jedzenia; zbierali przepisy, rytualizowali posiłki, chomikowali.
- **Rozpad osobowości** — drażliwość, apatia, wycofanie społeczne, utrata humoru, empatii, libido.
- **Łamanie WŁASNYCH zasad** — pryncypialni ochotnicy kradli jedzenie, łamali dietę.
- **Każdy pękał w INNYM momencie** — niektórzy wytrzymali, jeden się okaleczył.
🔑 Wniosek dla gry: głód NIE zmienia wszystkich tak samo. To filozofia killer-demo OCEAN, przeniesiona
   z paniki na MORALNOŚĆ. Ta sama rana, inna reakcja — indywidualnie, emergentnie.

## Co świadomie pomijamy (uczciwość naukowa)
Prosty model „niska glukoza → niska siła woli" (ego-depletion / glukozowy model samokontroli) jest
w nauce KONTESTOWANY (kryzys replikacji). NIE opieramy na nim mechaniki. Opieramy się na MOCNYCH
obserwacjach: Minnesota (erozja zasad/empatii), zawężenie horyzontu czasowego pod niedoborem
(present bias — robustne w ekonomii behawioralnej). „Restraint słabnie" = grunt obserwacyjny, nie sporna neuro.

═══════════════════════════════════════════════════════════
# WARSTWA 1 — GŁÓD ODCZUWANY → JEDZENIE (plaster #2)
═══════════════════════════════════════════════════════════
**Co robi:** sygnał odczuwany napędza zwykłe „idź zjedz". To bliźniak pragnienia z mostu, ale BOGATSZY
o modulację osobowością.

**Jak (warstwa 1):**
- Wyzwalacz na teraz: proxy `Glucose ≤ CriticalKcalThreshold` (true-state jako tymczasowy stand-in
  za sygnał odczuwany). Docelowo: grelina-fale (oscylator wyuczony porami posiłków) jako osobny hook.
- `GetActionableNeed()` JUŻ mapuje `Level_1_Nutrition → 1 (Hunger)` (most plaster #1). Warstwa 1 = re-attach
  odłączonej gałęzi „Handle Hunger" (recon: `parent_id:null`) + dowód, że C++ Glucose (nie BP HungerLevel) je.
- **Modulacja OCEAN (hook, lekki):** Neurotyczność wzmacnia PILNOŚĆ odczuwanego głodu — nerwowy je
  wcześniej/gwałtowniej. To TEN SAM lewar, co rzut paniki (`EffThr=Lerp(...)` z Neurotyzmem). Sumienny
  je z wyprzedzeniem, gromadzi (most do „znoszenia do spiżarni" L2/L3).

**Gdzie się wpina:** kaskada DALEJ liczy fazy pod spodem (są hookiem pod warstwę 2). Mgła snu (L1) obniża
skuteczność szukania jedzenia. Jedzenie konsumuje najpierw prywatne, potem publiczną spiżarnię (L2).

**Status:** to jest ZAKRES plastra #2 mostu (talk-first gate pisze Executor, grounded w żywym kodzie).

═══════════════════════════════════════════════════════════
# WARSTWA 2 — PRAWDA KASKADY → SKRAJNOŚĆ (osobny plaster, później)
═══════════════════════════════════════════════════════════
**Co robi:** głęboka faza kaskady (tłuszcz/autofagia) napędza NACISK INSTYNKTU; człowieczeństwo × OCEAN
to rzut obronny. Wynik: kradzież, w skrajności kanibalizm — emergentnie, indywidualnie.

**Jak (model rzutu — wzorzec rzutu paniki / mikrosnu):**
```
NaciskInstynktu  = f(faza kaskady)        // glukoza≈0, glikogen mały, FatBurn średni, Autophagy MAX
Restraint        = Człowieczeństwo × wpływ(Agreeableness, Conscientiousness)
Restraint        -= ErozjaGłodówki(czas w głębokiej fazie)   // Minnesota: zasady słabną z czasem
SzansaPęknięcia  = Clamp((NaciskInstynktu − Restraint) / Pasmo, 0, MaxSzansa)
rzut FRand() < SzansaPęknięcia  → instynkt wygrywa (kradzież / kanibalizm)
```
- **Próg kradzieży = Ugodowość:** wysoka A szanuje cudze jedzenie nawet w desperacji (kradnie późno/wcale),
  niska A kradnie wcześnie. Kanibalizm = najgłębsza faza + najniższy restraint.
- **🔴 Tragiczny haczyk (sedno):** człowieczeństwo SŁABNIE pod głodówką (Minnesota: utrata empatii/zasad)
  → rzut obronny robi się TRUDNIEJSZY dokładnie wtedy, gdy najbardziej potrzebny. „Moralność to luksus
  zaspokojonych potrzeb" (NPC_DEEPENING). Głodny NPC ma mniej człowieczeństwa, by oprzeć się kradzieży.

**Gdzie się wpina:** kradzież → hook `THEFT:` JUŻ strzela w InventoryComponent (L2) → zgłoszenie do Wodza
→ detektyw L5. Kanibalizm → tabu społeczne (L3), strach u innych (L4 reputacja −). Pęknięcie z głodu →
może odpalić panikę L0 (desperacja). Złamanie zasad widziane przez świadków → log akcji (L5 alibi).

**Status:** dedykowany plaster PÓŹNIEJ — gdy własność (L2) i panika (L0) dojrzeją. NIE w plastrze #2.

═══════════════════════════════════════════════════════════
# OCEAN → GŁÓD (mapa per cecha)
═══════════════════════════════════════════════════════════
| Cecha | Wpływ na głód | Warstwa |
|---|---|---|
| **Conscientiousness** | wysoka: je z wyprzedzeniem, gromadzi, racjonuje („idealny zbieracz"); niska: reaguje dopiero gdy boli. + opiera się łamaniu zasad (restraint) | 1 + 2 |
| **Neuroticism** | wysoka: ODCZUWA głód jako pilniejszy/dręczący (wzmacnia sygnał), pęka wcześniej; niska: znosi spokojnie. Ten sam lewar co rzut paniki | 1 + 2 |
| **Agreeableness** | wysoka: szanuje cudze jedzenie nawet w desperacji (próg kradzieży wysoko); niska: kradnie/kanibalizuje wcześnie | 2 (próg kradzieży) |
| **Openness** | drugorzędna: chęć spróbowania NIEZNANEGO źródła jedzenia (forage nowości, ryzyko zatrucia) | 1 (peryferyjnie) |
| **Extraversion** | drugorzędna: skłonność do dzielenia się / proszenia o jedzenie vs samotne zdobywanie | 3 (most do P2P) |

═══════════════════════════════════════════════════════════
# CZŁOWIECZEŃSTWO — rzut obronny przeciw instynktowi (spięcie)
═══════════════════════════════════════════════════════════
Z NPC_DEEPENING_concepts (szósty filar / saving-throw-vs-instinct). Głód to jego pierwsze konkretne pole:
- Kaskada pcha INSTYNKT (kradnij, kanibalizuj). Człowieczeństwo + OCEAN (A, C) to RESTRAINT.
- Skaluje z tym, jak WYSOKO na piramidzie NPC zwykle żyje (syty człowiek = więcej człowieczeństwa).
- DEGRADUJE pod głodówką (Minnesota) — to czyni tragedię emergentną, nie skryptowaną.
- Spięcia dalej: oprzeć się instynktowi „dla idei" = próg człowieczeństwa (most do prawdziwego imienia
  jako stawiania zasady ponad sobą; Grey Spring = erozja jaźni = anty-człowieczeństwo).

═══════════════════════════════════════════════════════════
# FELT vs TRUE + MASKI (spięcie z odłożonym konceptem)
═══════════════════════════════════════════════════════════
Głód realizuje felt-need vs true-state:
- **Sygnał odczuwany** (grelina-fale, OCEAN-pilność) ≠ **stan prawdziwy** (kaskada).
- **Maski** (NPC_DEEPENING): woda maskuje sygnał głodu bez kalorii (pełny żołądek, true bez zmian);
  w skrajności NPC zjada coś bezwartościowego, żeby ZGASIĆ sygnał, nie naprawiając stanu → crash.
- **Arbitraż** (NPC_DEEPENING): adresuj najbardziej OSIĄGALNĄ potrzebę w pobliżu, nie tylko najpilniejszą
  (woda blisko, jedzenie daleko → pij, gaś felt, true czeka). Most do EQS/Safe Zone.

═══════════════════════════════════════════════════════════
# PODZIAŁ C++ / BLUEPRINT (reguła projektu)
═══════════════════════════════════════════════════════════
## C++ (UMaslowBiologicalComponent — ROZSZERZAMY, nie nowy komponent; mózg):
- Kaskada faz (JUŻ jest). Sygnał odczuwany (grelina-oscylator) jako nowe pole/funkcja (warstwa 1, hook).
- `GetActionableNeed()` mapuje fazę → potrzeba (JUŻ jest dla Hunger=1).
- Warstwa 2: rzut pęknięcia (wzorzec `EvaluatePanicRoll`), restraint = człowieczeństwo × OCEAN, erozja.
- Modulacja OCEAN czytana z UNPCIdentityComponent (cache TWeakObjectPtr, bez FindComponent co tick — JUŻ wzorzec).
- UE_LOG na przejściach faz + na pęknięciu (kradzież/kanibalizm) — dla detektywa L5 i debugowania.

## Blueprint (przez BlueprintImplementableEvent — C++ woła, BP rysuje; ciało):
- OnHungerPang / OnEat — animacja, ikona.
- OnDesperationBreak — wizual pęknięcia (chwyt za cudzą skrzynię), C++ liczy CZY, BP gra JAK.
- Ruch do jedzenia (MoveTo), animacja jedzenia — wykonanie, nie decyzja.

═══════════════════════════════════════════════════════════
# OPTYMALIZACJA (religia — 500+ NPC)
═══════════════════════════════════════════════════════════
- **Warstwa 1:** wyzwalacz = porównanie float (Glucose vs próg) na ISTNIEJĄCYM ticku metabolizmu. Zero nowego ticku.
- **Warstwa 2 rzut:** 1× FRand() + kilka operacji float TYLKO dla NPC w głębokiej fazie (Autophagy/FatBurn) —
  zdecydowana większość NPC NIGDY nie rzuca (są syci). Wzorzec rzutu paniki/mikrosnu — dowiedziony tani.
- **Pamięć:** człowieczeństwo = 1 float/NPC (+ ew. 1 float erozji) = ~8 B × 500 = ~4 KB. Sygnał odczuwany
  (faza oscylatora) = 1 float/NPC = ~2 KB. OCEAN JUŻ jest (20 B/NPC). Łącznie znikome.
- **Reguła:** rzut warstwy 2 NIGDY per-frame; tylko na kadencji metabolizmu, tylko dla głodujących.

═══════════════════════════════════════════════════════════
# KONTRAKT WARSTW (jak to się tnie na plastry)
═══════════════════════════════════════════════════════════
| Warstwa | Co | Plaster | Dotyka |
|---|---|---|---|
| 1 | głód odczuwany → jedzenie (C++ Glucose steruje, re-attach Handle Hunger, OCEAN-pilność hook) | **#2 mostu (następny)** | most Maslow→BT |
| 2 | prawda kaskady → skrajność (rzut pęknięcia, restraint, erozja, kradzież/kanibalizm) | osobny plaster, PÓŹNIEJ | L2 własność, L0 panika, L3 tabu, L5 detektyw |

🔑 Kaskada `EHungerPhase` MUSI dalej liczyć fazy w warstwie 1 — to hook pod warstwę 2. NIE spłaszczać
   głodu do jednego wyzwalacza w sposób, który gubi fazy. Płytko (w1) kładzie tor pod głęboko (w2).

═══════════════════════════════════════════════════════════
# GDZIE SIĘ WPINA (mosty)
═══════════════════════════════════════════════════════════
- **L0 panika:** pęknięcie z desperacji może odpalić Fight-or-Flight (most do `bIsInPanic`).
- **L2 własność:** kradzież → `THEFT:` (już strzela) → skrzynia → detektyw. Kanibalizm → ciało jako „zasób".
- **L3 społeczność:** tabu kanibalizmu, strach, znoszenie nadwyżek (sumienny), dzielenie się (ekstrawertyk).
- **L4 reputacja:** złamanie tabu → reputacja −, strach u innych; gromadzenie → wizerunek zapobiegliwego.
- **L5 detektyw:** kradzież/kanibalizm widziane przez świadków → log akcji → alibi.
- **Świat Caldreth:** jedzenie w lasach zboczowych/wybrzeżu (geografia napędza wyprawy); susza → migracja.

═══════════════════════════════════════════════════════════
# OTWARTE PYTANIA (do rozstrzygnięcia PRZED gate'ami warstw)
═══════════════════════════════════════════════════════════
1. Grelina-oscylator: jak modelować fale wyuczone porami posiłków? (prosty sin z fazą uczoną, czy licznik?)
2. Erozja człowieczeństwa: liniowa z czasem w głębokiej fazie, czy progowa? Odbudowa po najedzeniu — jak szybko?
3. Kanibalizm: osobna akcja BT czy skrajny wariant kradzieży? Wymaga „ciała jako zasobu" (nowy item-type?).
4. Maski/arbitraż: w którym plastrze? (felt-vs-true to potencjalnie osobny duży temat, nie część w1.)
5. Próg kradzieży vs Ugodowość: formuła (Lerp jak panika?) i gdzie próg krytyczny fazy.
6. Weryfikacja warstwy 2: jakie twarde liczby z logu dowiodą „ten sam głód, inna reakcja" (jak killer-demo OCEAN)?

═══════════════════════════════════════════════════════════
# STAN PROJEKTU W MOMENCIE SZKICU (kontekst)
═══════════════════════════════════════════════════════════
GOTOWE (grunt pod ten model):
- Kaskada `EHungerPhase` (Glucose→Glycogen→FatBurn→Autophagy→Death) — liczy fazy, gotowy hook warstwy 2.
- Most Maslow→BT plaster #1 (pragnienie): C++ steruje, `GetActionableNeed` mapuje Nutrition→Hunger=1.
- OCEAN slice #1: FOceanProfile + GetNeuroticism, wzorzec rzutu (EvaluatePanicRoll) — szablon dla rzutu w2.
- Hook `THEFT:` w InventoryComponent (L2) — gotowe ujście kradzieży.
NASTĘPNE: plaster #2 = WARSTWA 1 (talk-first gate Executora, grounded w żywym kodzie). Warstwa 2 = później.
