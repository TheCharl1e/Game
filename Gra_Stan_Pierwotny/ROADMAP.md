# ROADMAP — Projekt "Stan_Pierwotny" (Matrix NPC)

> Master task tracker. THE canonical roadmap. Claude Code reads this at session start.
> Replaces any flat T1–T15 working list — that scheme is retired; this is the single source of truth.
> Sorted by Maslow pyramid level. Granular: one row per component/patch.
> Update STATUS after every task. Never start a task whose `Depends` aren't ✅.

## Legend
| Sym | Status | | Tag | Priority |
|---|---|---|---|---|
| ✅ | DONE (built + compile/editor-confirmed) | | P0 | Critical path / blocks others |
| 🔨 | IN PROGRESS (started, not confirmed) | | P1 | Core gameplay, do soon |
| ⬜ | TODO (not started) | | P2 | Important, not urgent |
| 🔒 | BLOCKED (waiting on a dependency) | | P3 | Polish / late-game |

---

## 🚨 BLOKERY TERAZ (rozwiąż w tej kolejności)
1. ✅ **BUILD-01 — ZROBIONE 2026-06-16.** Pełny rebuild (edytor zamknięty) → Result: Succeeded; 4 komponenty zielone, moduł zlinkowany. (Wymagało wymuszenia UHT — Live Coding zatruł makefile.) L2 odblokowane.
2. ✅ **SUP-01b — ZROBIONE 2026-06-16.** Gap A (GetPartDisplayName → title) + Gap B (collapse PartRow_0..6 before Switch) wykonane przez Monolith (add_node+connect_pins+set_pin_default, read-back potwierdzony, compile czysty). BONUS: TEST ApplyDamage(LeftArm,0.6,Fracture) na BP_NPC_Character BeginPlay (oznaczony "TEST ONLY - remove later") + usunięty duplikat komponentu BodyCondition (został BodyConditionComp).
3. Po buildzie: zweryfikuj 4 komponenty zielone → flip statusów.

## 🎯 NEXT ACTIONS (po blokerach)
- ✅ **L3-01 NPCRegistry** ⭐ keystone — DONE 2026-06-20 (build+PIE-verified). Odblokowane: P2P (L3-05), reputacja (L4-01), ActionLog/detektyw (L5-01).
- ✅ **L3-02 OCEAN slice #1** — DONE 2026-06-20 (Neurotyczność→panika, PIE-verified, **ŻYWY W GRZE**). Następnie: DataAsset-hybryda + L3-03 (OCEAN jako filtr BT, mnożniki wag) lub **L5-01 ActionLog**.
- **L2-03 DataTables** — DT_FoodStats, DT_ItemDefinitions, wpięcie DT_ActionCosts.
- **TECH-01 cleanup** — redirectory, duplikaty _Experyment. (TECH-03 literówki BB ✅ done.)

---

## 🛠️ BUILD & INFRASTRUKTURA

| ID | Task | Status | Prio | Depends | Notes |
|---|---|---|---|---|---|
| BUILD-01 | Full rebuild, editor CLOSED (UHT + compile) | ✅ | P0 | — | Done 2026-06-16, Result: Succeeded (needed forced UHT regen — Live Coding poisoned the makefile) |
| BUILD-02 | Verify 4 C++ components compile green | ✅ | P0 | BUILD-01 | All 4 compiled + linked into UnrealEditor-Stan_Pierwotny.dll (2026-06-16) |
| INFRA-01 | Claude Code installed (Opus 4.8, Max) | ✅ | — | — | Working in E:\Game |
| INFRA-02 | CLAUDE.md (9 zasad) | ✅ | — | — | |
| INFRA-03 | PROJECT_STATE.md | ✅ | — | — | |
| INFRA-04 | Script cache policy (Scripts/) | 🔨 | P1 | — | Policy set; populate from session scripts |
| INFRA-05 | mcp-unreal server (Go + plugin + register) | ✅ | P0 | — | WORKING — Claude Code drives editor via MCP |
| INFRA-05b | mcp-unreal doc index source path | 🔒 | P2 | — | Index built but total_docs=0; needs UE docs source. lookup_docs empty until fixed |
| INFRA-06 | Git init + .gitignore + first commit | ✅ | P0 | — | master e14fa24. Safety net live |
| INFRA-07 | Docs split (ARCHITECTURE.md + DECISIONS.md) | ✅ | P2 | — | Done 2026-06-12 |
| INFRA-08 | Test "laboratory" level (empty map, 2-3 NPC) | ⬜ | P1 | BUILD-02 | Isolated system testing, clean logs |
| INFRA-09 | Commit-before-big-change habit | 🔨 | P1 | INFRA-06 | git commit before each AI patch batch |

---

## 🧹 DŁUG TECHNICZNY (TECH-DEBT)

| ID | Task | Status | Prio | Depends | Notes |
|---|---|---|---|---|---|
| TECH-01 | Fix Up Redirectors (cała Content) | ⬜ | P2 | — | After moves/renames; prevents broken refs |
| TECH-02 | Usuń duplikaty _Experyment | ⬜ | P2 | — | PROJECT_STATE §5 |
| TECH-03 | Napraw literówki kluczy Blackboard | ✅ | P1 | — | **DONE 2026-06-19 (commit 1e2540b):** Nearst→Nearest w BB_NPC (4 klucze) + ref w BT_NPC/BTTask_Eat/FindFood_Experyment/FindWater_Experyment. Integralność struktury potwierdzona (FindWater K2Node 41==41, /Script/ 16==16; −4KB = re-serializacja, nie utrata węzłów). |
| TECH-04 | Rozdziel 6 gier-szablonów od rdzenia (MOBA/RTS/Soulslike/itp.) | ⬜ | P2 | — | Move template experiments to a branch/folder; keep module focused on the sim |
| TECH-05 | Wyczyść ROADMAP/PROJECT_STATE z duplikatów wierszy | 🔨 | P3 | — | Claude Code's hasty edits left mangled rows |
| TECH-06 | EditorStartupMap → CaldrethMap (GameDefaultMap zostaje Game) | ✅ | P2 | — | **DONE 2026-06-18 (commit 0091100):** `EditorStartupMap=/Game/DocelowaGra/CaldrethMap.CaldrethMap`, zweryfikowane (relaunch → wyspa widoczna). **GameDefaultMap CELOWO zostaje `/Game/DocelowaGra/Game.Game`** (populowana mapa NPC) — NIE wskazywać na CaldrethMap, bo ma 0 NPC + brak NavMesh (gra startowałaby w pustym świecie). **Przełączyć GameDefaultMap→CaldrethMap GDY** dostanie spawn NPC + NavMesh. NIE ruszać mapa.umap. |
| TECH-07 | `GetHPPercent()` dzieli przez literał 100, nie przez `CurrentMaxHP` | ⬜ | P2 | — | **Znaleziono 2026-06-20 podczas L3-02.** `MaslowBiologicalComponent.cpp:637-640` zwraca `CurrentHP/100.0` (= AbsoluteMaxHP), NIE `CurrentHP/CurrentMaxHP`. Autofagia/odwodnienie OBNIŻA `CurrentMaxHP` (i `CurrentHP` jest do niego klampowane) → wyniszczony NPC przy PEŁNYM aktualnym zdrowiu czyta `GetHPPercent()=0.5`. Paski HUD błędnie czytają po wyniszczeniu. Fix: dzielić przez `CurrentMaxHP` (guard >0). L3-02 panika świadomie NIE używa tego gettera (liczy własne HP%). |
| TECH-08 | ~~BP_NPC Event Tick pinuje CurrentHP=100 co klatkę~~ → Dwa rozłączone systemy metabolizmu | ⬜ | P3 | — | ~~**Stary opis (pin HP)**~~ — **MISDIAGNOZA 2026-06-20: brak pinu HP, było `TRASH_` artefakt pomiaru** (get_component zwracał martwy/reinstancowany komponent; write znikał, świeża instancja czytała default CDO=100; recon przeczytał WSZYSTKIE grafy ciągłe — żaden nie pisze CurrentHP Maslowa). **REALNY TEMAT (przeramowany, P3):** BP_NPC prowadzi WŁASNY system potrzeb (`HungerLevel`/`ThirstLevel`/`SleepLevel` + struct `MetabolismStats`: Kcal/Glikogen/Tłuszcz/Białko/Stamina, napędzany TimeManagerem) RÓWNOLEGLE do C++ `UMaslowBiologicalComponent`. **RECON 2026-06-20** (`REPORTS/raport_2026-06-20_metabolizm_recon.md`): wyborem akcji sterował WYŁĄCZNIE BP (`EvaluateNeeds→CurrentNeed` enum); serwis C++ był rozkonfigurowany (6 selektorów w `CurrentNeed`, zły typ) i **zerował `CurrentNeed` co tick**; kaskada kataboliczna policzona, nieczytana. **KIERUNEK (gate `MASLOW_BT_BRIDGE_design.md`, OK dyrektora): C++ steruje, BP = cienkie ciało.** **PLASTER #1 DONE 2026-06-20** (`raport_2026-06-20_maslow_bt_bridge_plaster1.md`): `GetActionableNeed()`→serwis pisze JEDEN klucz `SetValueAsEnum(CurrentNeed)`; BP `EvaluateNeeds` zapis klucza ZAMROŻONY (odpięty, odwracalny). PIE-verified: zepsucie C++ Hydration→`CurrentNeed=Thirst`→BT Handle Thirst (BP ThirstLevel=0). ~~🚩 REFINEMENT: stan C++ re-inicjalizowany co tick~~ → **WYCOFANE 2026-06-20 (recon TRASH_, `raport_2026-06-20_trash_recon.md`): to był H2 — artefakt TRASH_ skażonej sesji, NIE re-init.** Na świeżym edytorze stan C++ jest TRWAŁY per-instancja (set `CriticalHydrationThreshold=99` trzyma przez ticki; hydration drenuje legalnie, nie regeneruje). **Most jest CZYSTY — nie ma „kto karmi C++".** Plastry mostu: #2 głód, #3 sen+fix pułapki omdlenia, #4 panika/Flee (ożywia L3-02), #5 sprzątanie+kasacja BP need-calc. |
| TECH-09 | ~~Churn komponentów `TRASH_` w PIE~~ — ZAMKNIĘTE: artefakt | ✅ | P3 | — | **ZAMKNIĘTE 2026-06-20 (`raport_2026-06-20_trash_recon.md`).** Na ŚWIEŻYM edytorze + świeżym PIE: `get_components_by_class` = **count 1, zero TRASH_, brak inkrementu suffixu**. Churn z porannej/plaster-#1 sesji = **artefakt zmęczonej sesji edytora** (długi run: rebuild + reopen + wiele start/stop PIE + reanimacje). NIE pętla respawnu. Dowód H2: set `CriticalHydrationThreshold=99` trzyma przez ticki na jedynej, trwałej instancji. **Higiena:** weryfikacje iniekcyjne rób na świeżym edytorze. |
| TECH-10 | BP-drugi-mózg re-aplikuje BP-defaulty na pola C++ Maslow co kadencję (parallel-writer) | ⬜ | P2 | — | **Znaleziono 2026-06-22 (recon FAZY 2, ŚWIEŻY edytor → to NIE ghost).** Częściowo PODWAŻA „Most CZYSTY" z TECH-08/09: tamten recon weryfikował tylko Hydration/Glucose (te FAKTYCZNIE persist, jak HoursAwake). ALE `BodyFat`→**1500**, `GlycogenReserves`→**1000**, `CollapseThreshold`→**24** snap-back KAŻDĄ kadencję na świeżej sesji → żywy writer (BP `MetabolismStats`: Tłuszcz/Glikogen) nadpisuje pola C++. **One-writer violation, rodzeństwo plastra #1** (BP `EvaluateNeeds`). Należy do **plastra #5** (kasacja BP need-calc). **Blokuje akumulację BodyFat (APPETITE/grubas — fat nie urośnie jak resetowany co tick).** NIE blokuje głodu (Glucose persist → głód wymuszalny). Pamięć: `maslow-bp-resets-cpp-fields-per-cadence`. |
| TECH-11 | Food-tracking array (Food/HowMuchFood/Drink/Shelter) żyje w BP_NPC_AI zamiast w mózgu C++ | ⬜ | P2 | — | **KIERUNEK, nie drobiazg (2026-06-22, FAZA 2).** Tablica `Food` + licznik `HowMuchFood` na BP_NPC_AI, zapełniane przez perception `OnTargetPerceptionUpdated` (ActorHasTag→Add, `HowMuchFood+=1`), czytane przez C++-bramkowaną gałąź BT. **Perception tylko DODAJE, nigdy nie usuwa** → HowMuchFood rośnie monotonicznie i kłamie, tablica puchnie null-ami po DestroyActor (×500 = wyciek + rosnący koszt ForEach). To **ten sam wzorzec co TECH-10 (BodyFat/Glycogen w BP) i plaster #1 (CurrentNeed w BP)** — stan świata trzymany w „ciele" zamiast w „mózgu". Objawy (prune, kłamliwy licznik, brak perception-loss cleanup) znikną jako KLASA dopiero po migracji do **C++ food-registry (analogia NPCRegistry/L3-01)**. Slice 1b łata objaw: FindFood `IsValid`-ForEach guard (zero Accessed-None) + `PruneFoodArray` (RemoveItem null + recompute HowMuchFood, autoring ręczny — Monolith nie umie wildcard array). Dług kierunkowy: **kandydat do objęcia przez plaster #5 (kasacja BP need-calc)**. Pod-dług: prune łapie ZNISZCZONE food, NIE żywe-poza-percepcją (to perception-loss cleanup, osobny mechanizm). **— DECYZJA MODELU 2026-06-22 (recon decyzyjny, fakty nie gust):** jedyna prawda percepcji = **AIPerception_Sight** — nadzbiór cone-vision: FOV+dystans PLUS histereza (SightRadius 1500 / LoseSightRadius 2000) + event-driven + loss-events + niski koszt ×500; cone (ScanForFood/Drinks) NIE niesie żadnego unikatu. **`ScanForFood`/`ScanForDrinks` + `VisibleFoods`/`VisibleFoodsCount` (BP_NPC_Character) = MARTWY DUPLIKAT (klasa cold-burn): ZERO acting-konsumentów** (czytane tylko w samym skanie, Contains-dedup) + brute `GetAllActorsOfClass(BP_Food)`×wywołanie×500. **ZAKRES kroku 1 (percepcja TERAŹNIEJSZA → C++, weak-ptr + loss-handler):** (1) **kasacja cone** (ScanForFood/Drinks+VisibleFoods) W ZAKRESIE — jedna prawda od razu, zero martwego kodu „na potem"; (2) **MaxAge ZOSTAJE 0** — percepcja = „widzę TERAZ" bez zapominania; loss-handler (`WasSuccessfullySensed=false`) usuwa z „widzę teraz", w kroku 2 ten sam sygnał przepisze fakt do pamięci (hak teraz, mechanika potem); (3) **per-NPC config (zasięg/kąt/VisionAcuity/OCEAN) = SUP-03, POZA krokiem 1** — dziś AIPerception class-default (1500/2000/90), cone hardkod 100; po migracji C++=właściciel → wstrzyknięcie z DataAsset to jeden punkt. Rozłączne od plastra #5 (drugi-mózg = inny BP). **KROK 2 = osobny gate: PAMIĘĆ NPC** (generyczna pamięć świata, drabina Disappointed/cond.1.4) — MUSI czekać na czystą jedną-prawdę percepcji (pamięć czyta z percepcji). Architekt projektuje gate scalenia; kod po „zatwierdzam". |
| TECH-11b | Drink/Shelter-percepcja → C++ (OSOBNO, kopia wzorca TECH-11 Food) | ⬜ | P2 | TECH-11 | **DŁUG JAWNY (2026-06-22, wydzielony z gate TECH-11 krok 1 — zakres tamtego = TYLKO Food).** Po scaleniu Food do `UNPCPerceptionComponent`: skopiować wzorzec na **Drink** (ma ŻYWEGO konsumenta — BP FindWater, AIPerception aktywne) i **Shelter** (AIPerception, BEZ konsumenta na dziś). Każdy zmysł-cel = **osobna tablica `TWeakObjectPtr` + gain/loss + getter** (różne tagi/konsumenci — NIE wrzucać do jednej tablicy z Food). Kasacja cone w TECH-11 obejmuje martwy `ScanForDrinks`, ale **`BP_NPC_AI.Drink` ZOSTAJE** do tego scalenia. NIE scalać razem z Food — osobny task, osobny verify. |
| TECH-12 | TOOLING: granica Monolith — nie rekonstruuje świeżo autorowanych węzłów rozwiązujących typ z POŁĄCZENIA | ⬜ | P3 | — | **UTRWALENIE 2026-06-22 (pamięć `monolith-no-wildcard-node-reconstruction`).** Monolith NIE zrekonstruuje świeżo autorowanych węzłów BP, których typ pinu rozwiązuje się DOPIERO z podłączenia: **wildcard struct-piny** (montage-builder), **wildcard array-piny** (`RemoveItem`/`Length`), typed-object func-params→bool. **OBEJŚCIE:** napisać **C++ helper z KONKRETNYM typem pinu** (np. `CompactNullActors(TArray<AActor*>&)`) → Monolith spina czysto. Prymitywy BP „rozwiązywane przeciągnięciem typu" = **ręczny autoring w edytorze**, nie Monolith. Eskalować, nie re-potwierdzać. |

---

## 🩸 POZIOM 0 — Stres (Fight or Flight)

| ID | Task | Status | Prio | Depends | Notes |
|---|---|---|---|---|---|
| L0-01 | EMaslowPriority enum (Level_0_FightOrFlight) | ✅ | — | — | In MaslowBiologicalComponent.h |
| L0-02 | Panic override: abort BT task queue on threat | ⬜ | P1 | L1-05, L0-04 | Top BT priority branch, reads IsInPanic BB key |
| L0-03 | Flee behavior (drop loaded container, run to Safe Zone) | ⬜ | P1 | L0-02, L3-01, L2-12 | Panic loot-drop via DropContainer() |
| L0-04 | Threat detection → panic (predator via senses/EQS) | ⬜ | P1 | L2-07, SUP-03 | VisionAcuity gates detection range. BP_NPC_WOLF = threat |

---

## 🫀 POZIOM 1 — Fizjologia

| ID | Task | Status | Prio | Depends | Notes |
|---|---|---|---|---|---|
| L1-01 | MaslowBiologicalComponent core | ✅ | — | — | Pre-existing, async timers, hidden stats |
| L1-02 | Catabolic cascade (Glucose→Glycogen→Fat→Protein) | ✅ | — | L1-01 | EHungerPhase Phase_0..Phase_4_Death |
| L1-03 | Getters patch (Glycogen%/BodyFat%/HoursAwake%) | 🔨 | P1 | BUILD-02 | Applied? Confirm on rebuild. HUD binds these |
| L1-04 | BTService_MaslowBlackboardSync (C++→BB bridge) | ✅ | — | L1-01 | Compiled |
| L1-05 | BB keys (6) + service on BT_NPC root (editor) | 🔨 | P1 | L1-04, TECH-03 | Manual editor wiring; watch BB key typos |
| L1-06 | Sleep model: HoursAwake increment timer | ⬜ | P1 | L1-03 | Timer ++HoursAwake, reset on sleep |
| L1-07 | Sleep debuffs (fog -30%, microsleeps, passout ragdoll) | ⬜ | P1 | L1-06 | Ragdoll/anim via BlueprintImplementableEvent |
| L1-08 | Rested buff (+20% work/eureka) | ⬜ | P2 | L1-06 | |
| L1-09 | Temperature system (day/night + fire/clothing) | 🔨 | P1 | L1-01, L2-01 | **AmbientTemp (strefy) + warstwa doby DONE+VERIFIED** (cold ×fat-burn AKTYWNE, hipotermia STOPNIOWA). ZOSTAJE: fire (heat source) + clothing = Inventory equip izolacja |
| L1-10 | Thirst activity scaling (rest×1/work×3/combat×5) | 🔨 | P2 | L1-01 | Verify present in component |
| L1-11 | APPETITE / Grubas slice 1 (proces jedzenia + makra→magazyn + rozpychanie żołądka + fat→izolacja) | ✅ | P1 | L1-02 | **DONE 2026-06-21 (commit `545a95d`).** runaway (LeptinBrake=0). Start/Bite/Stop + DepositBiteMacros (carb→fat0.75/fat0.95/protein0.5) + GastricCapacity dual-driver + SatietyOverfillFactor1.15 + StartingBodyFat1500 + UAnimNotify_EatBite + AItemBase.RemainingPortion. **PIE-VERIFIED BLOK 0/1/2/4** (InsulationFactor0.88, stretch 100→104.5→106.2, makra delty). Gate APPETITE_GRUBAS_design.md. |
| L1-11b | **APPETITE slice 1b — wiring BTTask_Eat → StartEatingItem/ConsumeBite/StopEating** | ✅ | **P1** | L1-11 | **DONE + PIE-VERIFIED 2026-06-22.** Pierwsza pełna żywa pętla jedzenia: głód→FindFood→MoveTo dochodzi→`StartEating Volume=1 bites=5 Carb=15`→5×ConsumeBite→`StopEating(Finished) mealSize=1.0`→`OnItemDepleted`→DestroyActor. **Verify (twarde liczby):** happy path ✓ (BodyFat 1500→1500.3); EC-EAT-2 ✓ (`StopEating(SourceGone)` gdy food zniszczone w trakcie); **fail-safe** ✓ (`StartEatingItem REFUSED — row 'Nonexistent' not found`, czytelny owner-name `BP_NPC_Character_C_1`, zero StartEating/wisu); **prune** ✓ (po zjedzeniu `Food len=0, HowMuchFood=0` — realny licznik, nie kłamliwy; `CompactNullActors`+`PruneFoodArray` w FindFood); **block-3** ✓ (Accessed-None=0). Commity C++: `eeba93c` StartEatingItem, `e25ed2d` log-fix, `3b2d6a3` CompactNullActors. Assety untracked (backup `2026-06-22_eat-anim`+`faza2-rewire` PRE/POST/FINAL). **DŁUGI JAWNE (ROADMAP):** Delay(7.6)→OnMealEnd (rozjazd early-satiety/SourceGone); ConsumeFood martwa para; TECH-10 BodyFat-reset (grubas nie utyje); TECH-11 food-tracking-w-BP (kierunek C++ food-registry); perception-loss cleanup (prune łapie zniszczone, nie żywe-poza-percepcją). ~~PENDING PIE VERIFY~~ HIST: dep1 ✅ (`AS_NPC_Eat` + 5× `AnimNotify_EatBite` @1/2.5/4/5.5/7s) + dep2 ✅ (`BP_Food` reparent→`AItemBase`, `FoodTableRowName="Berries"`). **C++ helper `StartEatingItem(AItemBase*,UDataTable*,int32)`** (commit `eeba93c`, editor-closed UHT build green) — rozwiązuje row→FFoodItemRow w mózgu, bool fail-safe (IsValid/FoodTable/Row guardy → Warning+false, StartEating tylko gdy rozwiązano). **BTTask_Eat:** Event→Cast BP_Food→Cast NPC_Character→`StartEatingItem(food,DT_FoodStats,5)`→Branch(bool)→[true] `PlaySlotAnimationAsDynamicMontage(AS_NPC_Eat)`→Delay(7.6)→Finish(true); [false/CastFailed] Finish(false). **Kolejność: StartEatingItem→Branch→montaż TYLKO na true** (nigdy przed/równolegle). **BP_Food:** `OnItemDepleted→DestroyActor`. **FindFood guard:** `IsValid(NearestFood)`→Branch przed SetValueAsVector (pusta tablica→Finish-false, kasuje Accessed-None). **DŁUGI ŚWIADOME:** (a) **Delay(7.6)≈animlen domyka task zamiast OnMealEnd** → rozjazd przy early-satiety (task wisi do 7.6s) i StopEating(SourceGone) (czeka na martwej sesji); docelowo OnMealEnd(BIE, istnieje)→FinishExecute event-driven. (b) `ConsumeFood`(×1.0) ← `ConsumeFoodItem` ← nikt = **martwa para** (NIE aktywny drift, bo nieosiągalna); usunięcie obu = director-call (find-refs 2026-06-22). (c) [[maslow-bp-resets-cpp-fields-per-cadence]]/TECH-10 BodyFat-reset blokuje akumulację grubasa. **VERIFY blok (wymagany):** happy path (StartEating→~5 ConsumeBite→StomachFill/Glucose rosną, RemainingPortion maleje→Stop) + **ścieżka-false** (zepsuty RowName→`REFUSED row not found`, Finish-false, ZERO bIsEating) + **zawrót pętli** na guard FindFood (brak Accessed-None). Backupy: `2026-06-22_eat-anim` + `2026-06-22_faza2-rewire` (PRE+POST). ~~BLOCKED~~ HIST: Rdzeń C++ gotowy (Start/Bite/Stop BlueprintCallable + `UAnimNotify_EatBite`). **dep1 ✅ ROZWIĄZANY 2026-06-22:** placeholder anim jedzenia `AS_NPC_Eat` (`/Game/DocelowaGra/AI_NPC/`, duplikat DocelowaGra MM_Idle na SK_Mannequin NPC) z **5× `AnimNotify_EatBite`** @ 1.0/2.5/4.0/5.5/7.0s (read-back OK, backup `2026-06-22_eat-anim`). Dostarczony jako SEKWENCJA, nie UAnimMontage — Monolith montage-builder zostawił `duration=0` (segment nie wszedł w długość, notify nie dało się dołożyć; `slot_anim_tracks` nieosiągalne z py). Rewire odegra ją via `PlaySlotAnimationAsDynamicMontage` (odpala notify→ConsumeBite — funkcjonalnie równoważne). Pose = idle-placeholder (realna anim jedzenia później). **dep1 (HIST, STOP-GATE):** notify był na ZERO animacjach → StartEating wisiał (bIsEating=true, 0 kęsów, wyglądał jak sukces). **dep2:** `BP_Food` to plain `Actor`, NIE `AItemBase` → `ConsumeBite` `Cast<AItemBase>` (nadgryzienie/`OnItemDepleted`) padnie + brak `FoodTableRowName` → brak ścieżki na `FFoodItemRow Meal`. Świat-jedzenie i C++-API jedzenia (AItemBase/FFoodItemRow/DT_FoodStats) = dwa nierozłączone systemy; reparent BP_Food→AItemBase lub most innym sposobem = decyzja zakresu. **Pkt 2 OK:** DT_FoodStats row=`FoodItemRow`(FFoodItemRow). **Guard BP-null w FindFood (Accessed None NearestFood przy pustej tablicy) — w zakresie tego rewire'u.** Pamięć: `maslow-bp-resets-cpp-fields-per-cadence` (TECH-10 blokuje też akumulację BodyFat). Pierwotny plan: wpiąć notify na anim + Start/Bite/Stop zamiast instant; legacy `ConsumeFood` fat×1.0 wyrzucić/wyrównać do ×0.95. Verify na świeżym edytorze. |
| L1-11v | **APPETITE slice 1-verify — capture BLOK 3 (shrink) + BLOK 5 (timeline izolacji) + kaskada starvation** | ⬜ | P2 | L1-11 | **VERIFY-DEBT (NIE pominięte):** real-time capture, świeży NPC, **slomo ≤8 + JAWNIE dolewaj wodę** (slomo zabija odwodnieniem zanim dojdzie do starvation), dedykowany skrypt per blok. BLOK3: shrink GastricCapacity ku Base100 przy bIsFasting (phase≥FatBurn). BLOK5: gruby vs chudy NPC w Ocean8 → InsulationFactor niższy u grubego → CurrentTemp stygnie wolniej (porównaj timeline). + czysta kaskada Glucose→Glycogen→FatBurn→Autophagy@StartingBodyFat1500. Blokada harnessu: reinstance-na-`set` wipe do BeginPlay. |

---

## 🛡️ POZIOM 2 — Bezpieczeństwo i Stabilność

| ID | Task | Status | Prio | Depends | Notes |
|---|---|---|---|---|---|
| L2-01 | InventoryComponent (per-compartment, ownership) | ✅ | P0 | BUILD-01 | Compiled green 2026-06-16. Tarkov-style, OwnerID int32 |
| L2-02 | ItemBase ownership patch (AActor*→int32 OwnerID) | ⬜ | P1 | BUILD-01 | Patch may not be applied yet — verify, then build |
| L2-03 | DataTables (DT_ItemDefinitions / FoodStats / ActionCosts) | ⬜ | P1 | BUILD-02 | Create + fill rows + new storage fields; wire DT_ActionCosts |
| L2-04 | Public Storehouse actor (PUBLIC_OWNER_ID inventory) | ⬜ | P1 | L2-01 | Village "anthill" pantry per Safe Zone |
| L2-05 | "Carry surplus to storehouse" BT behavior | ⬜ | P2 | L2-04 | Uses TransferTo (exists) |
| L2-06 | "Eat own first, then public" consume logic | ⬜ | P1 | L2-04 | Hunger: scan private → then storehouse |
| L2-07 | BodyConditionComponent (26-part senses, cascade) | ✅ | — | — | Compiled 06-09, in loaded DLL, on BP_NPC_Character |
| L2-08 | Medical: HealPart treatment via items (bandage/splint) | ⬜ | P2 | L2-07, L2-01 | RequiredTreatmentItem → consumes item |
| L2-09 | Illness from raw meat (PoisonChance) | ⬜ | P2 | L2-03, L2-07 | DT_FoodStats PoisonChance → wound/sickness |
| L2-10 | Crafting/employment: skills, specializations | ⬜ | P2 | BUILD-02 | Lumberjack/miner/builder; feeds L4 mastery |
| L2-11 | Equipment drag&drop full wiring (HUD) | 🔨 | P2 | L2-01, SUP-01 | EquipItem/UnequipSlot via D&D |
| L2-12 | DropContainer() — drop loaded container (panic/theft) | ⬜ | P2 | L2-01 | The "drop pack while fleeing = lose contents" mechanic |

---

## 👥 POZIOM 3 — Przynależność i Socjalizacja

| ID | Task | Status | Prio | Depends | Notes |
|---|---|---|---|---|---|
| L3-01 | ⭐ NPCRegistry (UWorldSubsystem, int32→NPC) | ✅ | **P0** | BUILD-02 | **KEYSTONE — DONE 2026-06-20.** `UNPCRegistrySubsystem` (Register/Unregister/GetNPCById/GetRegisteredCount, int32 ID start=1 nie-recyklowane, self-heal) + `UNPCIdentityComponent` (register BeginPlay / unregister EndPlay, EC-1..6), gate-exact (zero drift). Build.cs include path; pełny UHT build zielony (53.6s); komponent dodany do BP_NPC_Character (zapisany). **PIE-verified:** ID 1/2 rejestracja (count 1→2), nowy NPC→ID 3 (nie-recykling), EndPlay→`ID retired`, count→0; zero per-lookup spam. (Lookupy GetNPCById nie hard-live — UWorldSubsystem nieosiągalny z tooling; ścieżka trywialna. **Live-verify GetNPCById ← do domknięcia przy PIERWSZYM konsumencie tożsamości: L5 ActionLog lub L2 OwnerID→NPC; NIE osobny mini-gate.**) **Odblokowuje P2P (L3-05) + Reputację (L4-01) + ActionLog/Detektyw (L5-01).** |
| L3-02 | OCEAN personality data (UDataAsset/DataTable) | ✅ | P1 | — | **Slice #1 DONE 2026-06-20 (commit 48e1a73):** `FOceanProfile` (5 named floats 0–1) na UNPCIdentityComponent (per-instancja, EditAnywhere) + Neurotyczność→szansa paniki (stochastyczny rzut na kadencji metabolizmu, latch `bIsInPanic`, histereza wyjścia). PIE-VERIFIED killer-demo (ta sama rana HP%=0.35: C1 N=0.9→PanicChance0.25→ENTER panic; C2 N=0.1→PanicChance0.00→spokój). ⚠️ **KOREKTA 2026-06-20 (recon metabolizmu): panika NIE dociera do BT** — żaden wpięty dekorator nie czyta kluczy Maslow (`IsInPanic`), gałąź „Zagrożenie" odłączona od korzenia. OCEAN/`bIsInPanic` jest **policzony i poprawny w C++, ale BEZ wpływu na zachowanie DO mostu Maslow→BT.** „Żywy w grze" dotyczyło wyłącznie linii logu z `EvaluatePanicRoll` (transition), nie zmiany akcji NPC. Most ożywi panikę w **plastrze #4** (`GetActionableNeed` zwraca Flee; re-attach gałęzi „Zagrożenie"). HP-pin był MISDIAGNOZĄ (artefakt TRASH_, TECH-08). DataAsset-hybryda + dryf + L3-03 = follow-up. |
| L3-03 | OCEAN as BT filter (traits modify behavior) | ⬜ | P1 | L3-02, L1-04 | Neurotic+low-Agreeable → risky/theft-prone |
| L3-04 | Daily cycle as turn + plan register ("blockchain") | ⬜ | P1 | L3-01 | Morning: eval Maslow → queue day's actions |
| L3-05 | P2P contract system (FContract, virtual pool) | ⬜ | P1 | L3-01, L2-01 | NPCs post/accept barter quests |
| L3-06 | Contract BT tasks (offer/accept/fulfill barter) | ⬜ | P1 | L3-05 | |
| L3-07 | Safe Zone assignment via EQS | 🔨 | P1 | — | EQC_SearchZone/EQS_ZoneSearch exist; assign NPCs to zones |
| L3-08 | Family/friendship bonds (relationship values) | ⬜ | P2 | L3-01 | Raises psychological stability |
| L3-09 | Group challenges raise trust (kill big spider) | ⬜ | P3 | L3-05 | Shared big-fight → +friendship/trust |
| L3-MEM | ⭐ GATE: Pamięć NPC (generyczna pamięć świata „co WIDZIAŁEM") | ⬜ | P1 | TECH-11 | **KROK 2 percepcji — OSOBNY GATE (2026-06-22). Architekt projektuje, kod po „zatwierdzam".** Generyczna pamięć świata: nie „widzę TERAZ" (TECH-11) lecz „WIDZIAŁEM" — JEDEN mechanizm dla jedzenia/wody/schronienia/NPC/zagrożeń: **(typ + pozycja + czas + pewność)**, przechowywana **na NPC** (kanon 1.1 — pamięć w „mózgu", NIE flaga na przedmiocie). **Zapominanie (MaxAge) należy TU**, nie do percepcji (w TECH-11 MaxAge ZOSTAJE 0 = „widzę bez zapominania"; ten sam loss-sygnał przepisze fakt do pamięci). **Drabina Disappointed = condition 1.4**: NPC dochodzi do zapamiętanego miejsca, pusto → jawny stan rozczarowania (pamięć nieaktualna). **MUSI czekać na czystą jedną-prawdę percepcji (TECH-11 krok 1)** — pamięć czyta z percepcji. **OTWARTE DECYZJE SZYMONA:** (a) generyczna-vs-tylko-jedzenie; (b) Disappointed jako condition na drabinie; (c) kolejność implementacji. |

---

## 👑 POZIOM 4 — Szacunek, Uznanie, Dominacja

| ID | Task | Status | Prio | Depends | Notes |
|---|---|---|---|---|---|
| L4-01 | Reputation system ("Value" per NPC) | ⬜ | P1 | L3-01 | Rises with good P2P, quests, rescues |
| L4-02 | DT_ReputationEvents (deltas per action) | ⬜ | P1 | L4-01 | Witness-count multiplier |
| L4-03 | Mastery/skill tags + recognition | ⬜ | P2 | L2-10 | "Master Lumberjack" tag; others seek them |
| L4-04 | Monopoly emergence (NPCs seek masters) | ⬜ | P2 | L4-03, L3-05 | "You're best, I'll pay double" deals |
| L4-05 | Preference matrix (likes/dislikes, luxury demand) | ⬜ | P2 | — | Favorite food/colors, luxury quests |
| L4-06 | Faction dynamics / Safe Zone geopolitics | ⬜ | P3 | L3-07 | Strong zone projects power on weaker zones |
| L4-07 | Combat skill progression | ⬜ | P2 | L2-10 | Stone-throwing → trained warriors |

---

## 🚀 POZIOM 5 — Samorealizacja, Eureki, Geopolityka

| ID | Task | Status | Prio | Depends | Notes |
|---|---|---|---|---|---|
| L5-01 | ⭐ Detective: ActionLog component | ⬜ | P1 | L3-01 | Per-NPC log [Time,Zone,Action,WitnessedNPCs] |
| L5-02 | FActionLogEntry struct (formalize in C++) | 🔨 | P1 | — | Ludos made partial ST_ActionLogEntry |
| L5-03 | Theft logging hook (TryWithdraw) | ✅ | — | L2-01 | LogInventory "THEFT:" already fires |
| L5-04 | Chief alibi cross-check + lie detection | ⬜ | P1 | L5-01, L3-01 | Cross-reference logs, detect contradictions |
| L5-05 | Penalty system (reputation loss, banishment) | ⬜ | P2 | L5-04, L4-01 | Liar "cracks" → consequences |
| L5-06 | Innovation points → Eurekas | ⬜ | P3 | — | All needs met → innovation |
| L5-07 | DT_Eurekas (fire, wheel, metal axe...) | ⬜ | P3 | L5-06 | Prereqs + unlocks new items |
| L5-08 | Campfire Sync (daily knowledge merge) | ⬜ | P2 | L3-07 | Evening: discoveries → village map |
| L5-09 | Feudalism/tithe (10% storehouse to strong zone) | ⬜ | P3 | L4-06 | Economic conquest |

---

## 🧩 SYSTEMY WSPIERAJĄCE

| ID | Task | Status | Prio | Depends | Notes |
|---|---|---|---|---|---|
| SUP-01 | NPC Inspector HUD — shell + integration | ✅ | P1 | — | DONE 2026-06-18: click→open POTWIERDZONE w PIE (fix: BP_NPC_Character CapsuleComponent Trace Response Visibility=Block; przedtem trace przelatywał → Cast FAIL). SetInspectedNPC→SetNPCRef + kaskada Body widoczna. (Zakładki SUP-01a/c/d/e/f = osobny zakres.) |
| SUP-01a | Compose WBP_Tab_* into inspector WidgetSwitcher | ⬜ | P1 | SUP-01 | Tabs exist as separate assets (Ludos); assemble into shell |
| SUP-01b | Body tab: Gap A (GetPartDisplayName) + Gap B (collapse rows) | ✅ | P1 | — | DONE 2026-06-16 via Monolith. (A) `Get Part Display Name` node added in PopulatePartRow, sourced from CastByteToEnum_0 (row EBodyPart), → title SetText.InText; stub `Get PartName` getter removed. (B) 7× `SetVisibility(Collapsed)` on PartRow_0..6 chained IsValid→Switch in RefreshBodyPanel. Both compiled clean + read-back verified, saved. Monolith CAN author BP nodes (earlier note was wrong). |
| SUP-01c | WBP_BodyPartButton — clickable silhouette regions | ⬜ | P1 | — | NEVER built (Ludos ran out of credits). 6 regions |
| SUP-01d | Stats tab: OCEAN + vitals + metabolism + behavior | 🔨 | P1 | L1-03 | Bars built; bind to real getters after rebuild |
| SUP-01e | Equipment tab: paper doll + compartment grids + D&D | 🔨 | P2 | L2-01 | Built by Ludos; bind GetCompartments after build |
| SUP-01f | Log tab: rows + theft banner | 🔨 | P2 | L5-02 | Built by Ludos; mock entries until L5-01 |
| SUP-02 | RTSCameraPawn (WASD/Q-E/scroll/MMB) | 🔨 | P2 | BUILD-02 | Confirm compiled; parent for BP_RTSCamera |
| SUP-03 | Senses → EQS perception wiring + **zmysł jako dane (per-NPC AIPerception config z DataAsset)** | ⬜ | P1 | L2-07 | VisionAcuity gates EQS sight — payoff of BodyCondition. **ROZSZERZENIE 2026-06-22 („zmysł jako dane"):** dziś AIPerception = **class-default** (SightRadius 1500 / LoseSightRadius 2000 / FOV 90, cone-hardkod 100) — JEDNAKOWY dla każdego NPC. Docelowo per-NPC config (zasięg/kąt/VisionAcuity/OCEAN-modyfikator) wstrzykiwany z **UDataAsset** zamiast class-default (kanon 1.1/1.5: byt = DataAsset). Po migracji percepcji do C++ (TECH-11) właścicielem jest `UNPCPerceptionComponent` → wstrzyknięcie zasięgu/kąta = **jeden punkt**. POZA zakresem TECH-11 krok 1. |
| SUP-04 | OnBodyChanged event broadcast (HUD refresh) | ⬜ | P2 | L2-07, SUP-01b | Replace 0.5s poll with event |
| SUP-05 | HUD-cleanup: wytnij martwe `Get DebugUI`→SetText z `THE ATMOSPHERE`(3×)/`Debug`(2×) w BP_DayNightCycle | ✅ | P3 | — | **DONE 2026-06-22 (Monolith, chirurgicznie):** THE ATMOSPHERE — bypass exec `Knot_0→CallFunction_27` + 9 węzłów (Pora/SunIntens/MoonIntens); Debug — body opróżniony (8 węzłów, w tym Print „nope"); zmienna `DebugUI` usunięta. **0 `Get DebugUI`, compile green, log PIE czysty** (0 spamu po wielu tickach THE ATMOSPHERE). Łańcuchy słońca/atmosfery/temp nietknięte. HUD nietknięty (`HUDReference=WBP_RTSHud_C` in_viewport, `CurrentTime="15:14"`). |

---

## 🔒 ZATWIERDZONE DECYZJE (Decision Log)
> Locked. Do NOT reverse without explicit re-approval (CLAUDE.md rule).

| Decision | Rationale |
|---|---|
| No Event Tick; Timers + BT/EQS only | 500+ NPC performance |
| Ownership by `int32 OwnerID`, never `AActor*` | IDs survive owner death; no dangling pointers |
| `int32` not `uint16` for item stacks | Blueprint pin compat; ~64KB cost negligible |
| Inventory: per-container compartments (Tarkov-style) | Backpack/pants/tunic each own storage |
| Container unequip requires empty (v1) | No orphaned items; DropContainer() later (L2-12) |
| Body: 26 parts, hierarchical | Full senses (eyes/ears/tongue/hands/fingers) |
| Body damage: cascade (parent caps child, min) | Wounded arm caps hand+fingers |
| Senses derived + cached, recompute on damage only | Never per-frame; BT reads cached float O(1) |
| **Keep BOTH body enums** | BP E_BodyPart(6)=click layer; C++ EBodyPart(26)=data layer |
| **Part names via GetPartName(EBodyPart)→Text helper** | UMETA DisplayName is editor-only, stripped in shipping |
| C++ = brain, Blueprint = body (visual) | Hybrid workflow; visuals via BlueprintImplementableEvent |
| Ask before any change affecting game mechanics | Enums/parts/contracts/traits = refactor risk |
| One canonical ROADMAP (this file), Maslow-sorted | No parallel task lists → no drift |

---

## 📌 CRITICAL PATH (why order matters)
```
BUILD-01 (rebuild) ──→ BUILD-02 (verify) ──┬─→ L2 systems (storehouse, medical, datatables)
   close the editor                         │
                                            └─→ L3-01 NPCRegistry ⭐
                                                   │
                          ┌────────────────────────┼────────────────────────┐
                          ↓                         ↓                        ↓
                  P2P Contracts (L3-05)      Reputation (L4-01)      ActionLog (L5-01)
                          │                         │                        │
                          ↓                         ↓                        ↓
                  Monopoly (L4-04)          Penalties (L5-05)        Detective (L5-04)
```
Two gates rule everything: **BUILD-01** (mechanical — close editor, rebuild) and
**L3-01 NPCRegistry** (architectural — the keystone for the entire social half of the game).
Nothing social ships until an `int32 ID` can resolve to a living NPC.
