# Raport weryfikacyjny — OCEAN L3-02 slice #1 (Neurotyczność → panika)

**Data:** 2026-06-20
**Gate:** OCEAN_L3-02_design.md (talk-first, zatwierdzony przez dyrektora; flagi A–E rozstrzygnięte)
**Zakres:** slice #1 — Neurotyczność przesuwa SZANSĘ paniki L0 (stochastyczny rzut, latch, histereza)

---

## Wynik: ✅ MECHANIZM DOWIEDZIONY · ⚠️ UŚPIONY W GRZE (TECH-08)

Wszystkie ogniwa łańcucha cecha→zachowanie zweryfikowane twardymi liczbami z żywego PIE.
Funkcja jest poprawna i kompletna kodowo, ale **nie wyzwoli się w realnej grze** dopóki nie naprawimy
**TECH-08** (BP_NPC pinuje HP=100 co klatkę → HP% zawsze 1.0 → rzut nigdy nie wchodzi w pasmo).

---

## Build
- Editor closed → `Build.bat Stan_PierwotnyEditor Win64 Development` (UE 5.7, E:\UE_5.7).
- `Result: Succeeded`, 21 s. Skompilowane: `NPCIdentityComponent.cpp`, `MaslowBiologicalComponent.cpp`,
  UHT przetworzył `FOceanProfile` (nowy USTRUCT), oba DLL zlinkowane. 0 ostrzeżeń z kodu OCEAN.

## Killer-demo (PIE, twarde liczby z LogMaslow na dysku)
Dwa NPC, ręcznie ustawiona Neurotyczność, identyczny stan: `HP=35/100` → **HP%=0.35**.
Telemetria z tej samej klatki (#946, 10:26:26 UTC):

| NPC | Neuroticism | EffThr | PanicChance | rolled | latched | `[Panic] ENTER` |
|---|---|---|---|---|---|---|
| **C1_NEUROTIC** (`_C_1`) | 0.90 | 0.52 | **0.25** | **1** | **1** | ✅ TAK |
| **C2_STABLE** (`_C_0`) | 0.10 | 0.28 | **0.00** | 0 | 0 | ❌ NIE |

- Linie `ENTER stochastic panic` w całym logu: **1** (tylko C1). C2: **0**.
- Surowe: `[Panic:BP_NPC_Character_C_1] ENTER stochastic panic — Neuroticism=0.90 EffThr=0.52 HP%=0.35 PanicChance=0.25.`
- **„Ta sama rana, inna reakcja"** — stabilny NPC ma matematycznie ZEROWĄ szansę paniki przy 35% HP;
  neurotyczny ma realne 0.25 i spanikował.

## Ogniwo latch → sędzia (izolowane, ten sam NPC, jedna klatka)
- `bIsInPanic=False` → `EvaluateCurrentNeed()` = **SATISFIED**
- `bIsInPanic=True`  → `EvaluateCurrentNeed()` = **LEVEL_0_FIGHT_OR_FLIGHT**

## Histereza wyjścia
Potwierdzona ubocznie: gdy heal BP przywracał HP→100 (HP%≥EffThr), latch był czyszczony — dlatego
live-readbacki pokazywały `bIsInPanic=False` mimo wcześniejszego ENTER. Wejście i wyjście działają.

---

## 🚩 Znaleziska (złapane podczas weryfikacji — „złapany = zapisany")

### TECH-07 (P2) — `GetHPPercent()` dzieli przez literał 100
`MaslowBiologicalComponent.cpp:637-640` zwraca `CurrentHP/100.0` (= AbsoluteMaxHP), nie `CurrentHP/CurrentMaxHP`.
Autofagia/odwodnienie obniża `CurrentMaxHP` → wyniszczony NPC przy pełnym aktualnym zdrowiu czyta 0.5;
paski HUD błędne. L3-02 świadomie liczy własne `CurrentHP/CurrentMaxHP`. Fix odłożony (osobny ticket).

### TECH-08 (P1) — BP_NPC pinuje `CurrentHP=CurrentMaxHP=100` co klatkę
Coś w `BP_NPC_Character` (Event Tick lub BT-driven) resetuje HP do pełna ~co klatkę — łamie zasadę
„No Event Tick" i UNIEWAŻNIA cały model HP metabolizmu (odwodnienie/autofagia/obrażenia cofane).
Dowód: w PIE `CurrentHP=35` wracało do 100 w 1–2 ticki; zatrzymane DOPIERO przy omdleniu
(HoursAwake>24 → twardy `StopLogic`), wtedy poleciało 11 czystych rzutów 0.25 i C1 spanikował.
**Skutek dla L3-02:** w żywej grze HP%≡1.0 → rzut nigdy nie wchodzi w pasmo → cecha uśpiona.
**Naprawić przed** dowolnym HP-zależnym zachowaniem (panika L0, flee L0-02/03, leczenie).

---

## Uwagi metodyczne
- `MetabolismInterval=10 s`; demo kompresowane przez `slomo`. Interakcje uboczne (autofagia czyści timer
  metabolizmu przy śmierci z odwodnienia; omdlenie z HoursAwake) utrudniały live-readback — ostateczny
  dowód wzięty z LogMaslow na dysku, nie z pojedynczego odczytu obiektu.
- Konfiguracja NPC ustawiana na żywych komponentach w PIE (set_editor_property), bez edycji/zapisu BP
  (zgodnie z regułą RF_Transient / manual-save).
- Testowy 2. NPC posprzątany (level wrócił do 1 NPC, bez zapisu).
