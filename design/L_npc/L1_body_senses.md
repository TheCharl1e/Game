# L1 — Ciało i zmysły (Body / Senses)

**Oś:** L (NPC) · **Warstwa:** L1 · **Status:** ✅ · **Migrowano:** ETAP 2 (2026-07-01). Strzałka-w-dół: L1 zna tylko L0.

Warstwa modeluje fizyczną integralność NPC: 26-częściowe ciało, którego uszkodzenia kaskadują na części zależne i na zmysły pochodne. Zmysły są tanim, cache'owanym odczytem konsumowanym przez BT/EQS. Percepcja świata (co NPC widzi) stoi na jednej prawdzie — `AIPerception_Sight` — migrowanej do C++ (`UNPCPerceptionComponent`).

## Ciało (26 części, kaskada)

- **26 części w hierarchii zawierania** (`UBodyConditionComponent`, `Source/Stan_Pierwotny/Body/`): Body → Head → oczy/uszy/język; Torso → ramiona → dłonie → 5 palców każda; nogi → stopy. Zdrowie części: 0.0–1.0.
- **Reguła kaskady (rodzic-kapuje-dziecko):** `GetPartEffectiveHealth(part)` = własne zdrowie ograniczone (min) całym łańcuchem rodzica. Ramię sprawne w 40% kapuje swoją dłoń i palce na 40% niezależnie od ich własnego zdrowia.
- **Urazy** (`EInjuryType`): None, Wound, Fracture, Sprain, Burn, Sever (HUD + leczenie). Leczenie: `HealPart` +0.3.
- **Magazyn:** `PartHealth`/`PartInjury` to rzadkie mapy — brak klucza = część zdrowa. Bez Tick. Dane opcjonalnie ze współdzielonego `UBodyHierarchyAsset`; fallback do wbudowanego domyślnego.

## Zmysły (pochodne, cache'owane)

Zmysły (`ESenseType`: Vision, Hearing, Speech, HandPrecision L/R, Mobility) to **ważona średnia efektywnego zdrowia** części składowych, **cache'owana i przeliczana tylko przy zmianie (uszkodzeniu)** — odczyt w BT/EQS jest O(1), bez przeliczania per-tick.

Wagi:
- **Wzrok:** oczy 0.5 / 0.5.
- **Słuch:** uszy 0.5 / 0.5.
- **Mowa:** język 1.0.
- **Precyzja dłoni:** Dłoń 0.4 / Kciuk 0.3 / Wskazujący 0.1 / Środkowy 0.08 / Serdeczny 0.07 / Mały 0.05.
- **Mobilność:** noga 0.35 / stopa 0.15 (×2, per strona).

Konsumenci: wzrok → wykrywanie zagrożeń [[L0_fight_or_flight]], znajdowanie jedzenia [[L1_hunger]], świadek w śledztwie L5. Precyzja dłoni → jakość craftingu L2/L4. Mobilność → ucieczka L0, prędkość ruchu. Mowa → sukces kontraktów P2P L3, zeznania L5.

**Dwa enumy ciała (celowo utrzymywane oba):** BP `E_BodyPart` (6 regionów) = warstwa klikalna/HUD (`WBP_Tab_Body`: sylwetka + wiersze części); C++ `EBodyPart` (26 części) = warstwa danych. 6 regionów BP mapuje na 26 części C++.

## Percepcja (jedna prawda: AIPerception_Sight)

- **Jedyna prawda percepcji = `AIPerception_Sight`** (`AIPerceptionComponent`) na kontrolerze `BP_NPC_AI` — nadzbiór starego cone-vision: FOV 90 + dystans + histereza (SightRadius 1500 / LoseSightRadius 2000) + event-driven + loss-events, niski koszt ×500. Cone nie niósł żadnego unikatu.
- **Migracja do C++:** nowy `UNPCPerceptionComponent` (`UActorComponent`) na kontrolerze (percepcja ≠ biologia, więc NIE rozszerzenie Maslowa; źródło percepcji jest po stronie kontrolera, więc NIE na pawnie). Brak cross-actor cached-ptr — percepcja jest w 100% controller-side.
- **Przechowywanie:** `TArray<TWeakObjectPtr<AItemBase>> PerceivedFood` — weak-ptr auto-nullują zniszczone aktory (siatka bezpieczeństwa). Licznik `GetPerceivedFoodCount()` liczy tylko `IsValid` (compact-on-read) — koniec monotonicznego, kłamiącego `HowMuchFood`.
- **Handler zysku/straty:** gain (`WasSuccessfullySensed=true` + tag `Food`) → `AddUnique`; loss (`=false`) → `Remove` + emit `OnFoodPerceptionLost` (`BlueprintImplementableEvent`, dziś pusty hak pod pamięć L3). Loss łapie żywe-poza-zasięgiem, czego stary licznik nie miał (tablica tylko rosła). **MaxAge = 0** — „widzę TERAZ" bez zapominania (zapominanie należy do KROKU 2 / pamięci L3-MEM).
- **Kasacja martwego cone-vision:** martwe `ScanForFood`/`ScanForDrinks` (0 call-sites), `VisibleFoods`/`VisibleFoodsCount`, martwy duplikat pawn-owego `AIPerception` (R2.3, brak handlera/ref), `PruneFoodArray` oraz gałąź Food handlera `OnTargetPerceptionUpdated`. **Zostaje** `AIPerceptionStimuliSource_Sight` na pawnie (pawn JEST postrzegany przez innych) oraz gałęzie Drink/Shelter (TECH-11b).

**Źródło:** DESIGN_how_it_works.md CZ.I (Body) + TECH-11_PERCEPTION_design.md (Game_git)
