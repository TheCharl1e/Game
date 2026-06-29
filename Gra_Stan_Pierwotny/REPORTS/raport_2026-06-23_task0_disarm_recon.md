# Raport — TASK 0 (disarm) RECON · 2026-06-23

> Recon przed kodem (most Maslow→BT = fundament blokujący). Read-only na ŻYWYM grafie
> (świeży edytor = prawda dyskowa). Wynik: **STOP-AND-FLAG — disarm JUŻ WYKONANY.**

## Zadanie
TASK 0: rozbroić BP `EvaluateNeeds` — odpiąć drugi-writer klucza Blackboard `CurrentNeed`,
tak by C++ był jedynym pisarzem decyzji.

## Sprzeczność na wejściu (3 źródła, 3 stany — dlatego recon obowiązkowy)
| Źródło | Twierdzi |
|---|---|
| ROADMAP TECH-08 (plaster #1) | BP zapis klucza **ZAMROŻONY** (odpięty exec, węzły zostają) — 06-20 |
| Kod C++ `BTService_MaslowBlackboardSync.cpp:76-78` | 4× SetValueAsEnum **USUNIĘTE**, disarm **finished 2026-06-22** |
| Pamięć `bp-need-brain-zombie` | BP **wciąż** pisze CurrentNeed, plaster #1 **INCOMPLETE** |

## Co pokazał żywy graf (Monolith, `/Game/DocelowaGra/AI_NPC/BP_NPC_Character`)

**Twarde liczby:**
- `search_nodes "Set Value as Enum"` w całym BP → **0 trafień.** Żaden węzeł zapisu klucza
  Blackboard `CurrentNeed` nie istnieje. **Kod C++ ma rację — SetValueAsEnum zostały USUNIĘTE, nie tylko odpięte.**
- `get_graph_data EvaluateNeeds`: 4× `Set CurrentNeed` (VariableSet, BP-zmienna) — **wszystkie mają
  `execute` input `connected_to:[]`** (odpięte, nigdy nie odpalą). Główny łańcuch `Entry→IfThenElse_1→_2→_0`
  liczy warunki potrzeb, ale **wszystkie wyjścia exec (`then`/`else`) puste** → funkcja dożynkuje w nicość.
- Sieroty po usuniętym SetValueAsEnum: `Make Literal Name "CurrentNeed"` (×4) + `Get Blackboard`
  (ReturnValue `connected_to:[]` — nic nie konsumuje) + `Get Controller`. Martwe, nieszkodliwe.
- `EvaluateNeeds` **wciąż wołane** raz z EventGraph (`K2Node_CallFunction_7`), ale bez efektu (nic nie pisze).
- Konsument C++: `BTService_MaslowBlackboardSync.cpp:79` → `BB->SetValueAsEnum(CurrentNeedKey, GetActionableNeed())`
  co ~1 s. **Jedyny żywy pisarz klucza.**

**Uwaga narzędziowa:** `find_variable_references CurrentNeed` zwróciło 0 (reads+writes) — to znany
BUG Monolith (potwierdzony w recon AmbientTemp). Autorytatywne = `search_nodes` (= 4 VariableSet). Nie ufać find_variable_references.

## WNIOSEK
**TASK 0 był już wykonany, zanim został zlecony.** Drugi-writer klucza Blackboard `CurrentNeed`
nie istnieje w grafie (4× SetValueAsEnum usunięte, finished 2026-06-22). C++ `BTService_MaslowBlackboardSync`
jest genuinnie JEDYNYM pisarzem. Fundament mostu pod TASK 1/2/3 stoi — strukturalnie czysty.

Pamięć `bp-need-brain-zombie` była STALE ("BP still 2nd writer / plaster #1 incomplete") — **poprawiona 2026-06-23.**

## Pozostałości (martwe, NIE w zakresie disarm — decyzja dyrektora)
Cosmetic cleanup, NIE konieczny do działania mostu:
- 4× odpięte `Set CurrentNeed` (VariableSet, BP-zmienna, 0 czytelników) — wyspy bez wejścia exec.
- Sieroty `Get Blackboard`/`Get Controller`/`Make Literal Name "CurrentNeed"` po usuniętym SetValueAsEnum.
- `EvaluateNeeds` całe = zombie (wołane, nic nie robi). Pełne usunięcie = osobny task (dotyka zapisu
  BP_NPC_Character — RF_Transient-risk; granica nietykalna). Park jako dług, nie rób na ślepo.

## REKOMENDACJA / co dalej
1. **TASK 0 = DONE (already)** — bez zmian w kodzie/asset. Disarm istniał na dysku.
2. (opcjonalnie) **Certyfikacja runtime** świeżym PIE: potwierdzić, że BB `CurrentNeed` zmienia się
   WYŁĄCZNIE za C++ (psuję Glucose→Hunger / nasycam→0; BP HungerLevel nietknięty). Strukturalnie
   pisarza-konkurenta NIE MA, więc runtime nie ujawni nowego; plaster #1 (06-20) już to PIE-udowodnił.
   Robię jeśli dyrektor chce twardego runtime-stempla pod fundament.
3. **Czekam na payload TASK 1** (głód) — nie dostałem go jeszcze.
