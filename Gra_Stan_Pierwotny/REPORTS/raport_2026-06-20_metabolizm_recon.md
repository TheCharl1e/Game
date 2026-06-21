# Raport recon — Dwa metabolizmy: który system steruje wyborem akcji NPC

**Data:** 2026-06-20
**Zlecenie:** dyrektor — RECON read-only, łańcuch decyzji OD KOŃCA (co BT realnie czyta), twarde liczby z live/log. NIE naprawiać, NIE projektować.
**Werdykt na pytanie nadrzędne:** Akcjami steruje **system BP** (`HungerLevel/ThirstLevel/SleepLevel → EvaluateNeeds → CurrentNeed`). **C++ `UMaslowBiologicalComponent` NIE steruje wyborem akcji** — jego most do Blackboardu jest źle podpięty i martwy, a co gorsza **aktywnie zeruje `CurrentNeed`**. Jedyny żywy kanał C++→zachowanie to **omdlenie ze snu (`StopLogic`)**, które trwale ubija BT.

---

## 1. Co BT realnie czyta przy wyborze gałęzi (dekoratory)

Drzewo `/Game/DocelowaGra/AI_NPC/BT_NPC`, korzeń → Selektor **„Potrzeby"**. Dzieci pod korzeniem i ich dekoratory:

| Gałąź | Dekorator (klucz / warunek) | Klucz BB | Typ |
|---|---|---|---|
| Handle Thirst | `CurrentNeed Is Equal To Thirst` (IntValue 2) | **CurrentNeed** | Enum `E_NeedState` |
| Handle Sleep | `CurrentNeed Is Equal To Sleep` (IntValue 3) | **CurrentNeed** | Enum (gałąź **PUSTA** — 0 dzieci) |
| (Handle Hunger) | — | — | **ODŁĄCZONA** od korzenia (`parent_id:null`) |
| (Zagrożenie / panika) | — | — | **ODŁĄCZONA** od korzenia |
| (sekwencja Eat) | — | — | **ODŁĄCZONA** od korzenia |

Wewnątrz Handle Thirst: sub-Sequence bramkowana `NearestWaterLocation Is Set` → `BTTask_FindWater_Experyment` → `Move To` (klucz `NearestWaterLocation`) → `BTTask_Drink` → `BTTask_Check`.

**Wniosek 1:** Jedyny klucz bramkujący wybór gałęzi to **`CurrentNeed`** (Enum). Funkcjonalnie żywa jest wyłącznie **Handle Thirst**. Żaden dekorator pod korzeniem NIE czyta kluczy Maslowa (`MaslowPriority/HPPercent/IsInPanic/...`). Gałąź paniki („Zagrożenie") jest odłączona → **panika L3-02 nie ma jak wpłynąć na zachowanie**.

## 2. Kto ZAPISUJE `CurrentNeed` — i konflikt

- **BP `EvaluateNeeds`** (funkcja na **BP_NPC_Character**): czyta `HungerLevel/HungerThreshold`, `ThirstLevel/ThirstThreshold`, `SleepLevel/SleepThreshold`, `SaftyLevel`; ustawia zmienną `CurrentNeed` i pisze ją do BB przez **`SetValueAsEnum(KeyName="CurrentNeed")`** (4 gałęzie: None/Hunger/Thirst/Sleep). **Poprawny typ.**
- **C++ `BTService_MaslowBlackboardSync`** (serwis na korzeniu „Potrzeby", Interval=1.0 s): **wszystkie 6 selektorów kluczy wskazuje na `CurrentNeed`** (zrzut z `export_bt_spec`):
  `MaslowPriorityKey→CurrentNeed`, `HydrationPercentKey→CurrentNeed`, `GlucosePercentKey→CurrentNeed`, `HPPercentKey→CurrentNeed`, `IsInPanicKey→CurrentNeed`, `IsStarvingKey→CurrentNeed`.
  Kod woła `SetValueAsInt/Float/Bool` → wpisuje Int/Float/Bool do klucza **Enum**.

| Klucz `CurrentNeed` | BP `EvaluateNeeds` | C++ serwis |
|---|---|---|
| metoda | `SetValueAsEnum` ✅ | `SetValueAsInt/Float/Bool` (mismatch) |
| dedykowane klucze Maslow | — | **nikt nie pisze** (wszystkie selektory = CurrentNeed) |

**Wniosek 2:** Most Maslow→BB jest **rozkonfigurowany**. Dedykowane klucze (`MaslowPriority/HydrationPercent/GlucosePercent/HPPercent/IsInPanic/IsStarving`) nie są zapisywane przez nic. Serwis celuje w `CurrentNeed` — ten sam klucz, którego używa system BP.

## 3. Twarda weryfikacja PIE (mapa `/Game/DocelowaGra/Game`, 2 NPC)

### 3a. Stan bazowy (live, oba NPC identyczne — `BP_NPC_Character_C_1` / `_C_2`)
| BP (potrzeby) | wartość | C++ Maslow | wartość | Klucze BB Maslow | wartość |
|---|---|---|---|---|---|
| HungerLevel / próg | 70 / 60 (PONAD) | Glucose | 740 / 1000 | MaslowPriority | **0** |
| ThirstLevel / próg | 70 / 60 (PONAD) | CurrentHP | 100 / 100 | HPPercent | **0** |
| SleepLevel / próg | 40 / 80 | CurrentHydration | 48 | GlucosePercent | **0** |
| **CurrentNeed** | **NONE (0)** | bIsInPanic | False | IsInPanic | **false** |

**Dowód że serwis nie ląduje w BB:** komponent C++ ma realne liczby (HP=100, Glucose=740, Hydration=48), a **klucze BB Maslow stoją na defaultach 0/false**. Gdyby serwis działał, `HPPercent` byłby ~1.0. Jest 0 → **nigdy nie zapisany**.

### 3b. Kto zeruje `CurrentNeed` (test różnicowy — kluczowy)
- `runtime_set_bb_value CurrentNeed=2` → odczyt **natychmiastowy = 2** (set trzyma).
- **BT DZIAŁA**: po 3 s `CurrentNeed` wraca do **0**, `active_node=None`.
- **BT ZATRZYMANY**: ten sam set `=2` **utrzymany przez 4 s+** (bez zmian).

**Wniosek 3b:** Zerujący zapis jest **związany z tickiem BT**. Jedyny per-sekundowy pisarz na bezczynnym korzeniu to serwis Maslowa → **serwis C++ aktywnie nadpisuje `CurrentNeed` do 0 co ~1 s** (na tym buildzie typowane settery JEDNAK piszą do nie-pasującego klucza Enum; ostatni z 6 zapisów = `SetValueAsBool(IsStarving=false)`→0). Serwis nie tylko nie karmi BT — **sabotuje klucz, na którym stoi system BP.**

### 3c. Jedyny żywy kanał C++→zachowanie: omdlenie ze snu
Log `LogMaslow` (oba NPC, niewymuszone):
```
[Sleep] BP_NPC_Character_C_1: zmęczenie 0->1 @HoursAwake=16.67
[Sleep] BP_NPC_Character_C_1: zmęczenie 1->2 @HoursAwake=20.00 (mikrosen 21.67, 23.33)
[Sleep] BP_NPC_Character_C_1: zmęczenie 2->3 @HoursAwake=25.00
[Sleep] BP_NPC_Character_C_1: OMDLENIE (HoursAwake=25.00) — BT zatrzymany, ragdoll.
```
Po reanimacji (HoursAwake→0) metabolizm **znów** wspiął HoursAwake do 25 → **ponowne OMDLENIE**. Stan końcowy: `HoursAwake=38.33, FatigueState=COLLAPSED, bIncapacitated=True, brain_running=False, CurrentHP=20` (spadło ze 100 — odwodnienie/autofagia C++).

**Wniosek 3c:** Sleep engine C++ **woła `StopLogic` i trwale ubija BT**. Wpięta gałąź „Handle Sleep" jest PUSTA, a `CurrentNeed` nigdy nie staje się Sleep → **NPC nie ma jak się przespać i zostaje w omdleniu na stałe**. To jedyny realny wpływ C++ na zachowanie — i jest **destrukcyjny (zatrzymanie), nie arbitraż potrzeb**.

## 4. Kaskada kataboliczna — policzona, ale nieczytana

`EvaluateCurrentNeed()` (jedyny konsument kaskady) czyta `Glucose/CurrentHP/CurrentTemp/Hydration/Stamina/bIsInPanic`, ale jego wynik trafia **wyłącznie do serwisu**, który jest martwy/rozkonfigurowany (pkt 2–3). HUD też nie binduje tych getterów (CODE_REGISTRY). Live: `CurrentHP` spadło 100→20, Hydration krytyczne — i **żadne zachowanie się nie zmieniło** (NPC nie poszedł pić/jeść; padł z osi snu).

**Wniosek 4:** Kaskada (Glucose→Glycogen→Fat→Autophagy→HP) jest **policzona, ale nieczytana przez AI** — martwa jak była temperatura. Wpływ na wybór akcji: **zero**.

---

## Odpowiedź na pytanie nadrzędne
**Wyborem akcji steruje system BP** (`EvaluateNeeds → CurrentNeed`, jedyne poprawnie wpięte źródło dekoratorów). **C++ Maslow nie steruje** — most do BB jest rozkonfigurowany (dedykowane klucze nigdy nie zapisane, live=0), a serwis dodatkowo **zeruje `CurrentNeed` co tick**. Jedyny żywy wpływ C++ na grę to **omdlenie (`StopLogic`)**, które trwale zatrzymuje BT. Kaskada kataboliczna jest martwa (policzona, nieczytana). Panika L3-02 nie dociera do BT (klucz `IsInPanic` nie czytany przez żaden wpięty dekorator; gałąź „Zagrożenie" odłączona).

### Brutalna ironia stanu obecnego (emergentne zachowanie live)
Żaden z systemów nie produkuje działającego zachowania potrzebowego: serwis C++ zeruje `CurrentNeed` → potrzeby BP się nie utrzymują; nawet samodzielnie wpięta jest tylko Thirst (Hunger odłączony, Sleep pusty), a nic nie bootstrapuje `CurrentNeed`; po ~2.5 min C++ omdlewa NPC na stałe. **Live NPC: stoją bezczynnie, potem trwale omdlewają.**

## Konflikt do rozplotu (dla ARCHITEKTA — ja NIE projektuję)
1. **Dwa źródła prawdy** o tym samym NPC: BP (`HungerLevel/ThirstLevel/SleepLevel` + `MetabolismStats`/`BodyComposition`/`ActionQueue`) vs C++ (`Glucose/Glycogen/Fat/HP/Hydration/HoursAwake`). Nigdy nie synchronizowane.
2. **Most C++→BB rozkonfigurowany:** 6 selektorów serwisu na 1 klucz `CurrentNeed` zamiast na dedykowane klucze; do tego mismatch typów (Int/Float/Bool → Enum).
3. **Serwis aktywnie psuje system BP** (zeruje `CurrentNeed`).
4. **C++ omdlenie ubija BT bez ścieżki wyjścia** (Handle Sleep pusta).
5. Gałęzie Hunger/panika/Eat **odłączone** od korzenia.

Decyzja kierunkowa (jeden system / most / który zostaje) należy do dyrektora + ARCHITEKTA. Recon nic nie zmienił.

## Metoda / zastrzeżenia
- Remote Control API (30010) wyłączone → BP-zmienne czytane przez Python (`execute_script`, port 8090), klucze BB przez Monolith runtime (8090). Wartości z żywych instancji PIE i logu na dysku.
- Infra: edytor był offline; uruchomiony przeze mnie. Pierwszy start zderzył się o porty 8090/9316 z dogasającą wcześniejszą instancją (Monolith „Failed to bind after 20 attempts") → restart jednej czystej instancji. Bez zmian w assetach/kodzie.
