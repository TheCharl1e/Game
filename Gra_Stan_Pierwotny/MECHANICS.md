# MECHANICS — Projekt "Stan_Pierwotny" (Matrix NPC)

> Concrete variable + value reference for every gameplay system.
> Purpose: a ground-truth doc so Claude Code can set up meaningful TEST scenarios
> (e.g. "force hunger to Phase_2_Fat and verify MaxHP drop").
> Values marked **[LOCKED]** are designed/approved. **[TBD]** = needs design decision (ask architect).
> Claude Code: verify each value against the actual C++ before testing; flag mismatches.
>
> 🔴 **Verified against C++ 2026-06-16 (Maslow / Body / Inventory). `[✓code]` = confirmed in
> code. `⚠️` = doc claimed but NOT implemented in code yet. Body & Inventory matched fully;
> Maslow had real drift — corrected below.**

---

## 🩸 POZIOM 0 — Stres (Fight or Flight)

**Rule:** panic overrides ALL lower needs. NPC drops current BT task queue immediately.

| Variable | Type | Value | Status |
|---|---|---|---|
| EMaslowPriority::Level_0_FightOrFlight | enum | 0 (highest) | [LOCKED] |
| Panic trigger (IN CODE) | — | `CurrentHP ≤ CriticalHPThreshold` = **25** → Level_0 | [✓code] |
| Panic trigger (DESIGNED): threat via VisionAcuity | — | VisionAcuity getter exists; threat→panic wiring not built (L0-04) | [GETTER READY, ENGINE TODO] |
| PanicDuration (how long flee state holds) | float (s) | **8.0** | [✓approved] |
| FleeSpeedMultiplier | float | **1.5×** | [✓approved] |
| Drop loaded container on flee? | bool | yes (DropContainer) | [LOCKED] |

**Test hook:** spawn BP_NPC_WOLF near NPC → NPC's VisionAcuity gates whether it detects → panic.

---

## 🫀 POZIOM 1 — Fizjologia

### Hunger — catabolic cascade
Body burns fuel in strict order. Enum `EHungerPhase`:

| Phase | Fuel source | Effect | Status |
|---|---|---|---|
| Phase_0_Glucose | Blood glucose | normal | [LOCKED] |
| Phase_1_Glycogen | Liver/muscle glycogen | normal, few hours buffer | [LOCKED] |
| Phase_2_FatBurn | Body fat (BodyFat) | burns at **0.5× rate**; cold ×2 (<10°C) | [✓code] |
| Phase_3_Autophagy | Muscle | **MaxHP −= 0.25×burn/tick** — **REVERSIBLE** (białko → MaxHP, RepairMuscle; brak trwałej podłogi w slice 1) | [✓VERIFIED] |
| Phase_4_Death | — | death | [LOCKED] |

| Variable | Type | Value | Status |
|---|---|---|---|
| Glucose / MaxGlucose | float | **1000** (code, not 100) | [✓code] |
| GlycogenReserves / MaxGlycogen | float | **1000** (code) | [✓code] |
| MaxBodyFat (sufit) | float | **5000** | [✓code] |
| StartingBodyFat (spawn, ROZDZIELONE od Max) | float | **1500** (~30% Max; BeginPlay: BodyFat=StartingBodyFat) | [✓VERIFIED] EditDefaultsOnly, per-archetyp później; live spawn=1500 |
| BaseBurnRate / tick (10s timer) | float | **10** × kcalMult × tempMult | [✓code] |
| ~~CriticalKcalThreshold~~ → **StableKcalThr** | float | Glucose ≤ **500** (calm baseline) | **[DEPRECATED]** rung czyta teraz EffectiveKcalThreshold (Hunger w1); 500 żyje jako StableKcalThr |
| Starvation → steal/cannibalism trigger | phase | Phase_3+ | [LOCKED] |

### Appetite / Grubas (slice 1 — proces jedzenia + makra→magazyn + rozpychanie żołądka + most izolacji)
> Runaway (LeptinBrake=0 do slice 2). PIE-verified 2026-06-21 (bloki 0/1/2/4). Shrink(B3)+timeline izolacji(B5) = verify-debt (ROADMAP APPETITE 1-verify).

| Variable | Value | Status |
|---|---|---|
| SatietyOverfillFactor (`Setpoint = Gastric × Overfill`) | **1.15** | **[✓VERIFIED]** bez overfill stretch-EMA zamrożony (MealSize nigdy > Gastric). Live: Setpoint=115 @Gastric100 |
| BaseGastricCapacity / MaxGastricCapacity | **100 / 250** | **[✓VERIFIED]** init=Base w BeginPlay; sufit 2.5×. Live: start 100 |
| StretchRate (EMA event, na zamknięciu posiłku) | **0.30** | **[✓VERIFIED]** `Gastric=Lerp(Gastric, MealSize, 0.30)`. Live monotonicznie: 100→104.5→106.2 |
| ShrinkRate (EMA tick, gdy bIsFasting=phase≥FatBurn) | **0.02** | [TBD→tune] << StretchRate (asymetria). Shrink path = code, live-demo = verify-debt |
| DigestionRatePerTick (drenaż StomachFill) | **5.0** /tick | [TBD→tune] live: StomachFill drenuje |
| StretchBonusScale (część E: rozpchany→je wcześniej) | **0.5** | [TBD→tune] `EffKcalThr += Max(0,Gastric−Base)×scale` |
| CarbToFatEfficiency (nadwyżka cukru>cap Glucose → fat) | **0.75** | **[✓VERIFIED]** live: 200 nadwyżki → BodyFat +150 |
| FatToFatEfficiency (FatG → BodyFat) | **0.95** | **[✓VERIFIED]** live: FatG=100 → BodyFat +95 |
| ProteinToFatEfficiency (surplus białka po MaxHP → fat) | **0.5** | **[✓VERIFIED]** live: ProteinG=100, MaxHP pełne → BodyFat +25 |
| MinInsulationFromFat (`InsulationFactor = Lerp(1.0, Min, BodyFat/Max)`) | **0.6** | **[✓VERIFIED]** live: BodyFat1500 → InsulationFactor=0.88 (most fat→izolacja ożył, był 1.0) |
| EEatStopReason | Full / Finished / Interrupted / SourceGone / Incapacitated | **[✓VERIFIED]** live: Interrupted (nadgryzione zostaje), SourceGone (zniszcz item, zero crasha), Full (satiety@115) |

### Thirst — hydration
| Variable | Type | Value | Status |
|---|---|---|---|
| Hydration / MaxHydration | float | 100 | [LOCKED] |
| HydrationBurnRatePerTick | float | **2.0** × action mult | [✓code] |
| Drain × Rest / Work / Combat | mult | ×1 / ×3 / ×5 | ⚠️ NOT hardcoded — set via DT_ActionCosts / SetCurrentActionMultiplier (kcal comment: Idle 1.0 / Combat 5.0) |
| Poison → hydration burn | mult | **×3** when `bIsPoisoned` | [✓code] |
| Dehydration effect (Hydration ≤ 0) | — | MaxHP & HP **−10/tick** → death | [✓code] (stamina-regen-block NOT impl) |
| CriticalHydrationThreshold (→ need) | float | ≤ **20** | [✓code] |

### Temperature
| Variable | Type | Value | Status |
|---|---|---|---|
| Tied to day/night cycle | — | yes | [LOCKED] |
| Cold → kcal burn multiplier | mult | **×2 when CurrentTemp < 10°C** (code) | [✓code] |
| CriticalTempThreshold (→ need / hypothermia) | float | ≤ **34°C** | [✓code] |
| Mitigated by | — | fire (Eureka), clothing (equip) | [LOCKED] |

### Sleep — hidden adenosine
| Variable | Type | Value | Status |
|---|---|---|---|
| HoursAwake | float | starts **0** (code) | [✓ETAP 1 BUILT 2026-06-18, commit a7efcdb] — now incremented on the metabolism timer: `HoursAwake += AwakeRatePerTick × ActivityMultiplier` / tick (no new timer, no Event Tick). Getters `GetHoursAwake()`/`GetHoursAwakePercent()`. |
| MetabolismInterval | float | **10.0** s (real-time) | [✓code value] — sleep accrual & metabolism share this tick |
| AwakeRatePerTick | float | **1.6667** (game-h/tick @ Idle) | [✓code default, FALLBACK] — engine auto-derives from world-clock TimeSpeed at BeginPlay (`ResolveAwakeRateFromWorldClock`, "one clock"); =(1/6)×10s at current pace. Tylko gdy brak zegara. |
| FatigueState (EFatigueState) | enum | Awake / MentalFog / Microsleeps / Collapsed | [✓ETAP 1 BUILT] — recomputed each tick from HoursAwake; `GetFatigueState()` BlueprintPure; orthogonal to bIsRested |
| MaxHoursBeforeCollapse / CollapseThreshold | float | **24.0** | [✓code value, ✓ETAP 1: FatigueState=Collapsed @ ≥24h] (ragdoll BP trigger = future ETAP) |
| Mental fog (work efficiency) | mult | **1.0@16h → 0.70@24h** (liniowo) | [✓ETAP 2 BUILT, commit poniżej] `GetWorkEfficiencyMultiplier()` = Lerp(1.0, 0.7, (HoursAwake-16)/8) klamp[0.7,1.0]; +RestedWorkBonus gdy bIsRested. Live-verified: 0.85@20h, 0.70@24h |
| MentalFogThreshold (mental fog onset) | HoursAwake | **16.0h** | [✓approved, ✓ETAP 1: MentalFog @ ≥16h] |
| MicrosleepThreshold (microsleeps onset) | HoursAwake | **20.0h** | [✓approved, ✓ETAP 1: Microsleeps @ ≥20h] |
| MicrosleepChancePerTick | float | **0.15** | [✓ETAP 2 BUILT, tune] szansa mikrosnu/tick gdy ≥20h **i `!bIncapacitated`** (omdlały nie mikrosnie — Bug 1 fix) |
| MicrosleepDuration | float (s) | **1.5** | [✓ETAP 2 BUILT, tune] długość mikrosnu (timer → `OnMicrosleepEnd`) |
| Collapse (omdlenie) | akcja @≥24h | BT StopLogic + StopMovement + ragdoll | [✓ETAP 2 BUILT] `bIncapacitated`, `OnCollapse()`; **EndPlay czyści timer+flagi (EC-1 fail-safe)** |
| SleepRecoveryPerTick | float | **4.0** (game-h/tick) | **[TBD→tune]** reset: `HoursAwake -= SleepRecoveryPerTick × TempQuality` gdy `bIsSleeping`. Live-verified: 8.33→0 |
| "Rested" buff (good sleep) | mult | **+0.2** do work efficiency (additive) | [✓ETAP 2 BUILT] `RestedWorkBonus`; `bIsRested=true` po `StopSleep` gdy HoursAwake→0 w komforcie. Live: WorkEff=1.2 |
| StartSleep() / StopSleep() | API (BT) | — | [✓ETAP 2 BUILT] BlueprintCallable; StopSleep wznawia BT (`RestartLogic`) + znosi `bIncapacitated` |
| ComfortTempMin/Max · ColdSleepMultiplier | float | 15 / 24 / 3.5 | **[✓AKTYWNE od AmbientTemp]** `GetTempQualityMultiplier()` liczy z **AmbientTemp** (komfort 15-24 OTOCZENIA → 1.0, poza → liniowo 0.075/°C, podłoga 0.25). ColdSleepMultiplier AKTYWNY w sprzężeniu (sen w zimnie ×3.5). Live: 0.475 w Ocean 8 ✓ |

### Climate / AmbientTemp (temperatura otoczenia — rdzeń strefowy, ETAP AmbientTemp)
| Variable | Value | Status |
|---|---|---|
| FZoneDef.BaseTemp (per biom) | Lava 80 · Caldera 45 · Desert 35 · AshSlope 28 · Savanna 26 · Oasis 22 · Grassland 20 · Beach 19 · SlopeForest 18 · River 14 · Ocean 8 · Mountain 4 | **[TBD→tune]** DT_ZoneDefs (data-driven) |
| AmbientTemp | = GetZoneBaseTemp() + DayNightTempOffset | **[✓BUILT+VERIFIED]** strefa (cache) + offset doby (warstwa doby AKTYWNA). Live: AshSlope 35.33 (południe) ↔ 20 (noc) |
| DayNightTempOffset (warstwa doby) | = DayNightAmplitude × (SunFactor − 0.5) × 2 | **[✓BUILT+VERIFIED]** `UpdateDayNightTempOffset()` co tick z zegara świata. SunFactor = SunIntensity/MaxSunIntensity (klamp 0..1). Noc −Amp, południe +Amp. ~~SunFactor niesie SEZON za darmo (zima=niższe SunIntensity)~~ 🔴 **DRIFT (recon 2026-06-19, live graf): NIEPRAWDA.** THE ATMOSPHERE liczy pik dzienny `Lerp(25,100,sin(π·T))` = 100 w KAŻDE południe niezależnie od sezonu (MaxSunIntensity=100 const); MaxElevationSun (sezonowy=37.5@DoY102) rusza tylko SunPitch, nie SunIntensity. Sezon zmienia długość dnia (THE CALLENDAR), nie amplitudę piku → noon SunFactor≈1.0 cały rok. Patrz REPORTS/raport_2026-06-19_ambienttemp_recon.md. Brak zegara → 0 |
| THE CALLENDAR — interpolacja sezonowa | DawnStart/DustStart = Lerp(A,B,sin(π·T)); MaxElevationSun/SunYaw = Lerp(A,B,T); T=(DayOfYear−start)/długość | **[✓FIXED 2026-06-19]** był BUG: `Divide_IntInt`+`T:int32` → T=0 → 4 skokowe stopnie. Naprawione bypassem (4× `Divide_DoubleDouble`, graf THE CALLENDAR). Live: DoY100 DayLenght 11.75→14.71, MaxElevSun 37.5→42.01. ZAPISANE+commit. GRANICE SEZONÓW: MaxElevSun płynie (łańcuch endpointów); DawnStart/DustStart/DayLenght NADAL skaczą (DayLen do −5.19h, np. DoY83→84 16.94→11.75) — osobny dług: endpointy sezonów nie łańcuchowane + krzywa lin/sin niespójna, NIE wina Bug #2. Patrz raport recon. |
| DayNightAmplitude | 8.0 | **[TBD→tune]** amplituda dobowa °C (noc −8, południe +8). Live verify: noc offset=−8, południe +7.33 |
| DefaultAmbientTemp (poza strefą) | 20 | [TBD→tune] |
| ColdAmbientThreshold (cold-burn split) | 11 | **[TBD→tune]** AmbientTemp<próg → ×2 spalanie tłuszczu (organizm grzeje; NIE temp ciała) |
| Sprzężenie ciało↔otoczenie (3 reżimy) | sen+ambient<15 → dryf ×ColdSleepMultiplier; jawa+ambient<ExtremeCold → dryf wolniej; else → termoregulacja ku setpoint 36.6 | [✓BUILT] hipotermia STOPNIOWA (fail-safe) |
| BodyCoolingRate / BodyWarmingRate | 0.05 / 0.08 | [TBD→tune] |
| MinBodyTemp / MaxBodyTemp / BodySetpointTemp | 25 / 42 / 36.6 | [TBD→tune] |
| ExtremeColdThreshold (jawa→hipotermia) | 0.0 | **[✓VERIFIED]** żadna strefa bazowo nie zamarza na jawie; ekstremum z warstwy doby. Live: Mountain noc = 4−8 = −4 < 0 → regime 2 jawa, `[Temp] HIPOTERMIA (ambient=−4.0, śpi=0)`, CurrentTemp 36.6→29.57 |
| InsulationFactor (brak izolacji) | 1.0 | [LOCKED, hook ekwipunek/ogień Eureka] |
| ZoneRecheckDistance (cache perf) | 500 UU | [TBD→tune] refresh strefy po ruchu > próg (stojący NPC = 0 zapytań GetZoneAtLocation) |
| CriticalTempThreshold (hipotermia priorytet) | 34 | [✓code, OŻYWIONY] CurrentTemp≤34 → Level_1_Temperature (był martwy) |

---

## 🛡️ POZIOM 2 — Bezpieczeństwo

### Inventory (per-compartment, Tarkov-style)
| Variable | Type | Value | Status |
|---|---|---|---|
| OwnerID | int32 | NPC id; PUBLIC_OWNER_ID = **-1** | [LOCKED] |
| Item stack size | int32 | (BP pin compat) | [LOCKED] |
| Container unequip requires empty (v1) | rule | yes | [LOCKED] |
| Theft hook | TryWithdraw → bUnauthorized | logs "THEFT:" | [LOCKED] |
| Hunger consume order | — | private first, then public storehouse | [LOCKED] |

### Body — 26 parts, cascade
**Health 0.0–1.0 per part. Effective = min(own, parent chain).** Absent in map = 1.0 (healthy).

**Sense weights (sum to 1.0 per sense):**

| Sense | Parts & weights | Status |
|---|---|---|
| Vision | LeftEye 0.50, RightEye 0.50 | [LOCKED] |
| Hearing | LeftEar 0.50, RightEar 0.50 | [LOCKED] |
| Speech | Tongue 1.00 | [LOCKED] |
| HandPrecision (each hand) | Hand 0.40, Thumb 0.30, Index 0.10, Middle 0.08, Ring 0.07, Pinky 0.05 | [LOCKED] |
| Mobility | LegL 0.35, FootL 0.15, LegR 0.35, FootR 0.15 | [LOCKED] |

**Injury types** (`EInjuryType`): None, Wound, Fracture, Sprain, Burn, Sever. [LOCKED]

**Sense → gameplay impact:**
| Sense | Gates | Status |
|---|---|---|
| Vision | EQS perception range, threat detection (L0), food finding (L1), witness ID (L5) | [LOCKED] |
| Hearing | off-screen threat / alarm detection | [LOCKED] |
| Speech | P2P contract success, interrogation testimony, social bonding | [LOCKED] |
| HandPrecision | craft quality, combat accuracy, tool use | [LOCKED] |
| Mobility | move speed, flee ability (L0) | [LOCKED] |

**Cascade example:** LeftArm 0.40 → LeftHand, all 5 fingers capped at 0.40 → HandPrecision(L) drops sharply.

### Medical / Illness
| Variable | Type | Value | Status |
|---|---|---|---|
| HealPart amount per treatment | float | **0.3** | [✓approved] |
| Raw meat PoisonChance | % | **0.35** (35%) | [✓approved] |

---

## 👥 POZIOM 3 — Przynależność

### OCEAN personality (Big Five)
| Trait | Range | BT effect | Status |
|---|---|---|---|
| Openness | **[TBD]** 0–1 | explore vs exploit | [TBD] |
| Conscientiousness | 0–1 | reliable gatherer (storehouse deposits) | [TBD] |
| Extraversion | 0–1 | seeks contracts/social | [TBD] |
| Agreeableness | 0–1 | low → breaks property law | [TBD] |
| Neuroticism | 0–1 | high → risky/panic-prone | [TBD] |

**Example rule [LOCKED concept]:** Neurotic + low-Agreeable → risky decisions, theft, fear in others.

### P2P Contracts & Registry
| Variable | Type | Value | Status |
|---|---|---|---|
| NPCRegistry: int32 → NPC | UWorldSubsystem | TMap<int32, TWeakObjectPtr> | [TBD — design with architect] |
| Daily cycle = 1 turn | — | morning plan, evening sync | [LOCKED] |
| Safe Zone size (initial group) | int | ~10 NPC | [LOCKED] |

---

## 👑 POZIOM 4 — Szacunek

| Variable | Type | Value | Status |
|---|---|---|---|
| Reputation "Value" | float | **start 0** | [✓approved] |
| Rep delta: successful P2P | float | **+1** | [✓approved] |
| Rep delta: rescue | float | **+5** | [✓approved] |
| Mastery tag threshold (turns at task) | int | **50** | [✓approved] |
| Monopoly "pay double" trigger | — | when Mastery tag present | [LOCKED concept] |

---

## 🚀 POZIOM 5 — Samorealizacja / Detektyw

### Action Log (alibi system)
| Field | Type | Status |
|---|---|---|
| Time | float/text | [LOCKED] |
| Zone | id/name | [LOCKED] |
| Action | tag | [LOCKED] |
| WitnessedNPCs | array<int32> | [LOCKED] |

| Variable | Type | Value | Status |
|---|---|---|---|
| Log retention | turns | **1 day** | [✓approved] |
| Lie detection | contradiction in cross-referenced logs | [LOCKED concept] |
| Penalty: reputation loss | float | **−10 rep** | [✓approved] |
| Penalty: banishment threshold | rep value | **≤ −20** | [✓approved] |

### Eureka
| Variable | Type | Value | Status |
|---|---|---|---|
| Innovation points (all needs met) | float | **[TBD]** | [TBD] |
| Feudal tithe | % storehouse | **10%** | [LOCKED] |

---

## 🗺️ MAPA — Caldreth zones (DT_ZoneDefs)

Flagi per biom — `bSpawnable` (mogą się spawnić zasoby/propsy/NPC), `bHabitable`
(NPC mogą się osiedlić). Źródło: `Tools/MapGen/zone_defs.json` → `DT_ZoneDefs`
(row struct `FZoneDef`). Zatwierdzone przez architekta 2026-06-17.

| Biom | Spawnable | Habitable | Status |
|---|---|---|---|
| Ocean | ✗ | ✗ | [✓approved] |
| Beach | ✓ | ✗ | [✓approved] start rozbitków |
| Savanna | ✓ | ✓ | [✓approved] osada |
| Desert | ✗ | ✗ | [✓approved] |
| Grassland | ✓ | ✓ | [✓approved] osada |
| SlopeForest | ✓ | ✓ | [✓approved] osada |
| Mountain | ✗ | ✗ | [✓approved] |
| AshSlope | ✗ | ✗ | [✓approved] |
| Caldera | ✗ | ✗ | [✓approved] |
| River | ✗ | ✗ | [✓approved] sama woda (nurt); brzeg = Oasis ✓/✓ |
| Lava | ✗ | ✗ | [✓approved] |
| Oasis | ✓ | ✓ | [✓approved] osada |

> Habitable ⇒ Spawnable (osada musi mieć z czego żyć). DebugColor per biom w zone_defs.json.

## 🔢 GLOBAL CONSTANTS
| Constant | Value | Status |
|---|---|---|
| Target NPC count | 500+ | [LOCKED] |
| PUBLIC_OWNER_ID | -1 | [LOCKED] |
| No Event Tick anywhere | rule | [LOCKED] |
| Health range (body parts) | 0.0–1.0 | [LOCKED] |
| Acuity/sense range | 0.0–1.0 | [LOCKED] |

> **[TBD] count:** ~18 values await design decisions. Most are tuning numbers safe to
> prototype, but anything that changes a SYSTEM'S BEHAVIOR (OCEAN→BT mapping, NPCRegistry
> structure, contract rules) must be approved by the architect before building.
