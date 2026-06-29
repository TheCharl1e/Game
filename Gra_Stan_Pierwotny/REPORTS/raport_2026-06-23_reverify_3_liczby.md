# Raport — czysty re-verify fundamentu fizjologii (3 liczby) · 2026-06-23

> Na polecenie dyrektora: świeży edytor + POPRAWNA injekcja na nie-TRASH_ instancję, domknięcie trzech dziur,
> których CC nie złapał przez ghost-churn. Teraz złapane. Branch `verify/physiology-reverify`.

## Klucz: niezawodna technika injekcji (odkryta dziś)
- **`get_component_by_class` (singular) → `TRASH_MaslowBiologicalComponent_N` (ghost)** nawet na świeżym edytorze.
- Żywa instancja = nazwana dokładnie `MaslowBiological`. Resolve: `[x for x in actor.get_components_by_class(C) if 'TRASH_' not in x.get_name()][0]`, **re-resolve PRZED każdym set** (multi-set na jednym uchwycie churnuje).
- Sety WTEDY TRZYMAJĄ. `pie_call_function`/`ai_query runtime_*` czytają żywą instancję. Pamięć: [[trash-reinstancing-fresh-editor-for-pie-verify]] (przepis dopisany).

## ✅ M1 — gradient staminy L1-08 (liczba, której CC nie złapał przez cały L1-08)
Żywa instancja `MaslowBiological`: `CurrentStamina=30`, `StaminaRegenPerTick=0`, `HoursAwake=2`, flagi czyste.
Po jednej kadencji `ProcessMetabolism`:
- **`MaxWalkSpeed = 390.0`** = dokładnie `600 × GetStaminaSpeedMultiplier(30)` = `600 × 0.65`. ✓
- `CurrentStamina = 30.0` **UTRZYMANE** (zero resetu do 100) — ostateczny dowód, że TECH-10 „reset" był ghostem, nie BP-writerem.

→ **L1-08 NAPRAWDĘ zweryfikowany.** Feature realny, nie bug. Wcześniejsze nieuchwycenie = 100% artefakt `get_component_by_class`.

## ✅ M2 — przerwanie snu przez panikę (TASK 3)
- **Pre:** `active_node = BTTask_Sleep` (NPC śpi, HoursAwake=18).
- `apply_damage` (causer C_2) → threat. Dekorator Flee (`CurrentNeed==4`, `FlowAbortMode=Both`) **abortuje sen**.
- **Post (przejście):** `active_node` przeszło `BTTask_Sleep → BTTask_Flee → Move To` (gałąź Flee).
- **CZYSTY SIMULTANICZNY SNAPSHOT (jeden odczyt, świeży threat + restart BT):**
  **`IsThreatActive=true` ∧ `CurrentNeed=4` ∧ `active_node=BTTask_Flee`** — wszystkie trzy naraz. ✓

→ **Panika przerywa BT i przejmuje (sen→flee) — dowiedzione twardo, jeden spójny snapshot.**

## ✅ M3 — dekorator enum (IntValue-vs-display)
- Przy `CurrentNeed=4` (threat aktywny) → `active_node=BTTask_Flee` (gałąź Flee wchodzi). ✓
- Gdy threat wygasł (`CurrentNeed=3`) → restart BT → `active_node=BTTask_Sleep` (Sleep, NIE Flee). ✓
- → Dekorator **gatuje poprawnie po WARTOŚCI** (4→Flee, 3→Sleep). `IntValue=4` jest autorytatywne; `CachedDescription="None"` to tylko nieodświeżony display (kosmetyka TECH-12). **Wątpliwość zamknięta.**
- (Szybkie `active=None` przy powtórkach = MoveTo pada na ograniczonym navmeshu mapy Game → gałąź cykluje;
  `FleeLocation` pisany co cykl. To nav, nie dekorator.)

## WERDYKT
**Fundament warstwy fizjologii zweryfikowany czysto, po raz pierwszy wiarygodnie** (znany fix na ghosta):
L1-08 gradient (390), sleep-interrupt (jeden snapshot 4∧Flee∧threat), dekorator (powtarzalny gating po wartości).
TECH-10 ostatecznie potwierdzone jako artefakt. **Zero zmian w kodzie/asset** — to był pomiar. CTRL+S rano: brak.
Teraz można budować L1-09 na sprawdzonym fundamencie.
