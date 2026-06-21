# Stan_Pierwotny — JAK TO DZIAŁA (żywy dokument projektowy)

> Cel: baza do burzy mózgów. Każda mechanika: **co robi · jak (wartości) · gdzie się wpina · haczyki do rozbudowy**.
> To NIE jest lista zadań (to ROADMAP). To opis działania, żeby wymyślać ulepszenia.
> Status: ✅ zbudowane · 🔨 częściowe · ⬜ projekt/TODO · 🔴 luka

═══════════════════════════════════════════════════════════
# CZĘŚĆ I — NPC (piramida Maslowa)
═══════════════════════════════════════════════════════════
> Zasada naczelna: NPC nie działa dla społeczeństwa, póki nie zaspokoi własnych potrzeb.
> Każdy niższy poziom NADPISUJE wyższy. Zero LLM per NPC — twarda matematyka, timery, BT, EQS.

## 🩸 L0 — Stres (Fight or Flight) · 🔨
**Co robi:** panika nadpisuje WSZYSTKO. NPC rzuca zadania, ucieka/walczy.
**Jak:** w kodzie odpala się na `CurrentHP ≤ 25`. PanicDuration 8s, FleeSpeed ×1.5, drop loaded container.
**🔴 Luka:** projektowo panika miała reagować na ZAGROŻENIE przez wzrok (drapieżnik w polu widzenia), nie tylko na niskie HP. `GetVisionAcuity()` istnieje, ale wiring threat→panic nie zbudowany (L0-04).
**🔴 Dług techniczny (kolizje vs AI-sight):** dla klikalności HUD kapsuła BP_NPC_Character blokuje teraz kanał **Visibility** (fix wariant A — patrz CODE_REGISTRY). GDY budujemy AI line-of-sight (L0-04 / SUP-03 senses→EQS): jeśli percepcja użyje kanału Visibility, NPC zaczną się wzajemnie zasłaniać/wykrywać przez ten sam kanał co klik gracza → fałszywe okluzje/detekcje. **Docelowo (wariant D): dedykowany custom trace channel „Clickable"** dla kliku, a Visibility/percepcja osobno. Zrobić RAZEM z wiringiem AI-sight.
**Gdzie się wpina:** czyta HP z Maslow; przerywa kolejkę dobową (L3); psuje kontrakty P2P (NPC wraca z pustymi rękami).
**Haczyki do rozbudowy:**
- Panika stopniowana: nerwowy NPC (OCEAN Neuroticism) panikuje wcześniej/dłużej.
- Pamięć traumy: miejsce ataku oznaczane jako "niebezpieczne" w mapie wiedzy → NPC go unika.
- Reakcja grupowa: panika jednego NPC zaraża sąsiadów (stado ucieka razem).

## 🫀 L1 — Fizjologia

### Głód (kaskada kataboliczna) · ✅
**Co robi:** organizm spala paliwo w ścisłej kolejności; głód w skrajności → kradzież/kanibalizm.
**Jak:** `EHungerPhase`: Glucose → Glycogen → FatBurn (0.5× tempa) → Autophagy (MaxHP −0.25×burn, trwale!) → Death. MaxGlucose/Glycogen = 1000, MaxBodyFat = 5000.
**Gdzie się wpina:** Autophagy trwale obniża MaxHP → słabszy NPC → łatwiej w panikę L0. Głód napędza konsumpcję (najpierw prywatne, potem publiczna spiżarnia L2).
**Haczyki do rozbudowy:**
- Skutki długoterminowe: NPC po głodzie ma trwały debuff "wychudzony" aż odbuduje mięśnie.
- Kanibalizm jako tabu społeczne: złamanie obniża reputację L4, budzi strach u innych.
- Dieta/preferencje: ulubione jedzenie daje bonus do morale (most do L4 preference matrix).

### Pragnienie · ✅
**Co robi:** paliwo dla staminy; odwodnienie blokuje regenerację i szybko zabija.
**Jak:** Hydration max 100. Spadek skalowany aktywnością: Rest ×1 / Work ×3 / Combat ×5 (data-driven z DT). Odwodnienie −10 HP/tick.
**Gdzie się wpina:** wprost do mapy świata — woda jest tylko przy rzekach/wybrzeżu (Caldreth). Susza (pogoda) wysusza wadi → NPC musi migrować do wody stałej. To jest most NPC↔świat.
**Haczyki do rozbudowy:**
- Magazynowanie wody (bukłaki) jako item L2 → NPC planuje wyprawy w głąb lądu.
- Skażona woda → choroba (most do systemu medycznego).

### Temperatura · 🔨
**Co robi:** zimno zwielokrotnia spalanie tłuszczu; ciepło z ognia/odzieży chroni.
**Jak:** powiązane z cyklem dnia/nocy. Zimno ×2 spalanie tłuszczu @ <10°C. Łagodzone ogniem (Eureka) i ubraniem (equip).
**Gdzie się wpina:** most do świata — stoki popiołu (Caldreth) trzymają ciepło wulkanu w nocy → NPC w kryzysie zimna tam ucieka (blisko zagrożenia). Nasłonecznienie stoków (pogoda/słońce) modyfikuje lokalną temperaturę.
**Haczyki do rozbudowy:**
- Mikroklimat per strefa: las cienisty chłodniejszy, sawanna gorąca.
- Hipotermia/przegrzanie jako stany medyczne L2.
- Sezonowość: zima (słońce L) wymusza gromadzenie opału.

### Sen (ukryta adenozyna) · 🔴 GETTER READY, ENGINE TODO
**Co robi:** brak snu → mgła umysłowa → mikrosny → omdlenie.
**Jak (zaprojektowane):** HoursAwake rośnie, MaxHoursBeforeCollapse = 24h. Mgła −30% wydajności @16h, mikrosny @20h, passout (ragdoll) @24h. Dobry sen → buff "Rested" +20% praca/Eureka.
**🔴 Luka:** gettery istnieją, ale HoursAwake NIGDY nie rośnie — brak timera, brak silnika debuffów. Pasek "Awake" w HUD martwy.
**Gdzie się wpina:** mgła obniża wszystkie staty pracy (L2 crafting), Rested boostuje odkrywanie Eurek (L5). Cykl dnia/nocy (świat) wyznacza, kiedy NPC powinien spać.
**Haczyki do rozbudowy:**
- Bezpieczne spanie tylko w Safe Zone (L3) → sen wiąże NPC z grupą.
- Warty nocne: ktoś nie śpi, by pilnować (rola społeczna L4).
- Koszmary po traumie (most do pamięci L0).

## 🛡️ L2 — Bezpieczeństwo

### Ekwipunek (per-container) · ✅
**Co robi:** rozróżnia własność prywatną (kieszeń NPC) od publicznej (spiżarnia wioski).
**Jak:** każdy kontener ma OwnerID (int32). PUBLIC_OWNER_ID = -1. `TryWithdraw` z nieautoryzowanego → log "THEFT:". Unequip wymaga pustego kontenera.
**Gdzie się wpina:** kradzież → zgłoszenie do Wodza → detektyw L5. Głód L1 decyduje, co NPC konsumuje (prywatne→publiczne). Znoszenie nadwyżek do spiżarni = zachowanie społeczne L3.
**Haczyki do rozbudowy:**
- Zamki/skrytki: prywatna skrzynia, którą trzeba siłą otworzyć (ślad dla detektywa).
- Waluta/barter: itemy jako środek wymiany w kontraktach P2P.
- Dziedziczenie: po śmierci NPC ekwipunek → spadek (kto przejmuje? konflikt L4).

### Ciało (26 części, kaskada) · ✅
**Co robi:** uszkodzenie części ciała kaskadowo obniża części zależne i zmysły.
**Jak:** 26 części, zdrowie 0.0–1.0. Efektywne zdrowie = min(własne, łańcuch rodzica). Wagi zmysłów: wzrok (oczy 0.5/0.5), słuch (uszy 0.5/0.5), mowa (język 1.0), precyzja dłoni (Dłoń 0.4/Kciuk 0.3/Wskazujący 0.1/Środkowy 0.08/Serdeczny 0.07/Mały 0.05), mobilność (noga 0.35/stopa 0.15 ×2). Urazy: Wound/Fracture/Sprain/Burn/Sever. HealPart +0.3.
**Gdzie się wpina:** wzrok → wykrywanie zagrożeń L0, znajdowanie jedzenia L1, świadek w śledztwie L5. Precyzja dłoni → jakość craftingu L2/L4. Mobilność → ucieczka L0, prędkość ruchu. Mowa → sukces kontraktów P2P L3, zeznania L5.
**Haczyki do rozbudowy:**
- Protezy/kalectwo trwałe: utrata ręki = zmiana roli zawodowej.
- Blizny jako historia: widoczne urazy wpływają na reputację/strach.
- Specjalizacja vs uraz: mistrz drwal z połamaną ręką traci monopol L4.

### System medyczny · ⬜
**Co robi:** leczenie ran, choroby z surowego mięsa.
**Jak:** HealPart przez itemy (bandaż/szyna). Raw meat PoisonChance 0.35 → wound/sickness.
**Gdzie się wpina:** choroba obniża staty (L1/L2), tworzy zapotrzebowanie na zioła → zlecenie P2P ("mój syn chory, potrzebuję ziół" L3).
**Haczyki do rozbudowy:**
- Epidemie: choroba zaraźliwa w Safe Zone → kwarantanna jako decyzja społeczna.
- Zielarz/medyk jako specjalizacja L4.
- Zioła rosną tylko w lasach zboczowych (Caldreth) → geografia napędza medycynę.

═══════════════════════════════════════════════════════════
## ⭐ KEYSTONE — NPCRegistry · ⬜ (projektujemy razem)
═══════════════════════════════════════════════════════════
**Co robi:** zamienia int32 ID na żywego NPC. Bez tego cała warstwa społeczna jest niemożliwa.
**Jak (projekt):** UWorldSubsystem, TMap<int32, TWeakObjectPtr<NPC>>. Rejestracja na spawn, wyrejestrowanie na death (TWeakObjectPtr samo czyści martwe). GetNPCById(int32). <25KB przy 500 NPC.
**Gdzie się wpina:** WSZĘDZIE w L3/L4/L5. OwnerID w ekwipunku (L2) staje się żywym NPC. WitnessedNPCs w logu (L5) to lista ID. Kontrakty P2P wskazują na ID. Reputacja per ID.
**To jest brama. Nic społecznego nie ruszy bez tego.**

═══════════════════════════════════════════════════════════
## 👥 L3 — Przynależność · ⬜
═══════════════════════════════════════════════════════════

### OCEAN (Wielka Piątka) — silnik decyzyjny
**Co robi:** osobowość filtruje Behavior Tree zamiast prostych skryptów.
**Jak:** 5 cech (Openness, Conscientiousness, Extraversion, Agreeableness, Neuroticism) jako DataAsset na NPC. Modyfikują wagi w BT.
**Gdzie się wpina:** filtruje KAŻDĄ decyzję — nerwowy + nieugodowy łamie własność (L2), budzi strach. Sumienny = idealny zbieracz (znosi do spiżarni). Otwarty eksploruje (EQS).
**Haczyki do rozbudowy:**
- Dziedziczenie cech: dzieci NPC mieszają OCEAN rodziców.
- Dryf osobowości: trauma podnosi Neuroticism, sukcesy Extraversion.
- Kompatybilność: podobne OCEAN → łatwiejsza przyjaźń/partnerstwo.

### Cykl dobowy jako tura ("blockchain planów")
**Co robi:** dzień = jedna tura. Rano NPC planuje kolejkę akcji wg potrzeb.
**Jak:** o świcie NPC ocenia Maslow → generuje rejestr planów (np. 1. znajdź jedzenie LUB 2. umowa z drwalem). Zdarzenia losowe (atak L0) przerywają plan.
**Gdzie się wpina:** spina wszystkie poziomy — plan to priorytety Maslowa przełożone na akcje. Wieczorem: synchronizacja przy ognisku.
**Haczyki do rozbudowy:**
- Plany wieloturowe: budowa domu jako projekt na 5 tur.
- Negocjacja planów: dwóch NPC uzgadnia wspólny plan (polowanie).

### Kontrakty P2P (micro-questy)
**Co robi:** NPC sami wystawiają sobie zlecenia barterowe — bez globalnego zarządcy.
**Jak:** FContract (kto, co, za co, status). Drwal potrzebuje siekiery, kowal drewna → wymiana. Zlecenia trafiają do wirtualnej puli. Gracz to jeden z aktorów.
**Gdzie się wpina:** wymaga NPCRegistry. Sukces kontraktu → +reputacja L4. Złamanie (NPC zginął/uciekł L0) → spór → detektyw L5.
**Haczyki do rozbudowy:**
- Reputacja kontraktowa: kto łamie umowy, dostaje gorsze deale.
- Kontrakty łańcuchowe: A potrzebuje od B, B od C → sieć zależności.
- Wielkie wyzwania: wspólne ubicie pająka → skok przyjaźni całej grupy.

### Safe Zones (EQS) + ognisko
**Co robi:** NPC odnajdują strefy bezpieczeństwa, tworzą grupy ~10, dzielą się wiedzą.
**Jak:** EQS (Searching Zone) symuluje eksplorację. NPC przypisują się do stref (jaskinie, polany). Wieczorem ognisko: indywidualna wiedza → "Globalna Wiedza Wioski".
**Gdzie się wpina:** most do świata — Safe Zones to konkretne miejsca na Caldreth (polany w nizinach). Wiedza = mapa odkrytych zasobów/zagrożeń.
**Haczyki do rozbudowy:**
- Konkurencja o strefy: dwie grupy chcą tej samej polany.
- Wiedza jako waluta: sprzedaż lokalizacji złoża.
- Strefy się rozrastają: mała osada → wioska → miasto.

═══════════════════════════════════════════════════════════
## 👑 L4 — Szacunek · ⬜
═══════════════════════════════════════════════════════════

### Reputacja ("Value")
**Co robi:** wartość jednostki; wysoka sława przyciąga innych, więcej zleceń, większe zaufanie.
**Jak:** float per NPC. Udany P2P +1, ratunek +5, mnożnik za liczbę świadków. DT_ReputationEvents.
**Gdzie się wpina:** wymaga NPCRegistry. Niska reputacja (kłamca L5) → banicja. Wysoka → przywództwo (Wódz).
**Haczyki do rozbudowy:**
- Reputacja lokalna vs globalna: bohater w jednej wiosce, obcy w drugiej.
- Plotka: reputacja rozchodzi się przez ognisko (sync), nie natychmiast.

### Mistrzostwo i monopole
**Co robi:** NPC po wielu turach przy zadaniu staje się "Mistrzem" — inni go szukają.
**Jak:** tag mistrzostwa po 50 turach przy danej pracy. Inni rozpoznają tag → lepsze deale ("zapłacę podwójnie").
**Gdzie się wpina:** tworzy naturalne monopole. Most do precyzji dłoni (L2) — uraz mistrza = utrata monopolu.
**Haczyki do rozbudowy:**
- Uczniowie: mistrz uczy → transfer umiejętności.
- Sekrety zawodowe: mistrz nie dzieli się techniką → wiedza umiera z nim.

### Macierz preferencji + dominacja/frakcje
**Co robi:** unikalność (luksusowe dobra), sympatie/antypatie, ewolucja od przetrwania do władzy.
**Jak:** preferencje (lubię/nie lubię jedzenie, kolory, konkretne NPC) modyfikują chęć współpracy. Silna grupa wojowników → dominacja nad słabszymi strefami.
**Gdzie się wpina:** wstęp do L5 (feudalizm). Frakcje rzutują siłę na sąsiednie Safe Zones.
**Haczyki do rozbudowy:**
- Moda: preferencje kolorów/stylu rozchodzą się jak trend.
- Sojusze i zdrady między frakcjami.

═══════════════════════════════════════════════════════════
## 🚀 L5 — Samorealizacja · ⬜
═══════════════════════════════════════════════════════════

### Detektyw / system sprawiedliwości (alibi)
**Co robi:** Wódz krzyżowo sprawdza logi akcji → wykrywa kłamstwa → kary.
**Jak:** każdy NPC loguje [Czas, Strefa, Akcja, WidzianiNPC]. Spór → Wódz przesłuchuje. Sprzeczność w logach ("byłem na zachodzie" vs "nikogo tam nie było") → kłamca pęka. Kara: reputacja −10, banicja ≤−20.
**Gdzie się wpina:** wymaga NPCRegistry (WitnessedNPCs = lista ID) + logu akcji + reputacji L4. To kulminacja całego systemu — emergentna sprawiedliwość.
**Haczyki do rozbudowy:**
- Fałszywi świadkowie: sprzymierzeniec kłamie dla oskarżonego.
- Poszlaki: ślady (otwarty zamek L2, krew z urazu L2) jako dowody.
- Sędzia vs Wódz: rozdział władzy sądowniczej.

### Eureki + feudalizm
**Co robi:** zaspokojenie wszystkich potrzeb → punkty innowacji → globalne wynalazki. Silna strefa pobiera haracz.
**Jak:** Innovation Points (gdy Maslow zaspokojony) → Eureki (ogień, koło, broń) z prereqami. Feudalizm: "bronimy was za 10% spiżarni".
**Gdzie się wpina:** Eureki zmieniają cały świat (ogień → temperatura L1, broń → walka L0/L4). Dziesięcina = ekonomiczna dominacja zamiast wojny.
**Haczyki do rozbudowy:**
- Drzewo technologiczne emergentne: różne wioski odkrywają różne ścieżki.
- Utrata wiedzy: jeśli jedyny znawca ognia zginie, wioska cofa się.

═══════════════════════════════════════════════════════════
# CZĘŚĆ II — ŚWIAT CALDRETH
═══════════════════════════════════════════════════════════
> Wyspa wulkaniczna, koncentryczne strefy. WorldSizeUU = 1000000. Generowana proceduralnie (Tools/MapGen),
> importowana do UE jako ACaldrethZone + markery POI. Dwa zegary: WULKAN i SŁOŃCE.

**🕐 Tempo świata (zegar dobowy):** `WORLD/BP_DayNightCycle` ustawia w BeginPlay `TIME_(X)=M=6` →
`TimeSpeed = 1/6` game-h/real-sek → **pełna doba = 2.4 real-min** (start 10:00). To źródło prawdy czasu.
**Nota:** 2.4 min/dobę jest szybkie — wygodne do testów; **do ew. zwolnienia później** (zmiana `TIME_(X)=M`).
Silnik snu (L1-06/07) **podąża za tym zegarem automatycznie** (Maslow czyta `TimeSpeed` przy BeginPlay i
liczy `AwakeRatePerTick = TimeSpeed × interwał`) — zwolnienie zegara nie wymaga ruszania snu, „jeden zegar".

## Strefy koncentryczne (6 pierścieni) · 🔨
**Co robi:** dzieli wyspę na strefy wg realnej geologii wulkanu; każda strefa = inny afford dla NPC.
**Jak:** ACaldrethZone z EZoneType. Od centrum: Kaldera (ogień, śmierć) → Stoki popiołu (ciepło, jałowo) → Góry (bariera, kamień) → Lasy zboczowe (drewno/zwierzyna/zioła) → Niziny/polany (osady, żyzne) → Wybrzeże (woda, ryby). Flagi bSpawnable/bHabitable, DebugColor. Data-driven przez DT_ZoneDefs.
**Gdzie się wpina:** NPC pyta "w jakiej strefie jestem?" → to napędza zasoby (L1/L2), Safe Zones (L3), temperaturę (L1). Niziny = większość z 1000 NPC. Kaldera = rzadkie zasoby za ryzyko życia.

**⚙️ Query „w jakiej strefie stoję?" — plan wydajności (500+ NPC).** Prymityw: `ACaldrethZone::GetZoneAtLocation(World, Loc)→ACaldrethZone*` (+ `GetZoneTypeAtLocation→EZoneType`). Najmniejsza zawierająca strefa wygrywa (Beach/Oasis bije Ocean). Status ETAP 5: ✅ zbudowane, zweryfikowane (18 stref).
- **Jak często NPC pyta:** na planowaniu (świt tury), przy przekroczeniu granicy strefy, ew. w BTService (~1–2 s). **Per-frame WYKLUCZONE** regułą „no Tick". Realny pułap ≈ kilkaset zapytań/s.
- **Stan TYMCZASOWY:** `GetZoneAtLocation` używa `TActorIterator` = skan **O(wszystkie aktory)** per zapytanie. OK przy 18 strefach bez konsumenta NPC; przy 500 NPC per-frame byłoby ~90M ops/s (zabójca). Zostawione świadomie z `// PERF TODO`, bo dziś niegroźne.
- **Docelowo: hybryda (budujemy GDY będzie klasa NPC):**
  1. **Cache „moja strefa" na NPC** (`TWeakObjectPtr<ACaldrethZone>`) — zapytanie sprawdza tylko `CachedZone->ContainsWorldLocation`; pełny lookup tylko przy opuszczeniu strefy. ~99% trafień. **Największy zysk, najtańszy runtime** — ale wymaga kodu NPC.
  2. **Subsystem-indeks** (`UCaldrethZoneWorldSubsystem`, grid-bucket 32×32, build-once na load) — zamienia O(aktory) na **O(1)+~3 testy**. Kilka KB, bez Tick, bez siatki pikseli. Cache-miss idzie przez indeks, nie pełny skan.
- **Accuracy (osobno od speed) — ODŁOŻONE razem z cache+indeks (gdy będzie klasa NPC):** obrysy stref to dziś **AABB** → nakładające się bboxy dają niejednoznaczność (Grey Spring i River Source wpadają w ten sam Oasis). Ale **smallest-area-wins wystarcza dla 18 stref**, a precyzji `GetZoneAtLocation` **nikt jeszcze nie używa** (czeka na konsumenta NPC, jak cała query-optymalizacja). Fix (w tej samej paczce co cache+indeks) = **Moore-trace** prawdziwych wielokątów w importerze (`PointInPolygon` już je obsłuży, zero zmian w query).
- **Reguła:** nie zostawiać `TActorIterator` jako prymitywu docelowego; mierzyć cadence dopiero przy realnym konsumentcie NPC. Cała trójka (cache + subsystem-indeks + Moore-trace obrysy) ląduje razem, gdy powstanie klasa NPC pytająca o strefę.

**Haczyki do rozbudowy:**
- Strefy dynamiczne: erupcja wulkanu rozszerza stoki popiołu, niszczy las.
- Mikrobiomy: w obrębie strefy lokalne różnice (polana w lesie).

## Rzeki wulkaniczne · 🔨
**Co robi:** rzeki spływają promieniście z wulkanu; część to lawa, część wysycha.
**Jak:** trace steepest-descent od szczytu. Liczba losowa (min 3 stałe + tymczasowe). lava_fraction 0.1 (zawsze min 1 lawa). Permanent+water → sawanna/zieleń/stamp RIVER. Temporary = wadi (NIE zielenić, wysycha). Lava → stamp LAVA. River {path, kind, permanence}.
**Gdzie się wpina:** woda stała = pragnienie L1 (NPC osiada przy rzekach). Wadi wysycha (pogoda) → migracja. Lawa = bariera + zagrożenie. Sawanna wzdłuż rzek = żyzny teren.
**Haczyki do rozbudowy:**
- Most/bród: przeprawa przez rzekę jako punkt strategiczny.
- Lawa stygnie → nowy kamień/obsydian (zasób).
- Powódź sezonowa: rzeka wylewa, użyźnia, ale zagraża osadom.

## Pogoda (chmury Lagrange'owskie) · ⬜
**Co robi:** chmury rodzą się z parowania, wiatr je niesie, zrzucają deszcz; nawadniają strefy.
**Jak:** ~60 wędrujących paczek wilgoci (nie siatka). Rodzą się nad ciepłym morzem (wschód, nawietrzna), wiatr niesie na zachód. Siła wiatru = jak głęboko deszcz dociera. Orografia (góry) wymusza opad. SoilMoisture per strefa (nie komórka). Parowanie ∝ nasłonecznienie. Koszt niezależny od liczby NPC.
**Gdzie się wpina:** deszcz → wilgoć stref → wadi płyną/wysychają (pragnienie L1). Susza przesuwa ludzi. Klimat = średnia z pogody.
**Haczyki do rozbudowy:**
- Burze jako zdarzenia: widać nadciąganie znad morza, NPC szuka schronienia.
- Pora deszczowa/sucha (monsun) jako zegar migracji.
- Wpływ wulkanu na pogodę: popiół zaciemnia niebo, ochładza.

## Słońce sezonowe · 🔨 (do przebudowy?)
**Co robi:** prawdziwy łuk słońca; nasłonecznienie zależne od nachylenia stoku i pory roku.
**Jak:** astronomia — deklinacja δ ≈ 23.44°·sin(360°·(dzień−81)/365), wysokość/azymut z szerokości geo + kąta godzinnego. Insolacja = max(0, dot(Normal_strefy, kierunek_słońca))·(1−zachmurzenie). Normalne stref liczone raz z heightmapy.
**Gdzie się wpina:** nasłonecznienie → temperatura L1 (gorące stoki vs zimne), parowanie (pogoda). Długość dnia → sen L1 (HoursAwake). Drugi zegar obok wulkanu.
**🔴 Decyzje otwarte:** szerokość geo wyspy (silne sezony czy monsun?), oś "słonecznej strony", czy grań rzuca cień (drogie), wierność modelu (czy długość dnia zmienna).
**Haczyki do rozbudowy:**
- Przesilenia jako święta/wydarzenia kulturowe NPC.
- Stoki południowe = premium pod osady (most do L3/L4 — konkurencja o ziemię).

═══════════════════════════════════════════════════════════
# 🔗 MOSTY NPC ↔ ŚWIAT (gdzie systemy się spotykają)
═══════════════════════════════════════════════════════════
- **Pragnienie L1 ↔ rzeki/pogoda:** susza wysycha wadi → NPC migruje. Geografia tętni.
- **Temperatura L1 ↔ słońce/stoki popiołu:** zimno gna NPC do ciepła wulkanu (blisko śmierci).
- **Zasoby L1/L2 ↔ strefy:** drewno w lesie, ryby na wybrzeżu, obsydian w kalderze.
- **Safe Zones L3 ↔ niziny/polany:** społeczeństwa rodzą się w konkretnych miejscach.
- **Frakcje L4 ↔ góry:** bariery dzielą wyspę → naturalne granice państw.
- **Eureki L5 ↔ kaldera:** rzadkie materiały na wynalazki za cenę ryzyka.

═══════════════════════════════════════════════════════════
# 💡 JAK UŻYWAĆ TEGO DOKUMENTU DO BURZY MÓZGÓW
═══════════════════════════════════════════════════════════
1. Czytaj "haczyki do rozbudowy" — to nasiona pomysłów.
2. Szukaj nowych MOSTÓW: która mechanika L mogłaby wpiąć się w którą strefę/zjawisko świata?
3. Pytaj "co by było gdyby": gdyby wulkan wybuchł? gdyby zmarł jedyny mistrz ognia?
4. Każdy nowy pomysł → najpierw tu opisz (co robi/jak/gdzie się wpina), POTEM do ROADMAP jako zadanie.
5. Mechaniki definiujące rozgrywkę = DESIGN GATE (najpierw architekt, potem kod).
