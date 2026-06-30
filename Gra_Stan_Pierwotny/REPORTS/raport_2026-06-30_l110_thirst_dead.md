# Raport — L1-10: skalowanie pragnienia aktywnością — MARTWE (verify) · 2026-06-30

> Zadanie dyrektora: twardo sprawdzić, czy skalowanie spalania wody aktywnością
> (`rest×1 / work×3 / combat×5`) ŻYJE w runtime. Read-only verify, zero zmian kodu/assetów.
> Świeży edytor (anti-`TRASH_`), PIE na `Game.Game` (2 NPC), odczyt/iniekcja przez MCPUnreal Python.

## Wniosek
**Mechanizm spalania jest podpięty i POPRAWNY, ale sterownik aktywności NIE ISTNIEJE** →
`CurrentActionHydrationMultiplier` na stałe **1.0** w realnej grze. NPC w spoczynku, w ruchu
i w walce spala wodę identycznie (`2.0/tick`). Feature jest **inertny**, nie „present".
To ten sam odpięty drut, który PROJECT_STATE §5 Problem 1 opisał 2026-06-07 (DT_ActionCost
zdefiniowany, nigdy nieczytany) — od tamtej pory niezmienione.

## Ślad statyczny (rozstrzygający)
- Spalanie: `MaslowBiologicalComponent.cpp:172` — `TotalHydrationBurn = HydrationBurnRatePerTick(2.0) × CurrentActionHydrationMultiplier`.
- Mnożnik zmieniają TYLKO `SetActionByRow` (czyta `Row->WaterMultiplier` z `ActionCostTable`) i `SetCurrentActionMultiplier` (cpp:320/326).
- **Zero wołających:** C++ = tylko definicje (grep całego `Source/`); BP = **zero trafień** w całym `Content/` (grep `.uasset` po `SetActionByRow`/`SetCurrentActionMultiplier`/`ActionCostTable`).
- `ActionCostTable = nullptr` domyślnie; żaden BP nie przypisuje. `DT_ActionCosts.uasset` istnieje (`Content/DocelowaGra/Data/`), wisi odpięty.
- Jedyne zapisy `CurrentActionHydrationMultiplier` w cpp: konstruktor `=1.0` (L45) + dwa martwe settery (L323/L343).

## PIE-VERIFY (twarde liczby, żywa instancja `MaslowBiological`, nie-`TRASH_`)
Dwa NPC w `Game.Game`, oba aktywnie eksplorują (`LogExploration: wander fallback -> reachable point`).

**Baseline (przed iniekcją):** oba `hydMult=1.0`, `hyd=86.00`, `burn=2.0` — identyczny drenaż 100→86
MIMO ruchu (akcja „work"), mnożnik nietknięty. = sterownik nie działa.

**Test różnicowy (C_1 wstrzyknięty `hydMult=5.0`, C_2 kontrola `1.0`, oba reset `hyd=100`):**
Próbkowanie gęstsze niż interwał (10 s) → bezpośredni odczyt rozmiaru pojedynczego kroku (= drenaż na tick,
zero dzielenia przez zgadywaną liczbę kadencji). Wartości układają się w idealne kroki, oba NPC w lockstepie:

| worldtime | C_1 (mult 5) | C_2 (mult 1) | krok C_1 | krok C_2 |
|---|---|---|---|---|
| 212.38 | 90.00 | 98.00 | — | — |
| 228.38 | 70.00 | 94.00 | −20 (2 ticki) | −4 (2 ticki) |
| 242.72 | 60.00 | 92.00 | −10 (1 tick) | −2 (1 tick) |

**Tempo na tick (bezpośrednio):** C_2 = **−2.0/tick** (`HydrationBurnRatePerTick(2.0) × 1.0`) · C_1 = **−10.0/tick** (`2.0 × 5.0`). **Ratio dokładnie 5.0×.**
(Pierwszy pomiar oknem asynchronicznym dał brzydkie 5.25× / C_1 clamp do 0 — artefakt przesunięcia fazy timerów; krokowo widać czysty 5×.)

- **Mechanizm ŻYJE:** mnożnik skaluje burn co do cyfry (−10.0/tick vs −2.0/tick = 5×).
- Mnożnik=5.0 **UTRZYMAŁ SIĘ** przez round-trip (zero resetu) → TECH-10 ostatecznie potwierdzony jako artefakt ghosta, nie BP-writer (zgodne z reverify 2026-06-23).
- Iniekcja runtime-only na instancji PIE → zniknęła ze stopem PIE, **zero zapisu na dysk**.

## Status / dalej
- ROADMAP L1-10: `🔨 "Verify present"` → **⬜ "INERT — mechanizm OK, brak sterownika"**.
- **Naprawa = GATE ARCHITEKTA** (zmiana mechaniki, nie wykonana z marszu): wołać `SetActionByRow`
  z BT/metabolizmu przy zmianie akcji + przypisać `ActionCostTable=DT_ActionCosts` na BP_NPC_Character
  + zapełnić wiersze (rest/work/combat z `WaterMultiplier` 1/3/5). Bliźniaczo do długu „DT_ActionCost odpięty".
- CTRL+S rano: **brak** (verify read-only, zero zmian kodu/assetów; iniekcja PIE transient).
