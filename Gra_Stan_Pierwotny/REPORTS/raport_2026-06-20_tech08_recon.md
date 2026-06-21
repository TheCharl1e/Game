# Raport recon — TECH-08 (rzekomy pin HP w BP_NPC_Character)

**Data:** 2026-06-20
**Zlecenie:** dyrektor — „RECON-NAJPIERW, potem fix. NIE tnij na ślepo."
**Wynik:** ❌ **premisa obalona** — nie ma pinu HP. Nic do wycięcia. L3-02 żyje w grze.

---

## Hipoteza wejściowa (z weryfikacji L3-02, commit 48e1a73)
„BP_NPC_Character pinuje `CurrentHP=CurrentMaxHP=100` co klatkę (Event Tick/BT) → HP% zawsze 1.0 →
rzut paniki nigdy nie wchodzi w pasmo → L3-02 uśpiony w grze."

## Recon (read-only) — przeczytane grafy
| Graf | Węzły | Zapis CurrentHP/CurrentMaxHP Maslowa? |
|---|---|---|
| BP_NPC_Character EventGraph | 48 | **NIE** (brak Event Tick; 3× ApplyDamage na **BodyCondition**, komentarz „TEST ONLY - remove later") |
| BP_NPC_AI (AIController) EventGraph | 30 | **NIE** (OnPossess→RunBT; perception→Food/Drink/Shelter) |
| BTService_MaslowBlackboardSync | C++ | **NIE** (read-only Maslow→BB) |
| BTTask_Check | 8 | **NIE** (woła EvaluateNeeds; czyta HungerLevel) |
| BTTask_Drink | 14 | **NIE** (zmniejsza BP `ThirstLevel`) |
| EvaluateNeeds (funkcja BP) | 61 | **NIE** (czyta need-levele, ustawia `CurrentNeed`+BB) |
| ApplyActionCost (funkcja BP) | 29 | **NIE** (operuje na struct `MetabolismStats`: Kcal/Stamina) |

**Żaden graf nie pisze HP Maslowa.** W C++ HP pisane tylko: konstruktor=100, śmierć=0, odwodnienie−10,
autofagia (clamp w dół). Brak ścieżki do „=100" poza konstruktorem/BeginPlay.

## Przyczyna fałszywego sygnału „HP 35→100"
**Artefakt pomiaru — komponenty `TRASH_`.** Ścieżki w PIE:
`BP_NPC_Character_C_1.TRASH_MaslowBiologicalComponent_0` / `_1` (inkrementujący suffix między wywołaniami).
Prefiks `TRASH_` = obiekt do GC po reinstance Blueprinta. `get_component_by_class` zwracał martwą instancję;
write znikał z trashem, a świeży komponent czytał **default CDO = 100**. Churn wywołany ciężką sesją
(rebuild C++ + wiele start/stop PIE + bliski crash) — **nie** pin co klatkę.

## Czysty dowód (świeże PIE, stabilne okno, żywy komponent `MaslowBiological`, ZERO sztuczek)
LogMaslow na dysku — bez omdlenia, bez StopLogic, normalny czas:
```
[11:25:09] PanicRoll C_1: HP%=0.35 PanicChance=0.25 rolled=0 latched=0
[11:25:20] PanicRoll C_1: HP%=0.35 PanicChance=0.25 rolled=1 latched=1   <- naturalna panika
[11:25:29] PanicRoll C_1: HP%=0.35 PanicChance=0.25 rolled=0 latched=1   <- latch trzyma
[11:25:40] PanicRoll C_1: HP%=0.35 PanicChance=0.25 rolled=0 latched=1   <- HP WCIĄŻ 0.35
[11:25:20] [Panic:C_1] ENTER stochastic panic — Neuroticism=0.90 EffThr=0.52 HP%=0.35 PanicChance=0.25
```
- **HP=35 utrzymane ~30 s** → brak pinu.
- **Nerwowy NPC spanikował sam** → L3-02 żyje w grze.

## Wnioski / decyzje (zatwierdzone przez dyrektora)
1. **L3-02:** status „uśpiony do TECH-08" → **ŻYWY W GRZE**. Kod (48e1a73) był i jest poprawny.
2. **TECH-08:** P1→**P3**, przeramowane. Stary opis (pin HP) wykreślony jako MISDIAGNOZA. Realny temat:
   **dwa rozłączone systemy metabolizmu** (BP HungerLevel/ThirstLevel/SleepLevel/MetabolismStats RÓWNOLEGLE
   do C++ Maslow, nigdy nie zsynchronizowane) — dwie prawdy o tym samym NPC, dług architektoniczny.
3. **TECH-09 (nowy, P3):** churn komponentów `TRASH_` w PIE — potwierdzić na świeżym edytorze
   (artefakt zmęczonej sesji vs pętla respawnu NPC). Jedyna otwarta niewiadoma.

## Nie zrobione (świadomie)
- Pełny test przeżywalności (masowe zgony) — bezprzedmiotowy, bo fixa nie ma (nic nie zmieniono).
- Potwierdzenie TECH-09 na świeżym edytorze — osobny temat (edytor zrestartowany, gotowy).
