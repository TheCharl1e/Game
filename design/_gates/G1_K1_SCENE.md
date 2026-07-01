# G1-K1 — Scena gotowa i zaludniona

**Data:** 2026-07-01 · **Target:** `E:\Game_58`, mapa `CaldrethMap` · **Faza:** SEKCJA 0 (AUDYT) — **STOP przed budową**
**Środowisko:** Game_58 (UE 5.8), MCPUnreal :8090. CaldrethMap **wczytany read-only** do audytu (był otwarty „Game"; zero zmian, zero zapisu — level czysty, brak modalu).

---

## WYNIK AUDYTU (sekcja 0)

### ✅ Co już stoi
| Element | Stan | Szczegóły |
|---|---|---|
| BP_NPC_Character | ✅ | Ma `MaslowBiologicalComponent` + `NPCIdentityComponent` (+ Inventory, ContractTrader, BodyCondition, AIPerceptionStimuliSource). Rejestruje się w NPCRegistry (PIE-proven wcześniej: NPCId 1/2). |
| BP_DayNightCycle | ✅ **już postawiony** | 1 instancja na CaldrethMap @ (-650,-630,-170). RESOLVED#1 prawdopodobnie JUŻ spełniony — **do potwierdzenia:** czy to wariant `WORLD/BP_DayNightCycle` (żywy), nie martwy dup `AI_NPC/BP_DayNightCycle`, oraz czy realnie tyka (nie fallback). |
| NavMeshBoundsVolume + RecastNavMesh | ✅ obecne | NavBox: środek (0,0), ekstent (4500,4500). NavSystem obecny. |
| PlayerStart | ✅ | @ origin (0,0,92). |
| Strefy (dane) | ✅ | 18× `CaldrethZone` (GetZoneAtLocation) + 5× POIMarker. |
| Forage/drink test | ✅ | 1× BP_NPC_Character, BP_Food, BP_WaterSource — wszystko przy origin na podłodze. |

### 🔴 BLOCKER STRUKTURALNY (rzeczywistość przeczy DoD — flaguję przed budową)
**Podłoga to jeden `Floor` 8000×8000 uu @ origin (z=-170). NavMesh ±4500 pokrywa tylko tę podłogę.
18 stref jest rozrzuconych po ±370 000 uu, ale to DANE bez terenu pod nimi.**

Twarde liczby (live, CaldrethMap):
- `zones_in_navbox = 0 / 18`
- `spawnable_zones_in_navbox = 0 / 8`
- Przykłady stref bSpawnable: Beach @ (-114928,-39119) i (-365520,318881); Grassland @ (74683,-300836); Oasis @ (-297471,151539); Savanna @ (-291313,146098). **Wszystkie setki tysięcy uu od origin, bez podłogi i bez navmeshu.**
- Jedyny grunt: `Floor` ext (4000,4000) @ origin. `SM_SkySphere` = tylko niebo.

**Konsekwencja:** DoD „50 NPC spawnuje na NavMeshu w strefach bSpawnable" jest **NIEWYKONALNE na obecnej mapie** — strefy spawnowalne nie mają ani terenu, ani navmeshu. To nie jest parametr do „assume-log-continue"; to decyzja projektowa (patrz OPEN #1).

### ⬜ Czego brak (do zbudowania — twarde bramki)
- **BP_NPCSpawner** — NIE istnieje (ani asset, ani instancja; są tylko FoodSpawner + MOBA spawnery).
- **Spectator god-view pawn** — NIE istnieje (zero kamer/spectatorów w levelu).
- **NavMesh pod strefami** — brak (patrz blocker).

### Strefy — flagi (źródło: `Tools/MapGen/zone_defs.json`; live rowy DT_ZoneDefs zgodne, 12 typów)
- **bSpawnable = true:** Beach, Savanna, Grassland, SlopeForest, Oasis
- **bHabitable = true:** Savanna, Grassland, SlopeForest, Oasis
- **nie/nie:** Ocean, Desert, Mountain, AshSlope, Caldera, River, Lava
- ⚠️ **Rozjazd z RESOLVED:** gate mówi „NavMesh pokrywa bHabitable (…wybrzeże)", ale **Beach ma bHabitable=false** (spawnable-only). Skoro spawner używa bSpawnable, NavMesh musi pokrywać **bSpawnable** (nadzbiór z Beach), nie tylko bHabitable — inaczej NPC na Beach ląduje poza nav.

---

## RESOLVED (z gate'u — zablokowane)
- BP_DayNightCycle na CaldrethMap (JEDEN zegar) — **audyt: już stoi, do weryfikacji wariantu/tykania**.
- NavMesh pokrywający strefy — **wymaga rozwiązania blockera** (dziś pokrywa tylko origin-floor).
- Nowy BP_NPCSpawner — spawn na NavMeshu, tylko bSpawnable, count=param — **do utworzenia**.
- NPC = profil człowiek (L0+L1). Fauna = K4, nie tu.
- Gracz = god-view spectator — **do utworzenia**.
- WBP_NPC_Inspector (SetInspectedNPC) — do potwierdzenia z zespawnowanymi (klik→staty).

## DEFAULTS (assume-log-continue — założenia przyjęte, raportowane)
- SpawnCount start = 50 (param, do podbicia 200/500).
- Rozkład równomierny w bSpawnable, offset od Lava/Caldera.
- Tick pełny dla wszystkich (LOD → K4).
- Spawner z odstępem (bez nakładania kapsul).

## REDEFINICJA K1 — decyzja dyrektora 2026-07-01: **OPCJA D**
**Spawn przy origin** na istniejącej podłodze `Floor` 8000×8000 uu; strefa liczona przez `GetZoneAtLocation`
per pozycja NPC na podłodze. **Skutki:** blocker zdjęty; **NavMesh NIE wymaga rozbudowy** (±4500 już
pokrywa podłogę); spawner rozrzuca 50 NPC w obrębie podłogi (offset od krawędzi ±4000). Strefy-dane
pozostają rozrzucone (świadomie, ich teren = poza K1). AmbientTemp/strefa = z zapytania lokacji origin.

## OPEN (decyzja dyrektora — NIE samo-rozstrzygam)
1. ✅ **RESOLVED (D)** — blocker teren-vs-strefy: K1 spawnuje przy origin (patrz wyżej).
2. Docelowy count DoD skali: potwierdź 50/200/500. *(zakładam start 50 — DEFAULTS.)*
3. Zegar: zwolnić z 2.4 min/doba już teraz, czy zostawić szybki do testów? *(zakładam SZYBKI — DoD chce trendu fizjologii przez „kilka dób" w minutach PIE.)*
4. ~~NavMesh target~~ — **bezprzedmiotowe po D** (spawn na podłodze, nav już pokrywa). Rozjazd Beach/bHabitable = nieistotny dla K1.

## PLAN BUDOWY (po „tak" na bramki) — DUŻO REUSE, 1 nowy BP
Read-only prep potwierdził: zegar postawiony = `WORLD/BP_DayNightCycle` (żywy wariant ✅); obserwator
`BP_RTSCamera` + `GM_RTSGameMode`/`PC_RTSGameMode` **istnieją** (reuse); `WBP_NPC_Inspector` istnieje (reuse).
**Jedyny nowy asset: `BP_NPCSpawner`.**

1. **[NOWY BP] BP_NPCSpawner** (Actor): param `SpawnCount=50`, `SpawnRadius≈3500` (w obrębie podłogi ±4000, offset od krawędzi), spawn `BP_NPC_Character` na NavMeshu (`GetRandomReachablePointInRadius` wokół origin), odstęp kapsuł. 1 instancja @ origin.
2. **[REUSE] Obserwator** — ustawić CaldrethMap GameMode override = `GM_RTSGameMode` (god-view `BP_RTSCamera`, zero postaci gracza). Zweryfikować, że Inspector `WBP_NPC_Inspector` działa z zespawnowanymi (SetInspectedNPC→klik→staty).
3. **[VERIFY] Zegar** — potwierdzić tykanie `WORLD/BP_DayNightCycle` w PIE (nie fallback); zostaje SZYBKI (2.4 min/doba) do testów, chyba że zdecydujesz inaczej (OPEN#3).
4. **[PIE-verify DoD]** 50 NPC spawn+register (NPCRegistry count=50) na podłodze/navmeshu, zegar tyka, fizjologia (Hydration↓/Glucose↓/HoursAwake↑→mikrosen/omdlenie, AmbientTemp wg strefy origin) trend przez kilka dób, klik-Inspector→żywe staty, frametime@50 baseline. Twarde liczby, post-PIE UE_LOG.

**Twarde bramki tego planu (czekają na „tak"):** utworzenie `BP_NPCSpawner` (BP edit) · postawienie instancji spawnera @ origin (actor placement) · ustawienie GameMode override na CaldrethMap (level/world-settings edit) · PIE. Zapis mapy = Twój Ctrl+S (nie ścieżką silnikową).

## TWARDE BRAMKI — czekają na „tak"
Nic nie postawione/utworzone. Przed budową wymagam zgody na: postawienie/rozszerzenie NavMeshBounds, utworzenie `BP_NPCSpawner`, utworzenie spectator pawn, (ew.) weryfikację/przestawienie zegara, PIE, push. `set_pin_value` na pinach object/enum — zainstruuję, zrobisz ręcznie.

## WYNIK DoD — PIE-verify 2026-07-01 (CaldrethMap, Game_58 5.8, twarde liczby z live+log)
Build `ANPCSpawner` (C++, `World/NPCSpawner.{h,cpp}`) → `Game_58Editor` **Result: Succeeded** (UE 5.8, UHT 160 plików). Spawner postawiony @ origin (NPCClass=BP_NPC_Character, count=50, radius=3500), GameMode override=`GM_RTSGameMode`. **Scena NIEzapisana** (patrz długi — Content gitignored, backup=ręczny Ctrl+S).

| DoD | Wynik | Dowód |
|---|---|---|
| 50 NPC spawn na navmeshu, nie w Lava | ✅ | `[Spawner] Spawned 50/50 NPCs within 3500 uu of (0,0,0) (nav misses=0)` — 0 pudeł nav, rozrzut po podłodze origin |
| Rejestracja w NPCRegistry | ✅ | 51 NPC (50+1 test), NPCId **1..51**, `registered_ids_count=51`, ID nie-recyklowane |
| Zegar tyka (nie fallback) | ✅ | CurrentTime 16.08→7.29 (przeszło dobę), SunIntensity 81.2 live, DayOfYear 100 |
| AmbientTemp wg strefy+doby | ✅ | 34.51°C na NPC @ origin (strefowe, NIE fallback 20) |
| Fizjologia trend przez kilka dób | ✅ | avg: Hydration 94→**0**, Glucose 970→**420**, HoursAwake 5→**13.3** |
| Śmierć → clean cleanup | ✅ | NPC 51→46→**29**; log `[Registry] Unregistered ID X (live=N). ID retired.` (live 36→29 malejąco); min_HP=0 (odwodnienie→HP→śmierć→EndPlay→unregister) |
| Mikrosen/omdlenie | ⚠️ NIE złapane | odwodnienie (1 woda/50 NPC pod slomo8) zabiło przed HoursAwake=16; mechanizm dowiedziony osobno w L1-07/`task2_sen_done` |
| Klik-Inspector → żywe staty | ⚠️ nie zweryfikowane | UI-klik; `WBP_NPC_Inspector`+`PC_RTSGameMode` istnieją (reuse). Weryfikacja ręczna |
| Frametime@50 baseline | ⚠️ nie złapane | sample na ciężkiej klatce skryptu = 2666 ms (hitch, niewiarygodny). Potrzeba `stat unit` na czystej klatce |

**Werdykt rdzenia sceny:** ✅ scena ŻYJE — spawn/rejestracja/zegar/temp-strefowa/fizjologia-trend/śmierć-cleanup dowiedzione twardymi liczbami i logiem. Luki = długi (niżej), część wynika ze scarcity/toolingu, nie z infry sceny.

## ZNANE DŁUGI (K1)
- **d1 — recover-at-scale:** 1 `BP_Food` + 1 `BP_WaterSource` na 50 NPC → drenaż dominuje, „pije/je→recover" na skali nie pokazane (pętla recover dowiedziona dla 1 NPC: forage/drink LIVE). Densyfikacja afordancji = G1a.
- **d2 — mikrosen/omdlenie:** odwodnienie zabija przed HoursAwake=16 pod slomo8; do złapania trzeba zamrozić Hydration na próbce albo dać wodę pod dostatkiem.
- **d3 — frametime baseline:** złapać `stat unit`/`stat fps` na czystej klatce @50 (i @200/500) — nie sample-na-skrypcie.
- **d4 — Inspector klik:** weryfikacja ręczna (UI) z zespawnowanymi.
- **d5 — stan sceny NIE w gitcie:** Content gitignored → spawner@origin + GameMode override żyją tylko w `.umap`; utrwalenie = ręczny Ctrl+S dyrektora (backup ręczny). Obecnie **niezapisane**.
- **d6 — HoursAwake rozrzut:** wszystkie NPC startują HoursAwake identycznie → zsynchronizowane omdlenia; dodać rozrzut startowy (→G1a).
- **d7 — jednostrefowość origin:** AmbientTemp jednakowe dla wszystkich (origin=jedna strefa); różnicowanie strefowe = G1b (teren pod strefami).

---

## Batch-report
- Audyt read-only wykonany (Game_58/CaldrethMap; zero zmian, zero zapisu). CaldrethMap pozostaje otwarty (był „Game"; mogę przywrócić).
- **1 blocker strukturalny** (teren vs strefy) + **4 OPEN** + **2 elementy do utworzenia** (spawner, spectator) + **1 rozjazd flag** (Beach bHabitable vs NavMesh-target).
- Pozytyw: BP_NPC (Maslow+Identity), zegar, PlayerStart, NavMesh-nad-podłogą, forage-loop origin — **stoją**.
- **STOP.** Czekam na rozstrzygnięcie OPEN #1-4 i „tak" na twarde bramki, zanim cokolwiek postawię.
