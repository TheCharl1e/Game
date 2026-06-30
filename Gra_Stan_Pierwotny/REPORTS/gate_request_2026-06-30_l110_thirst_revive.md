# GATE-REQUEST — ożywienie L1-10 (skalowanie pragnienia aktywnością) · 2026-06-30

> **OD:** Executor (Claude Code) · **DO:** Architekt (Chat-Claude) + Dyrektor (Szymon)
> **Status:** RECON-STOP — verify zrobiony twardo, **kod/asset NIE ruszony**, czekam na gate.
> Bazuje na `REPORTS/raport_2026-06-30_l110_thirst_dead.md`. Nie implementuję do „zatwierdzam".

## 1. Co potwierdzone (twarde liczby, PIE 2026-06-30, świeży edytor)
- **Mechanizm burn × mult ŻYJE i jest liniowy:** `TotalHydrationBurn = HydrationBurnRatePerTick(2.0) × CurrentActionHydrationMultiplier` (cpp:172). Zmierzone na-tick: mult1 → **−2.0/tick**, mult5 → **−10.0/tick**, ratio **dokładnie 5.0×**.
- **Driver MARTWY:** `SetActionByRow` / `SetCurrentActionMultiplier` mają **zero call-sites** (C++ = tylko definicje; BP = zero trafień w `Content/`). `ActionCostTable = nullptr`; `DT_ActionCosts.uasset` istnieje, odpięty.
- **Wynik:** `CurrentActionHydrationMultiplier` na stałe **1.0** w grze → NPC w spoczynku, ruchu i walce spala wodę identycznie. Feature **inertny**.

## 2. ⚠️ KRYTYCZNA niespodzianka zakresu (architekt MUSI to rozważyć przed projektem)
`SetActionByRow(row)` ustawia **JEDNYM strzałem CZTERY** pola z `FActionCostRow`:
| pole | wpływa na | warstwa | status |
|---|---|---|---|
| `WaterMultiplier` | spalanie wody (pragnienie) | **L1-10 (cel)** | inertny |
| `KcalMultiplier` | spalanie energii (głód) | L1-02 | zweryfikowany żywy |
| `TempModifier` | temperatura | L1-09 | zweryfikowany żywy |
| `StaminaCostPerSecond` | stamina/prędkość | L1-08 | zweryfikowany żywy |

→ „Ożywienie" przez `SetActionByRow` **NIE jest zmianą tylko-pragnienia** — reaktywuje też kcal/temp/staminę naraz, czyli dotyka **3 już zweryfikowanych systemów**. Ryzyko regresji realne.

## 3. Decyzje do ZALOCKOWANIA (gate — NIE wybieram ich sam)
- **D1 — Driver:** kto i kiedy ustawia aktualną akcję na komponencie? Kandydaci: (a) C++ helper wołany z `BTTask` Start/Finish (wzorzec `StartEatingItem`); (b) C++ `BTService` czytający aktywny task/stan i ustawiający akcję co kadencję; (c) mapowanie z istniejącego `CurrentNeed`/gałęzi BT. = architektura mostu, analogiczna do MASLOW_BT_BRIDGE.
- **D2 — Taksonomia akcji → wiersze:** co liczy się jako `rest / work / combat` i co z resztą (idle, forage/MoveTo, eat, sleep, drink, **flee**). Spec mówi `rest×1 / work×3 / combat×5`, ale **systemu walki nie ma** (jest tylko Flee z L0) — Flee→×5 teraz, czy `combat` odłożyć?
- **D3 — Lifecycle / reset:** po zakończeniu akcji powrót do `rest(×1)` — kto resetuje (BTTask Finish? domyślny stan w service?). Bez tego mnożnik „zacina się" na ostatniej akcji.
- **D4 — Zakres mnożników (z §2):** **wąsko** (tylko `WaterMultiplier` przez `SetCurrentActionMultiplier`, reszta nietknięta) czy **szeroko** (pełny action-cost przez `SetActionByRow` — ożywia kcal/temp/stamina). Jeśli szeroko → **wymóg re-verify L1-02 / L1-08 / L1-09** (brak regresji).
- **D5 — Dane:** przypisać `ActionCostTable = DT_ActionCosts` na BP_NPC_Character (manual save, ryzyko RF_Transient) + zapełnić wiersze + wartości mnożników per akcja.

## 4. Powierzchnia implementacji (po „zatwierdzam" — żeby architekt znał koszt)
- C++: wpięcie wołania settera (patch, nie regen — wzorzec helpera jak `StartEatingItem`). Build editor-closed UHT.
- `DT_ActionCosts`: wiersze + przypisanie tabeli na BP (manual save usera).
- Jeśli D1=service: nowy C++ `BTService` albo rozszerzenie `BTService_MaslowBlackboardSync`.
- **Perf ×500:** 1 `set`/akcję, **zero nowego ticka** (jedzie na istniejącej kadencji metabolizmu / zdarzeniach BT). Brak per-NPC iteracji.
- Verify: per-tick drenaż wody per akcja (metoda jak dziś — krokowo) + (jeśli D4=szeroko) brak regresji kcal/temp/stamina.

## 5. Rekomendacja executora (opinia, NIE decyzja — architekt rozstrzyga)
Wariant **wąski na start (D4=tylko Water)**: ożywia L1-10 bez ruszania 3 zweryfikowanych systemów; pełny action-cost (kcal/temp/stamina) jako osobny, późniejszy gate z własnym re-verify. Minimalizuje powierzchnię regresji przy pierwszym wpięciu drivera.

## 6. Czego NIE robię
Zero kodu/assetów do czasu zatwierdzonego gate'u. Nie wybieram wartości mnożników, drivera ani taksonomii akcji. `DT_ActionCosts` i BP nietknięte.
