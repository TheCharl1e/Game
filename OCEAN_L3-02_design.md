# OCEAN L3-02 — Personality layer (design gate → SHIPPED slice #1)

> Status: **SLICE #1 BUILT + PIE-VERIFIED 2026-06-20.** Killer-demo proven (C1 N=0.9→ENTER panic;
> C2 N=0.1→PanicChance 0.00→calm; same HP%=0.35). See REPORTS/raport_2026-06-20_ocean_l3-02_slice1.md.
> ✅ **LIVE IN-GAME** (corrected 2026-06-20 after TECH-08 recon): HP is NOT pinned — it drops naturally
> via dehydration/autophagy → neurotic NPCs panic earlier. Clean proof (no tricks): HP=35 held 30 s +
> natural panic 11:25:20 (rolled=1 latched=1). The earlier "dormant/BP-pins-HP" claim was a MISDIAGNOSIS
> (TRASH_ reinstanced-component measurement artifact; no BP/BT node writes Maslow HP). See TECH-08/09.
> Flags A–E resolved (below). Executor grounded every claim in live code (file:line).

---

## 1. Locked decisions

| # | Decision |
|---|---|
| Q1 | **Storage = hybrid.** Target: `DA_PersonalityArchetype` (5 means + 5 variances + label) → spawn-time roll → per-instance on `UNPCIdentityComponent`. **Slice #1:** no DataAsset; OCEAN set **by hand** on 2 NPCs (`EditAnywhere`). |
| Q2 | **Slice #1 = stochastic panic ROLL** (not a binary threshold). Neuroticism shifts the *chance* of panic. Pattern mirrors microsleep. |
| Q3 | **Dual-track backbone.** Mechanism-per-effect (threshold-shift here; weight-multiplier later). **Utility-scoring layer = NOT now.** |

---

## 2. Slice #1 — Neuroticism → panic chance

### 2.1 Model (endpoints data-driven, zero hardcode)
```
HPPercent    = CurrentHP / CurrentMaxHP                        // FLAG A: NOT GetHPPercent() (broken)
EffThreshold = Lerp(StableThr, NervousThr, Neuroticism)        // HP% where chance becomes >0
PanicChance  = Clamp((EffThreshold - HPPercent) / PanicBand, 0, MaxPanicChancePerTick)
HPPercent >= EffThreshold  → PanicChance = 0 (no roll)
HPPercent <  EffThreshold  → chance rises as HP drops
```

**Default endpoints (satisfy the layering invariant — see §3 check):**
| Tunable | Default | Rationale |
|---|---|---|
| `StableThr` | **0.25** | == hard floor (`CriticalHPThreshold`/`AbsoluteMaxHP` = 25/100). Calmest NPC panics ONLY via hard floor — its stochastic band is empty. |
| `NervousThr` | **0.55** | Most-neurotic NPC's roll opens at 55% HP — wide band above the floor. |
| `PanicBand` | **0.15** | HP% width over which chance climbs 0→max. |
| `MaxPanicChancePerTick` | **0.25** | ~4 metabolism ticks to panic at full band. Mirrors `MicrosleepChancePerTick=0.15`. |

All `UPROPERTY(EditAnywhere)` on the Maslow component. No literals in the math.

### 2.2 Killer-demo (hard number)
2 NPCs, hand-set Neuroticism, identical `ApplyDamage` → **~35% HP** (above the 25% floor, between the two EffThresholds):

| NPC | Neuroticism | EffThreshold = Lerp(0.25,0.55,N) | HP% 0.35 | Result |
|---|---|---|---|---|
| C_1 | 0.9 | **0.52** | 0.35 < 0.52 | PanicChance = 0.25 → panics within ~few ticks → `IsInPanic=1` |
| C_2 | 0.1 | **0.28** | 0.35 > 0.28 | PanicChance = 0 → never rolls → `IsInPanic=0` |

Same wound, different reaction. Log prints **both**: `Neuroticism, EffThreshold, HPPercent, PanicChance, rolled?, latched`.
OCEAN deterministic; roll stochastic (same contract as microsleep — accepted).

---

## 3. Live-code grounding + 🔑 pre-build check

| Confirmed | Reality | File:line |
|---|---|---|
| IsInPanic pipe | `IsInPanicKey`; `bPanic = (Priority == Level_0_FightOrFlight)`. End-to-end already wired. | `BTService_MaslowBlackboardSync.cpp:79,92` |
| Insertion point | `EvaluateCurrentNeed()` Tier 0: `if (CurrentHP <= CriticalHPThreshold) return Level_0;` | `MaslowBiologicalComponent.cpp:661-665` |
| HP fields | `CurrentHP`, `CurrentMaxHP` (autophagy/dehydration lower it), `AbsoluteMaxHP=100`. `CurrentHP` clamped ≤ `CurrentMaxHP`. | `.cpp:31,224,228,132-133` |
| Microsleep pattern | Roll in `ProcessMetabolism→UpdateFatigue`; latch `bMicrosleeping`; cleared in `EndMicrosleep`. | `.cpp:309-312,347,360` |

**🔑 CHECK RESULT — `CriticalHPThreshold = 25.0f`** (`.cpp:55`) → as % = **0.25**. The brief's example
`StableThr ≈ 0.15` **VIOLATED** the invariant (0.25 > 0.15 → hard floor sits *above* the calmest band,
layers confused). **Resolved:** `StableThr = 0.25` (== floor, satisfies `floor% <= StableThr` with equality).
Demo HP moved 30%→35% so C_2 (EffThr 0.28) stays calm with margin.

---

## 4. Resolved flags (director decisions)

- **A — HP% source.** CONFIRMED. Do **not** use `GetHPPercent()` (divides by literal 100 = `AbsoluteMaxHP`, mis-reads after autophagy). Slice computes `CurrentHP / CurrentMaxHP` fresh (guard `CurrentMaxHP > 0`). The HUD-bar bug is **REAL + SEPARATE** → new **TECH** ticket in ROADMAP (⬜), NOT fixed here.
- **B — roll in judge.** Reconciliation ACCEPTED. Roll lives in `ProcessMetabolism` (metabolism cadence) → sets latch `bIsInPanic`. `EvaluateCurrentNeed` only **reads**: `if (bIsInPanic || CurrentHP <= CriticalHPThreshold) return Level_0;`. **Zero RNG in the judge.** Existing `CurrentHP <= CriticalHPThreshold` kept as a **hard floor read LIVE every BT tick** (instant panic on sudden lethal `ApplyDamage`), beside the latch (rolled on metabolism cadence). Layered: floor = universal last-resort; trait roll = band ABOVE it.
- **C — latch + exit.** ACCEPTED. Clear `bIsInPanic` when `HPPercent >= EffThreshold` (hysteresis, like microsleep). `PanicDuration` (design-doc "8s") **does NOT exist in code** (grep `Source/` empty; it's a future flee spec, L0-02/03) → HP-recovery exit suffices, **no 8s timer built**.
- **D — OCEAN shape.** `FOceanProfile` (BlueprintType; 5 named floats `Openness/Conscientiousness/Extraversion/Agreeableness/Neuroticism`, each clamped 0–1). Same 20 B as `float[5]`; readable log; same shape reused by the future DataAsset hybrid.
- **E — cross-component read (perf ×500).** Neuroticism lives on `UNPCIdentityComponent`; roll lives in Maslow. Maslow caches the sibling `UNPCIdentityComponent*` once (lazy, first `ProcessMetabolism`, `IsValid` guard), reads `Neuroticism` per metabolism tick. Cost ×500: 1 cached ptr + 1 float read per `MetabolismInterval`. No `FindComponentByClass` per tick. Memory: 20 B/NPC = 10 KB @ 500. Negligible.

---

## 5. Fail-safes
Roll only when: owner alive, `!bIsInPanic`, `CurrentMaxHP > 0`, `HPPercent` inside band. `IsValid` guard on cached Identity (NPC can die mid-action: P2P, sleep, collapse). EndPlay already clears Maslow timers — latch is plain state, reset on construct.

---

## 6. Dual-track note (Q3)
Slice #1 = **threshold-shift** (cheapest proof of the trait→behavior pipe). It does NOT exercise the BT
**weight-multiplier** path that L3-03 is about — intentional. Keep trait→BT exposure thin (one read site)
so adopting weight-multiplier / utility-scoring later is not a refactor. **Utility-scoring deferred.**

---

## 7. Build/verify plan
1. **Patch** (not regenerate): `FOceanProfile` + field on `UNPCIdentityComponent`; tunables + `bIsInPanic` + cached Identity ptr + roll in `ProcessMetabolism`; latch-read in `EvaluateCurrentNeed`; demo log.
2. Build **editor CLOSED** (UHT, UE 5.7) → reopen → PIE.
3. Killer-demo: 2 NPCs, Neuroticism 0.9 / 0.1, identical `ApplyDamage` → ~35% HP.
4. Truth from `Saved/Logs/` **and** live object props — `IsInPanic=1` vs `0`; values CHANGE at runtime.
5. CODE_REGISTRY / CHANGELOG / ROADMAP (L3-02 + new TECH for HUD bug), REPORT, commit (English).
