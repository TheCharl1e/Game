# EXECUTION PLAYBOOK v2 — zadanie po zadaniu (Stan_Pierwotny)

> Każde zadanie = gotowa komenda do wklejenia w Claude Code.
> [MCP] tag mówi którego narzędzia użyć. ⛔ = DESIGN GATE (Claude Code pokazuje plan, NIE buduje — wracasz do architekta).
> Idź z góry na dół. Nie zaczynaj zadania, którego zależności nie są DONE.

## 🔌 REGUŁY STAŁE
- **monolith** = autorowanie/spinanie węzłów Blueprint (add_node, connect_pins). POTWIERDZONE działa.
- **remiphilippe** = build / test / C++ / odczyt grafów / tworzenie assetów (enum/struct/DataTable/widget).
- Zapis assetu Blueprint: `editor.save_packages` (nie `blueprint.save_asset` — bywa zawodny).
- **git commit przed każdą fazą.** Po każdym zadaniu: update ROADMAP STATUS + wpis do CHANGELOG.
- ⛔ przy zadaniach mechanicznych: Claude Code pokazuje plan, ty wracasz do architekta (czat) po akceptację.

═══════════════════════════════════════════════════════════
## FAZA A — DOMKNIĘCIE HUD (jesteś tu)
═══════════════════════════════════════════════════════════

### A1 · Dedup komponentu + test kaskady na żywo · [monolith]
```
Przeskanuj WSZYSTKIE funkcje BP_NPC_Character pod kątem referencji
do komponentu BodyCondition (nie BodyConditionComp). Wypisz każde
miejsce albo potwierdź zero. Jeśli zero — usuń BodyCondition,
skompiluj. Compile czysty → flip SUP-01b → DONE w ROADMAP + raport.
```
VERIFY: odpal PIE, kliknij NPC, Body → Left Arm. Oczekiwane: ramię 40%,
dłoń + 5 palców ścięte do 40% (kaskada), nazwy części, spadek Hand precision.
STATUS 2026-06-16: ✅ build-część DONE (dedup BodyCondition, SUP-01b→DONE, raport).
Czeka na ręczną weryfikację PIE przed A2.

### A2 · Sprzątnięcie testowego ApplyDamage (po weryfikacji) · [monolith]
```
Gdy potwierdzę że kaskada działa w HUD — usuń blok testowy ApplyDamage
z BeginPlay BP_NPC_Character (ten w czerwonym komentarzu TEST ONLY).
Compile + save. Wpis do CHANGELOG.
```
VERIFY: BeginPlay czysty, brak testowych obrażeń przy starcie.

═══════════════════════════════════════════════════════════
## FAZA B — SILNIK SNU (L1-06/07) · szybka wygrana, ożywia HUD
═══════════════════════════════════════════════════════════
> Getter `GetHoursAwakePercent()` jest, ale HoursAwake nigdy nie rośnie → pasek Awake martwy.

### B1 ⛔ · Projekt silnika snu · [DESIGN GATE]
```
STOP — pokaż plan, nie buduj. Silnik snu (L1-06/07). Zaproponuj:
- timer inkrementujący HoursAwake (jaki interwał, jak liczony do godzin gry)
- progi debuffów: mgła umysłowa -30% (przy ilu h), mikrosny (przy ilu h),
  passout/ragdoll (≥24h = MaxHoursBeforeCollapse)
- reset HoursAwake przy śnie + buff "Rested" +20%
- co w C++ (matematyka/timer) a co w Blueprint (ragdoll, anim) via
  BlueprintImplementableEvent
Pokaż plan. Czekam na akceptację architekta.
```

### B2 · Implementacja silnika snu (po akceptacji) · [remiphilippe]
```
Zaimplementuj zatwierdzony silnik snu w MaslowBiologicalComponent:
timer ++HoursAwake, progi debuffów, reset+Rested przy śnie.
Wizualne efekty (ragdoll passout) jako BlueprintImplementableEvent.
Wartości z MECHANICS.md [approved]. Build. Update MECHANICS
(flip [GETTER READY, ENGINE TODO] → [LOCKED]) + CHANGELOG + raport.
```
VERIFY: pasek Awake w HUD rośnie w czasie; po progu widać debuff.

═══════════════════════════════════════════════════════════
## FAZA C — DŁUG TECHNICZNY · czysty grunt przed keystonem
═══════════════════════════════════════════════════════════

### C1 · Literówki kluczy Blackboard (P1 — cicho psuje BTService) · [remiphilippe]
```
Odczytaj klucze BB_NPC i klucze pisane przez BTService_MaslowBlackboardSync
(MaslowPriority, HydrationPercent, GlucosePercent, HPPercent, IsInPanic,
IsStarving). Wypisz rozbieżności. Napraw literówki żeby się zgadzały
dokładnie. Build. Raport.
```
VERIFY: BTService faktycznie zapisuje do BB (sprawdź w PIE wartości kluczy).

### C2 · Fix Up Redirectors + duplikaty · [remiphilippe]
```
Fix Up Redirectors w całym Content/. Potem wypisz wszystkie assety
z "_Experyment" w nazwie — pokaż listę PRZED usunięciem, potwierdzę
które usunąć. Nic nie kasuj bez mojej zgody.
```

### C3 · BB keys + service na BT root (L1-05) · [monolith]
```
W BT_NPC: dodaj 6 kluczy Blackboard jeśli brakują (MaslowPriority int,
HydrationPercent/GlucosePercent/HPPercent float, IsInPanic/IsStarving bool),
i dodaj BTService_MaslowBlackboardSync na root drzewa. Compile + save.
```
VERIFY: w PIE NPC reaguje na potrzeby (BT czyta priorytet z BB).

═══════════════════════════════════════════════════════════
## FAZA D — ⭐ KEYSTONE: NPCRegistry · projektujemy RAZEM
═══════════════════════════════════════════════════════════

### D1 ⛔ · Projekt NPCRegistry · [DESIGN GATE — architekt]
```
STOP — pokaż plan, nie buduj. NPCRegistry to fundament warstwy
społecznej (P2P, reputacja, detektyw). Zaproponuj:
- UWorldSubsystem z TMap<int32, TWeakObjectPtr<ANPC>>
- RegisterNPC/UnregisterNPC na spawn/death (fail-safe na śmierć)
- GetNPCById(int32) → NPC* z IsValid
- szacunek pamięci przy 500 NPC
Pokaż plan i szkic API. Czekam na architekta zanim cokolwiek napiszesz.
```

### D2 · Implementacja NPCRegistry (po akceptacji) · [remiphilippe]
```
Zaimplementuj zatwierdzony NPCRegistry (UWorldSubsystem). Pełna
separacja .h/.cpp, UFUNCTION BlueprintCallable, hojny UE_LOG.
NPC rejestruje się w BeginPlay, wyrejestrowuje w EndPlay.
Build. PROJECT_STATE (nowy komponent) + CHANGELOG + raport.
```
VERIFY: spawn kilku NPC → GetNPCById zwraca właściwych; po despawn → null bezpiecznie.

═══════════════════════════════════════════════════════════
## FAZA E — L3 WARSTWA SPOŁECZNA · serce "Matrixa"
═══════════════════════════════════════════════════════════

### E1 ⛔ · OCEAN — dane osobowości · [DESIGN GATE]
```
STOP — pokaż plan. OCEAN (Big Five) jako UDataAsset/DataTable na NPC:
Openness/Conscientiousness/Extraversion/Agreeableness/Neuroticism.
Zaproponuj zakres (0-1?) i JAK każda cecha filtruje Behavior Tree
(np. niska Ugodowość → częściej łamie prawo własności). Plan, nie kod.
```

### E2 · OCEAN — implementacja + bind do HUD · [remiphilippe + monolith]
```
[remiphilippe] Zaimplementuj zatwierdzoną strukturę OCEAN na NPC.
[monolith] Podłącz paski OCEAN w zakładce Stats (WBP_NPC_Inspector)
do realnych wartości (zastąp placeholdery). Build + save. CHANGELOG.
```
VERIFY: paski OCEAN w HUD pokazują realne cechy NPC.

### E3 ⛔ · Cykl dobowy jako tura · [DESIGN GATE]
```
STOP — pokaż plan. Dzień = tura. Rano NPC ocenia Maslow → generuje
kolejkę akcji na dzień (rejestr planów). Zaproponuj strukturę
FDailyPlan + jak kolejka wpina się w Behavior Tree. Plan, nie kod.
```

### E4 ⛔ · Kontrakty P2P · [DESIGN GATE]
```
STOP — pokaż plan. System kontraktów P2P: FContract (kto, co, za co,
status), wirtualna pula zleceń, NPC wystawiają i przyjmują barter.
Wykorzystaj NPCRegistry (int32 → NPC). Zaproponuj strukturę + jak
NPC oferuje/przyjmuje/realizuje. Plan, nie kod.
```

### E5 · Kontrakty P2P — BT taski (po akceptacji) · [remiphilippe + monolith]
```
Zaimplementuj zatwierdzony system kontraktów: struktury C++,
BT taski (offer/accept/fulfill) via monolith. Build + raport.
```
VERIFY: drwal i kowal wymieniają drewno↔siekiera bez globalnego skryptu.

### E6 · Safe Zones via EQS (L3-07) · [remiphilippe]
```
Wykorzystaj istniejące EQC_SearchZone/EQS_ZoneSearch: NPC odnajdują
strefy bezpieczeństwa i przypisują się do nich (grupy ~10). Dodaj
CurrentZoneID na NPC. Build. Raport.
```
VERIFY: NPC grupują się w strefach; CurrentZoneID ustawiony.

═══════════════════════════════════════════════════════════
## FAZA F — L2 UZUPEŁNIENIA · ekonomia osady
═══════════════════════════════════════════════════════════

### F1 · DataTables · [remiphilippe]
```
Utwórz/uzupełnij: DT_ItemDefinitions (FItemDefinition), DT_FoodStats
(Nutrition, PoisonChance), wepnij DT_ActionCosts do Maslow. Wartości
z MECHANICS.md [approved]; dla brakujących zaproponuj i zapytaj.
Build. Raport.
```

### F2 ⛔ · Public Storehouse · [DESIGN GATE]
```
STOP — pokaż plan. Aktor Storehouse z InventoryComponent OwnerID=-1
(PUBLIC_OWNER_ID), jeden na Safe Zone. Jak NPC znoszą nadwyżki
(TransferTo) i konsumują publiczne po prywatnym. Plan, nie kod.
```

### F3 · Storehouse — implementacja (po akceptacji) · [remiphilippe + monolith]
```
Zaimplementuj zatwierdzony Storehouse + BT behavior "znoś nadwyżki"
i "jedz prywatne potem publiczne". Build. Raport.
```
VERIFY: syty NPC znosi do spiżarni; głodny je swoje, potem publiczne.

### F4 · System medyczny (L2-08/09) · [remiphilippe]
```
HealPart przez przedmioty (bandaż/szyna) — RequiredTreatmentItem
konsumuje item. Choroba z surowego mięsa: DT_FoodStats PoisonChance
→ wound/sickness. Wartości z MECHANICS [approved]. Build. Raport.
```
VERIFY: opatrzenie leczy część ciała; surowe mięso czasem truje.

═══════════════════════════════════════════════════════════
## FAZA G — L4 SZACUNEK · ego, ekonomia, polityka
═══════════════════════════════════════════════════════════

### G1 ⛔ · Reputacja · [DESIGN GATE]
```
STOP — pokaż plan. Reputation "Value" per NPC (NPCRegistry).
Rośnie: udane P2P (+1), ratunek (+5). DT_ReputationEvents z deltami,
mnożnik za liczbę świadków. Plan + jak inni NPC reagują na Value.
```

### G2 · Mistrzostwo i monopole (L4-03/04) · [remiphilippe]
```
Tag mistrzostwa po N turach przy zadaniu (MECHANICS: 50). Inni NPC
rozpoznają tag i szukają mistrza ("zapłacę podwójnie"). Build. Raport.
```
VERIFY: drwal po wielu turach dostaje tag; inni go szukają.

═══════════════════════════════════════════════════════════
## FAZA H — L5 SAMOREALIZACJA · detektyw, eureki
═══════════════════════════════════════════════════════════

### H1 ⛔ · ActionLog + detektyw · [DESIGN GATE]
```
STOP — pokaż plan. UActionLogComponent: log [Time, Zone, Action,
WitnessedNPCs<int32>]. Wódz krzyżowo sprawdza alibi → wykrywa
sprzeczność → kłamca "pęka". Kary: reputacja -10, banicja ≤-20.
Wykorzystaj NPCRegistry. Plan, nie kod.
```

### H2 · ActionLog — implementacja (po akceptacji) · [remiphilippe + monolith]
```
Zaimplementuj zatwierdzony ActionLog + alibi check + kary.
Podłącz do zakładki Log w HUD (zastąp mocki). Build. Raport.
```
VERIFY: kradzież → zgłoszenie → Wódz sprawdza logi → kłamca ukarany;
zakładka Log pokazuje realne wpisy.

### H3 ⛔ · Eureki + feudalizm · [DESIGN GATE]
```
STOP — pokaż plan. Punkty innowacji (gdy potrzeby zaspokojone) →
DT_Eurekas (ogień, koło, lepsza broń) z prereqami. Feudalizm:
silna strefa bierze 10% spiżarni słabszej. Plan, nie kod.
```

═══════════════════════════════════════════════════════════
## 📌 ŚCIEŻKA KRYTYCZNA (skrót)
═══════════════════════════════════════════════════════════
HUD (A) → Sen (B) → Czysty grunt (C) → ⭐ NPCRegistry (D)
   → [odblokowane] L3 społeczność (E) ┬→ L4 reputacja (G)
                                       ├→ L5 detektyw (H)
                                       └→ L2 ekonomia (F)

NPCRegistry (D) to brama. Wszystko w E/G/H zależy od int32→NPC.
F (ekonomia) można robić równolegle do E po D.
