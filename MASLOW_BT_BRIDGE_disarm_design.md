# MASLOW/BT BRIDGE — ROZBROJENIE ZOMBIE (DESIGN GATE)
> Projekt do zatwierdzenia (architekt + Szymon, 2026-06-22). Dokańcza plaster #1 Bridge'a, który
> NIE usunął BP-pisarza decyzji. To robota BP (graf `BP_NPC_Character` + `WBP_NPC`), zero nowej logiki C++.
> Zasady naczelne: **ONE KEY / ONE WRITER** (klucz `CurrentNeed` ma mieć JEDNEGO pisarza) +
> **FREEZE NOT DELETION** (odpinamy zasilanie legacy loopu, nie kasujemy funkcji).
>
> 🔴 SEDNO: legacy BP-mózg to ZOMBIE — okablowany i wciąż piszący współdzielony klucz decyzyjny
> `CurrentNeed`, ale runtime-frozen (C++ wygrywa cadence'em). Uśpiona mina: gdy poziomy BP ruszą
> albo bind zacznie łapać, `EvaluateNeeds` zacznie stompować decyzję C++ co godzinę gry.

═══════════════════════════════════════════════════════════
# STAN WEJŚCIOWY (z recona 2026-06-22 — twarde, live Monolith)
═══════════════════════════════════════════════════════════
WERDYKT: **ZOMBIE** (wired + pisze klucz decyzyjny, ale runtime-frozen → de-facto nie steruje; C++ steruje).

- **Podwójny pisarz `CurrentNeed`** [SRC/live graf]: `EvaluateNeeds` (BP) to drabina priorytetów na
  `HungerLevel/ThirstLevel/SleepLevel/SaftyLevel` vs progi → każda gałąź robi
  `SetValueAsEnum(KeyName="CurrentNeed", E_NeedState)` — **4× `Make Literal Name="CurrentNeed"`**.
  Pisze TEN SAM klucz blackboardu, co C++ `BTService_MaslowBlackboardSync` (co 1.0 s,
  `CurrentNeed = GetActionableNeed()`, cpp:77).
- **Driver żywy** [SRC/live graf]: `BP_NPC_Character.EventGraph`: `On Hour Passed` (TimeManager=`BP_DayNightCycle`,
  realnie broadcastuje) → `IncreaseHunger` → `Set HungerLevel` → `Print String`. Łańcuch kompletny.
  `BeginPlay` woła `CharacterStats` raz (init progów/poziomów). `has_tick=false`, brak timera.
- **Runtime-frozen** [LIVE]: po ~4 h gry BP `HungerLevel/ThirstLevel/SleepLevel` STOJĄ na 70/70/40 (init),
  `BP.CurrentNeed=NONE`, a C++ `Glucose` spada (990→970). Increment BP się nie wykonuje (prawdopodobnie
  `OnHourPassed` jako ComponentBoundEvent do runtime-aktora nie łapie — mechanizm do potwierdzenia,
  hard-fakt = poziomy stoją).
- **Komentarz kłamie** [SRC]: `BTService_MaslowBlackboardSync` l.76/88 „C++ is now the sole writer" —
  NIEAKTUALNE (BP `EvaluateNeeds` wciąż pisze ten sam klucz).
- **Print spam** [SRC]: `EvaluateNeeds` (Hunger/Thirst/Sleep/None), `IncreaseHunger`, `WBP_NPC` (Hello) — na ekran.
- **Residual nie domknięty** [otwarte]: `isSleeping` (BP) vs `bIsSleeping` (C++) — co czyta animacja/BT snu
  (`BT_NPC` nieczytelny przez `blueprint_query`). BT czyta blackboard `CurrentNeed` (Sleep=3 z C++).

═══════════════════════════════════════════════════════════
# DECYZJE PROJEKTOWE (do zatwierdzenia — NIE zmieniać bez architekta)
═══════════════════════════════════════════════════════════
| # | Decyzja | Wartość |
|---|---|---|
| D-WRITER | Podwójny pisarz | **Usuń 4× `SetValueAsEnum("CurrentNeed")` z `EvaluateNeeds`.** Po tym C++ jest realnie jedynym pisarzem klucza. Bezpośrednie złamanie miny. |
| D-FREEZE | Legacy driver | **Odepnij `On Hour Passed → IncreaseHunger`** (freeze zasilania loopu). Funkcji NIE kasujemy — zostaje martwa-ale-obecna (reversibility). |
| D-DEPTH | Dlaczego oba | **Defense in depth:** D-WRITER chroni, gdyby poziomy ruszyły; D-FREEZE chroni, gdyby writer wrócił. Dwie niezależne bariery. |
| D-SPAM | Print spam | **Wytnij Print String** z `EvaluateNeeds` (Hunger/Thirst/Sleep/None), `IncreaseHunger`, `WBP_NPC` (Hello). |
| D-RESIDUAL | Sen BP vs C++ | **Recon `isSleeping` PRZED dotknięciem.** Jeśli animacja/BT czyta BP `isSleeping` → przełącz odczyt na C++ `bIsSleeping` (mostek). Jeśli nikt nie czyta → freeze jak reszta. Patrz pre-flight. |
| D-COMMENT | Kłamiący komentarz | **Popraw komentarz** w `BTService_MaslowBlackboardSync` — po disarmie „sole writer" staje się PRAWDĄ; odnotuj, że BP-writer usunięty (plaster #1 dokończony 2026-06-22). |
| D-KEEP | Co zostaje | **NIE kasujemy** `EvaluateNeeds/IncreaseHunger/CharacterStats/ScanForFood` ani BP-pól — freeze in place. C++ już rządzi, nic nie budujemy od nowa. |

═══════════════════════════════════════════════════════════
# CO TEN ETAP ROBI / CZEGO NIE
═══════════════════════════════════════════════════════════
✅ ROBI:
- Usuwa 4× BP-writer współdzielonego klucza `CurrentNeed` (`EvaluateNeeds`).
- Odpina driver `OnHourPassed→IncreaseHunger` (freeze legacy loopu).
- Wycina Print spam (3 miejsca).
- Domyka residual snu (recon + ew. mostek `isSleeping`→`bIsSleeping`).
- Prostuje kłamiący komentarz „sole writer".
- Efekt: `CurrentNeed` ma JEDNEGO pisarza (C++), plaster #1 Bridge'a realnie dokończony.

❌ NIE ROBI:
- Nie kasuje legacy funkcji/pól BP (freeze, nie delete — reversibility).
- Nie buduje nowej logiki C++ — C++ już liczy i rządzi wszystkimi trzema potrzebami poprawnie.
- Nie dotyka inspektora ani feedu (osobne tory; ten gate je ODBLOKOWUJE, czyszcząc źródło).
- Nie naprawia minute-source w `BP_DayNightCycle` (osobny drobiazg, nie ten system).

═══════════════════════════════════════════════════════════
# PERF
═══════════════════════════════════════════════════════════
Trywialne — usuwanie/odpinanie węzłów BP. Zero kosztu runtime, wręcz MNIEJ (znika Print spam co godzinę gry
+ co klatkę „Hello"). Nie w ścieżce 500 NPC w żaden nowy sposób.

═══════════════════════════════════════════════════════════
# WERYFIKACJA (Monolith live, PIE — twarde, zero placeholderów)
═══════════════════════════════════════════════════════════
1. **One writer:** graf `EvaluateNeeds` ma **0× `SetValueAsEnum("CurrentNeed")`** (było 4×).
   Jedyny pisarz klucza w projekcie = `BTService_MaslowBlackboardSync` (C++).
2. **Driver odpięty:** `On Hour Passed` nie ma już połączenia exec do `IncreaseHunger` (graf-check).
3. **Spam zniknął:** PIE — brak „Hunger/Thirst/Sleep/None" i „Hello" na ekranie/w logu.
4. **Regresja C++ nietknięta:** NPC dalej działa wg C++ — `GetActionableNeed()→CurrentNeed→BT`.
   Wymuś scenariusz (np. C++ głód poniżej progu) → `CurrentNeed` ustawia C++, zachowanie poprawne.
   Kontr-test: wymuś BP `HungerLevel` wysoko → `CurrentNeed` SIĘ NIE ZMIENIA z tego (writer usunięty).
5. **Residual snu:** rozstrzygnięty — animacja/BT snu czyta C++ `bIsSleeping` (nie BP `isSleeping`),
   albo potwierdzone, że BP `isSleeping` nikt nie czyta.
6. **Komentarz prawdziwy:** `BTService_MaslowBlackboardSync` odzwierciedla rzeczywistość (sole writer = fakt).

═══════════════════════════════════════════════════════════
# REGUŁA PATCHOWANIA
═══════════════════════════════════════════════════════════
Usuwanie/odpinanie węzłów w istniejących grafach — NIE regeneracja `BP_NPC_Character`/`WBP_NPC`.
⚠️ Zapis defaultów/assetów BP: ryzyko RF_Transient/Standalone strip przez Monolith → Ctrl+S ręcznie Szymon.
⚠️ Edycja grafu BP przez Monolith ostrożnie; `EvaluateNeeds` to rdzeniowa funkcja decyzyjna legacy —
jeśli ryzyko przy wycinaniu 4× SetValueAsEnum, rób wizualnie w edytorze, ZERO blind na tej funkcji.

═══════════════════════════════════════════════════════════
# 🔑 INSTRUKCJA DLA CLAUDE CODE — PRE-FLIGHT (POTWIERDŹ PRZED EDYCJĄ)
═══════════════════════════════════════════════════════════
1. 🔴 **RESIDUAL SNU (blokuje D-RESIDUAL):** co czyta BP `isSleeping`? Sprawdź animację (AnimBP),
   BT/serwisy, inne grafy. Czy ktokolwiek czyta BP `isSleeping` do decyzji/animacji, czy wszystko
   idzie z C++ `bIsSleeping`? Jeśli BP `isSleeping` czytane → potrzebny mostek (przełącz na `bIsSleeping`).
2. **KTO WOŁA `EvaluateNeeds`?** Potwierdź callerów. Jeśli `EvaluateNeeds` jest jeszcze wołane po
   usunięciu writera — to OK (drabina liczy, ale nie pisze klucza), ale potwierdź, że BP var `CurrentNeed`
   (NIE blackboard) nikt nie czyta do DECYZJI (tylko `WBP_NPC` display). Jeśli ktoś czyta BP var do decyzji
   → FLAGUJ.
3. **POTWIERDŹ lokalizację 4× `SetValueAsEnum("CurrentNeed")`** w `EvaluateNeeds` (dokładne węzły) przed wycięciem.
4. Patch, nie regeneracja. Funkcji nie kasuj (freeze). STOP po weryfikacji bloków 1–6.
