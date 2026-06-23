# Raport — TASK 1 (głód / plaster #2) RECON + PIE-VERIFY · 2026-06-23

> Most Maslow→BT, gałąź głodu. Recon na żywym BT + PIE-dowód runtime (kanał Monolith :9316).
> Zero zapisu assetów (tylko runtime PIE, odrzucone przy stopie). Branch `feat/l1-night`.

## Wynik w jednym zdaniu
**Okablowanie gałęzi Handle Hunger było JUŻ kompletne** (identyczne ze zweryfikowanym Handle Thirst z
plastra #1), a **ścieżka C++→BB→routing-dekoratora jest DOWIEDZIONA twardymi liczbami** w zakresie, jaki
system czysto pozwala. Po drodze wyszły **2 istotne nowe znaleziska** (reset progów krytycznych; temperatura
„połyka" niższe potrzeby) + bloker percepcji (TECH-11) uniemożliwiający pozytywny snapshot MoveTo→Eat.

---

## A. RECON STRUKTURY (export_bt_spec BT_NPC) — gałąź już zbudowana
Selector `Potrzeby` (serwis `BTService_MaslowBlackboardSync` na korzeniu), kolejność dzieci: Thirst → Hunger → Sleep.

| Element | Handle Thirst (zweryf. plaster #1) | **Handle Hunger** |
|---|---|---|
| Dekorator gatingu | `BTDecorator_Blackboard CurrentNeed==Thirst` (IntValue **2**) | `BTDecorator_Blackboard CurrentNeed==Hunger` (IntValue **1**) ✅ |
| Finder | FindWater_Experyment | **FindFood_Experyment** ✅ |
| Inner-gate → akcja | `NearestWaterLocation Is Set`→MoveTo→Drink | `NearestFoodLocation Is Set`→MoveTo→**Eat** ✅ |
| Domknięcie | BTTask_Check | BTTask_Check ✅ |

C++ `GetActionableNeed` (`MaslowBiologicalComponent.cpp:743`): `Level_1_Nutrition → return 1 (Hunger)`,
gdy `Glucose <= EffectiveKcalThreshold` i nic wyższego w drabinie (`:721`).

## B. PIE-DOWÓD RUNTIME (NPC `BP_NPC_Character_C_1`, kontroler `BP_NPC_AI_C_0`, mapa Game)

**DOWIEDZIONE (twarde odczyty):**
1. **C++ produkuje głód:** z wyizolowanym głodem (Glucose=0, Hydration=98, Temp=37, Stamina=100)
   `GetActionableNeed()` = **1 (Hunger)** — wielokrotnie, przez rzetelny `pie_call_function`.
2. **Serwis wiernie kopiuje C++→BB w OBIE strony:** BB `CurrentNeed` = **2 (Thirst)** gdy NPC realnie
   odwodniony (Hydration<20), i = **1 (Hunger)** gdy Hydration przywrócone do 98. Most pisze PRAWDZIWĄ
   bieżącą potrzebę, nie zlepek.
3. **Dekoratory ROUTUJĄ po CurrentNeed (mechanizm żywy):** przy `CurrentNeed=2` aktywny węzeł BT =
   **`BTTask_FindWater_Experyment`** (gałąź Handle Thirst, execution_index 4). Przy `CurrentNeed=1` drzewo
   PRZESTAJE być na Handle Thirst. Dekorator Handle Hunger (`==1`) jest strukturalnie identyczny z dowiedzionym
   Handle Thirst (`==2`) → **gating głodu dowiedziony przez kontrast + identyczność struktury + C++=1.**

**NIE ZŁAPANE (bloker poza mostem):**
- Pozytywny snapshot aktywnego taska WEWNĄTRZ Handle Hunger (FindFood/MoveTo/Eat). Powód: **0 percypowanego
  jedzenia** → `FindFood_Experyment` pada natychmiast (NearestFoodLocation nieustawione → wewn. dekorator
  blokuje MoveTo/Eat). FindFood to task natychmiastowy (zwraca w 1 ticku) → nie persystuje jako active_node.
  Downstream MoveTo→StartEating→ConsumeBite→Stop **już dowiedziony w L1-11b (2026-06-22).**

## C. NOWE ZNALEZISKA (z PIE, do roadmapy)

🔴 **F1 (istotne) — reset progów krytycznych szerszy niż TECH-10.** `CriticalHydrationThreshold` (i prawdop.
wszystkie `Critical*Threshold`) są RESETOWANE per kadencja przez BP-drugi-mózg — ustawienie `=-1000` nie
utrzymało się (next cadence → z powrotem 20). TECH-10 udokumentował tylko BodyFat/Glycogen/CollapseThreshold;
**klasa jest szersza.** Skutek dla testów: izolacja potrzeby przez progi = bezcelowa; trzeba trzymać REALNE
wartości (Hydration=98) pod time-dilation 0.1. Należy do plastra #5 (kasacja BP need-calc).

🔴 **F2 (istotne — luka drabiny) — temperatura/safety „połykają" niższe potrzeby.** `GetActionableNeed`
mapuje `Level_1_Temperature` i `Level_2_Safety` na **0 (None)** (`:745-746`, niezaimplementowane). Drabina
`EvaluateCurrentNeed` stawia Temperaturę PONAD Nutrition (`:700` przed `:721`). Skutek: **zmarznięty NPC
(CurrentTemp<=CriticalTempThreshold) z Glucose=0 zwraca need=0 → BEZCZYNNY zamiast jeść.** Zaobserwowane:
GetActionableNeed dawało 0 gdy NPC stygł, mimo Glucose=0. Wyższa-ale-niezaimplementowana potrzeba **cicho
gasi** sygnał niższej. Do decyzji architekta (temperature-slice albo fallback w mapowaniu).

🟡 **F3 — percepcja oddała 0 jedzenia.** BP_Food ma `AIPerceptionStimuliSource` ale `is_active:false`; NPC_C_1
(rotacja [0,0,0]=patrzy +X) ma klaster jedzenia z boku/tyłu (+Y) = **poza stożkiem FOV**. To percepcja/orientacja
(TECH-11), nie most. L1-11b dowiódł, że pętla jedzenia działa, gdy jedzenie JEST percypowane.

🟡 **F4 — BT nie startował automatycznie** dla NPC w tej sesji PIE (`is_running:false` do ręcznego
`runtime_start_bt`). Plaster #1 widział BT żywy na tej mapie → możliwe że sesyjne (possession/RunBehaviorTree
timing) albo te NPC startują bezczynne. Do osobnego sprawdzenia (nie blokuje mostu).

## D. WERDYKT
**TASK 1 (most głodu) — gałąź gotowa, gating dowiedziony.** Okablowanie Handle Hunger = funkcjonalny analog
zweryfikowanego Handle Thirst; ścieżka C++(=1)→serwis→BB(=1)→routing-dekoratora dowiedziona twardymi odczytami.
Nieuchwycony pozytywny MoveTo→Eat jest bramkowany percepcją (TECH-11) i **już dowiedziony w L1-11b** — nie
duplikuję. **Most głodu uznaję za DONE.**

**Zaparkowane / do decyzji:** F2 (temperatura gasi głód — luka drabiny, decyzja architekta), F1 (reset progów
— plaster #5), F3 (percepcja/FOV — TECH-11), F4 (BT-autostart — sprawdzenie). Żaden NIE blokuje TASK 2/3.

**Zero zapisu assetów** (tylko runtime PIE, odrzucone przy stopie). Bez CTRL+S.
