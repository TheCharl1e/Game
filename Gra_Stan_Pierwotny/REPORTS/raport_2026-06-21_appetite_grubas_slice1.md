# Raport — APPETITE / GRUBAS slice 1: kod + weryfikacja PIE

**Data:** 2026-06-21
**Gate:** `APPETITE_GRUBAS_design.md` (zatwierdzony; RECON-STOP 3 dziury potwierdzone; decyzje dyrektora zalockowane).
**Zakres:** slice 1 = runaway grubas (LeptinBrake=0). Proces jedzenia + makra→magazyn + rozpychanie żołądka (dual-driver) + most fat→izolacja.
**Wynik:** ✅ **Rdzeń dowiedziony twardo (BLOK 0/1/2/4).** ⚠️ BLOK 3 (shrink) + BLOK 5 (timeline izolacji) = verify-debt (ROADMAP L1-11v).

---

## RECON-STOP (3 dziury — potwierdzone przed kodem)
1. **Depozyt carb→fat NIE istniał** — `ConsumeFood` cappował Glucose/Glycogen i WYRZUCAŁ nadwyżkę; BodyFat rósł tylko z dietary fat. Model równoległy, nie kaskadowy.
2. **Jedzenie = instant single-shot** — brak StomachFill/procesu/remaining-portion; `AItemBase` bez nadgryzienia; AnimNotify jedzenia istniał tylko dla zwierząt.
3. **InsulationFactor ŻYWY** — czytany w sprzężeniu stygnięcia (`MaslowBiologicalComponent.cpp` reżimy 1/2), ale stała 1.0 (brak writera). Hook gotowy.
4. **Autofagia ODWRACALNA** (kod), nie „trwała" (HUNGER_design) — białko odbudowuje MaxHP. FLAGA → dyrektor: zostaje reversible.

## Build
3× pełny UHT (editor closed, `Build.bat Stan_PierwotnyEditor`): **Result: Succeeded** (86s / 50s / 21s). `MaslowBiologicalComponent.cpp` + `ItemBase.cpp` + nowy `AnimNotify_EatBite.cpp` skompilowane standalone (unity-excluded), oba DLL zlinkowane, 0 ostrzeżeń z kodu.

## Dowód PIE (świeży edytor, anti-TRASH_, live + LogMaslow z dysku)

### Stan startowy
| Sygnał | Wartość | Sprawdzenie |
|---|---|---|
| BodyFat (spawn) | **1500** | StartingBodyFat w BeginPlay (≠ Max 5000) |
| InsulationFactor | **0.88** | `Lerp(1.0, 0.6, 1500/5000)` — most fat→izolacja OŻYŁ (był 1.0) |
| GastricCapacity / Setpoint | **100 / 115** | Base100 × Overfill1.15 |
| Efektywności | 0.75 / 0.95 / 0.5 | carb / fat / protein |

### BLOK 0 — proces przerywalny
- Kęsy ×20 objętości: StomachFill 0→20→40→60→80; Glucose 300→340 (**+10/kęs proporcjonalnie**); item_rem 1.0→0.9→0.8→0.7→0.6.
- INTERRUPT → `bIsEating=False`, **item_rem=0.6>0 (nadgryzione istnieje)**.
- **EC-EAT-2:** `destroy_actor` itemu w trakcie → `consume_bite` → `StopEating(SourceGone)`, **zero crasha**.
- **EC-EAT-3:** NPC COLLAPSED (HoursAwake=53) → `StartEating` early-return.

### BLOK 1 — satiety auto-stop
Od pustego: StomachFill 0→...→110 (eating=True), kęs 12 → **120 ≥ Setpoint 115 → auto-stop**. Disk-log: `StopEating(Full) — mealSize=115 GastricCapacity=104.5 Setpoint=120.2`. NPC nie zjadł 20 kęsów.

### BLOK 2 — stretch monotoniczny (disk-log)
- `StopEating(Full) mealSize=115 GastricCapacity=104.5` → `Lerp(100,115,0.30)=104.5` ✓
- `StopEating(Full) mealSize=110 GastricCapacity=106.2` → `Lerp(104.5,110,0.30)=106.15` ✓
- Trend: **100 → 104.5 → 106.2** monotonicznie w górę (ku Max 250), persystuje bez setów.

### BLOK 4 — routing makr (delty, gate_eating=true)
| Test | Podane | Wynik | Efektywność |
|---|---|---|---|
| FAT | FatG=100 | ΔBodyFat **+95** | ×0.95 ✓ |
| CARB overflow | carb=space+200 | ΔGlucose +240 (do capu) + ΔBodyFat **+150** | ×0.75 ✓ |
| PROTEIN | ProteinG=100 (MaxHP pełne) | ΔBodyFat **+25** (surplus 50×0.5) | ×0.5 ✓ |
| VOLUME≠kcal | dense v20/f100 vs bulky v200 | dense Stom+20 Fat+95; bulky Stom+200 **Fat 0** | ✓ |

→ tłuszcz tuczy wyraźnie szybciej niż węgle; białko buduje MaxHP, ledwo tuczy; objętość syci niezależnie od kalorii.

---

## OTWARTE (ograniczenie harnessu, NIE kodu) — ROADMAP L1-11v
- **BLOK 3 (shrink):** formuła symetryczna do dowiedzionego stretchu, ale live-demo blokuje reinstance-na-`set` (wipe GastricCapacity do Base100) + niemożność wymuszenia fazy FatBurn.
- **BLOK 5 (timeline stygnięcia):** mapa fat→izolacja dowiedziona (0.88@1500); porównanie krzywych = real-time capture.
- **Czysta kaskada starvation:** NPC pod `slomo 30×` umarł z ODWODNIENIA (Hydration=0, PHASE_4_DEATH), nie starvation.

## Harness — pułapki złapane (→ pamięć)
- `get_component_by_class` (singular) = stale wrapper: odczyty/zapisy działają, **UFUNCTION dispatch no-op**. Użyj `get_components_by_class[...]` + probe.
- `set_editor_property` na żywym komponencie PIE → **reinstancing → re-BeginPlay → wipe iniekcji do defaultów**. Obejście: re-fetch po set, weryfikacja przez DELTY i UFUNCTIONy, nie absolutne baseline'y.
- `slomo` przyspiesza WSZYSTKIE dreny → 30× zabija odwodnieniem. Collapse czyść `start_sleep()`+`stop_sleep()` (nie setem).

## STOP — wg gate: po slice 1 (runaway). Slice 2 (LeptinBrake) + 1b (wiring BTTask_Eat) + 1-verify (BLOK 3/5) = osobne „jedź".
