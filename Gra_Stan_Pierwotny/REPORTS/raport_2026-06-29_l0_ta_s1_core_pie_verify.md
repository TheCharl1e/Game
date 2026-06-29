# Raport — L0-TA-S1 core PIE-verify (afordancje: rejestracja + spatial hash)

**Data:** 2026-06-29
**Środowisko:** **UE 5.7**, `E:\Game` (przywrócone jako env weryfikacyjny — patrz §Tooling), mapa **CaldrethMap**, Monolith (port 9316, survives PIE).
**Zakres:** re-scoped CORE subsystemu afordancji (rejestracja + dane per-instancja + spatial hash). Pełna pętla BT i hotspoty wymagające instancji subsystemu — NIEzweryfikowane (patrz §Odłożone).

---

## VERIFY — twarde liczby z żywego PIE

3 testowe `AffordanceSourceActor` postawione w CaldrethMap, skonfigurowane przez Python (`set_editor_property`),
read-back potwierdzony PRZED PIE:
- TEST_AFF_N: `AffordanceType=NOURISHMENT(1)`, Yield=50, RegenPerHour=10, ColdDampen=0
- TEST_AFF_H: `AffordanceType=HYDRATION(2)`, Yield=50, RegenPerHour=10, ColdDampen=0
- TEST_AFF_S: `AffordanceType=SHELTER(3)`, Yield=100, RegenPerHour=0, ColdDampen=0.5

Po `start_pie` — log `LogWorldAffordance` (verbosity Verbose), świat `UEDPIE_0_CaldrethMap`:
```
[Affordance] Subsystem ready (CellSize=2000, RegenInterval=10.0s).
[Affordance] Register id=0 type=1 yield=50.0  regen/h=10.00 dampen=0.00 owner=TEST_AFF_N cell=(-1,-1).
[Affordance] Register id=1 type=2 yield=50.0  regen/h=10.00 dampen=0.00 owner=TEST_AFF_H cell=(-1,-1).
[Affordance] Register id=2 type=3 yield=100.0 regen/h=0.00  dampen=0.50 owner=TEST_AFF_S cell=(-1,-1).
```

**Dowiedzione (twarde liczby z live):**
1. **Subsystem inicjalizuje się** — `WorldAffordanceSubsystem` startuje w PIE: `CellSize=2000`, `RegenInterval=10.0s` (timer regenu skonfigurowany).
2. **3/3 afordancje zarejestrowane** w BeginPlay, kolejne `id=0,1,2` (nie-recyklowane).
3. **Dane per-instancja przechodzą 1:1** edytor→rejestr: typ (1/2/3), yield (50/50/100), regen/h (10/10/0), dampen (0/0/0.5) — zgadzają się z konfiguracją.
4. **Spatial hash liczy komórkę** — każda dostała `cell=(-1,-1)` (z pozycji ~(-900/-800/-700, -70) i `CellSize=2000` → floor = (-1,-1)). Poprawnie.
5. **Self-register w BeginPlay** (`AffordanceSourceActor` → subsystem) działa end-to-end.

Sprzątnięto: PIE stop, 3 aktory `destroy_actor` (`LEFT []`), **mapy NIE zapisano** (CaldrethMap nietknięta).

---

## VERIFY #2 — REGEN NA TIMERZE (czas-integrowany, blind-spot ZAMKNIĘTY)

Dostęp do żywego world-subsystemu uzyskany: `unreal.find_object(game_world, 'WorldAffordanceSubsystem_0')`
→ instancja wołalna z Pythona (`register_affordance_simple`/`consume`/`get_affordance`/`query_nearest` jako BlueprintCallable).
(`World.get_subsystem`/`SubsystemBlueprintLibrary` w tym buildzie NIE istnieją — `find_object` po nazwie działa.)

Test (PIE, świat UEDPIE_0_CaldrethMap): `register_affordance_simple(Nourishment, yield=50, RegenPerHour=3600)`
→ `consume(40)` → odczyty `RemainingYield` po realnym czasie:

| t | RemainingYield | uwaga |
|---|---|---|
| t0 (po consume 40/50) | **10.0** | |
| t1 (~3 ticki regenu) | **40.0** | rośnie po wall-clock, napędzane timerem |
| t2 (kolejne ticki) | **50.0** | **clamp na MaxYield, bez przekroczenia** (`remaining<=max == True`) |

**Dowiedzione:** regen biegnie **na timerze (nie Tick)**, `RemainingYield` faktycznie ROŚNIE w czasie
(+10/tick: `RegenPerHour*RegenInterval/3600 = 3600*10/3600 = 10`), klampowany do `MaxYield` (`FMath::Min`,
brak overshootu). Formuła z `RegenTick` (real-seconds, brak zegara świata w Slice 1) potwierdzona empirycznie.
To zamyka regułę „zmień-w-czasie, nie tylko-kod-istnieje" (CLAUDE.md) dla regenu.

---

## VERIFY #3 — QueryNearest + atomowość claima + cancel-on-death

Wszystko na żywym subsystemie w PIE (Python na instancji). 3 afordancje: n1(Nourish,id0,@origin), n2(Nourish,id1,@5000), h1(Hydration,id2,@1000).

**QueryNearest:**
- nearest-of-type z origin → **0** (n1 bliższe niż n2) ✓
- filtr typu: Hydration → **2** (h1, nie Nourishment) ✓
- radius za mały (z (20000,0,0), R=1000) → **-1** ✓

**Atomowość claima** (rezerwujący A=WorldSettings_1, B=Brush_1):
- reserve n1 by A → **True**; reserve n1 by B → **False** (zajęte) ✓
- po rezerwacji `query_nearest(Nourish)` **pomija** n1 → **1** (n2) ✓
- `release(n1,A)` → `query_nearest` znów **0**; reserve n1 by B → **True** ✓

**Cancel-on-death (death fail-safe):**
- reserve n2 aktorem `DEATH_TEST_RESERVER` → reserved_by=**67596** ✓
- `destroy_actor()` → tuż po: reserved_by wciąż 67596 (zwolnienie dopiero na RegenTick — poprawnie) ✓
- po 1 RegenTick: reserved_by → **-1**, n2 znów do zapytań; verbose log `[Affordance] Released claim id=1 — reserver dead/invalid` ✓

Dowiedzione: O(local) zapytanie przestrzenne (nearest/typ/radius), atomowy claim (podwójna rezerwacja zablokowana,
query pomija zarezerwowane, release zwalnia), oraz auto-zwolnienie slotu po śmierci rezerwującego (weak-ptr → RegenTick sweep + log).
`Consume` częściowo: `consume(40)` z 50 zwrócił granted=40, RemainingYield→10 (VERIFY #2).

---

## POZOSTAŁE (czekają na ręczny autoring EQS/BT — tooling nie autoruje)
- **EQS:** `EnvQueryGenerator_Affordances` zwraca itemy; throttle `EQSBudgetManager`; cancel-on-death zwalnia slot budżetu.
  (`GatherAffordanceLocations` to C++ non-UFUNCTION → nie wywołasz z Pythona; potrzeba EQS assetu + uruchomienia query.)
- **Consume revalidate-on-arrival** krawędzie (pusta/zajęta-przez-innego po dojściu, roll-back) — szybkie do dorobienia tą samą techniką.
- **Pełna pętla DoD §8** (homeless→wander→EQS Safe Zone→attach→forage→spadek L1) — czeka na ręczny autoring EQS/BB/BT.

## Tooling — kontekst (dlaczego 5.7)
Natywny MCP UE 5.8 (`ModelContextProtocol`, HTTP :8000/mcp) **umiera na każdym cyklu PIE i nie wstaje sam** →
nieprzydatny do PIE-verify. Pivot na 5.7 (`E:\Game`): przywrócono uproject→5.7 + Monolith/MCPUnreal enabled +
Targety `V6`/`Unreal5_7` (były `Latest`/`Unreal5_8` → UBT 5.7 RulesError), headless rebuild `Result: Succeeded`.
Monolith przeżywa PIE i ma `run_python` (pełne API unreal) — stąd wykonalny ten verify. Zmiany w `E:\Game`
(uproject + Targety) **niezacommitowane, odwracalne**. Plan: po domknięciu hotspotów — port afordancji/EQS/BT do `E:\Game_58` (5.8 live target).
