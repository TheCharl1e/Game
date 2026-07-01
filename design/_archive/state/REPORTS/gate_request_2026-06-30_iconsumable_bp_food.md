# GATE-REQUEST — naprawa BTTask_Eat przez interfejs IConsumable (Game_58 / UE 5.8) · 2026-06-30

> **OD:** Executor (Claude Code) · **DO:** Architekt (Chat-Claude) + Dyrektor (Szymon)
> **Status:** RECON-STOP — diagnoza zrobiona (read-only), **kierunek zatwierdzony przez dyrektora = IConsumable**,
> kod/asset NIE ruszony. Bazuje na diagnozie BTTask_Eat z 2026-06-30. Nie implementuję do „zatwierdzam".

## 1. Problem (dowiedziony)
- **BTTask_Eat nie kompiluje w 5.8** (`Game_58.log` 12:13:53): „Nie można połączyć gniazd Food i As BP Food — Item Base ref nie pasuje do BP Food ref". Pin `Food`=`AItemBase*`, wyjście `Cast To BP_Food`=`BP_Food*`.
- **Root cause:** w 5.8 `BP_Food` reparentowany na **`AAffordanceSourceActor`** (rejestr afordancji / forage L0-TA-S1, PIE-proven żywy), a warstwa jedzenia zakłada **`AItemBase`**. BP ma JEDNEGO rodzica → konflikt.
- **Decyzja dyrektora (zalockowana):** NIE merge baz (bo rejestracja afordancji bezwarunkowa — patrz §2), NIE robocopy 5.7→5.8 (5.8 ma nowszy BP_Food + 4 pliki więcej, kopia by cofnęła afordancje). **Idziemy `IConsumable`:** BP_Food zostaje `AffordanceSourceActor` i DODATKOWO implementuje interfejs jadalności; jedzenie woła interfejs zamiast wymuszać wspólną bazę.

## 2. Fakty z kodu (read-only, do projektu interfejsu)
- **Rejestracja afordancji = BEZWARUNKOWA** (`AffordanceSourceActor.cpp:14-26`): `BeginPlay` zawsze woła `RegisterAffordanceSimple(AffordanceType, loc, Yield, RegenPerHour, this, ColdDampen)`; brak flagi/tagu/bramki. `EndPlay` wyrejestrowuje. → dlatego merge baz odpada (każdy AItemBase auto-rejestrowałby się jako afordancja).
- **`AItemBase` = food-owy aktor świata:** `MeshComponent`, `OwnerID(int32)`, `FoodTableRowName`, `RemainingPortion=1.0`, `ConsumePortion()`, `CanBeEatenBy()`, `IsStolenBy()`. **Zero podklas C++**, nigdy `SpawnActor<AItemBase>`.
- **Ekwipunek/P2P/materiały = struktury** (`FItemDefinition : FTableRowBase`, `FItemStack`, `EItemType {None,Food,Resource,Tool,Clothing,Luxury}`) — NIE aktory AItemBase.
- **`AffordanceSourceActor` = goły marker** (konstruktor: tylko `USceneComponent` root, brak mesha; `: public AActor`). Mesh/wizual BP_Food siedzi na samym BP (3 BlueprintComponents).
- **Konsumenci `AItemBase` (wszyscy padają na BP_Food w 5.8):**
  - `UMaslowBiologicalComponent::StartEatingItem(AItemBase* Food, UDataTable*, int32)` (Maslow:1055) + `Cast<AItemBase>(EatTargetFood)` (1126).
  - `UNPCPerceptionComponent`: `PerceivedFood = TArray<TWeakObjectPtr<AItemBase>>`, `GetNearestPerceivedFood()→AItemBase*`, `Cast<AItemBase>(Actor)`.
  - graf `BTTask_Eat`: `Cast To BP_Food → StartEatingItem`.
- **→ To nie jeden zerwany pin** — cała warstwa AItemBase jest w 5.8 rozjechana z BP_Food. Żywy forage idzie ścieżką afordancji (`WorldAffordanceSubsystem` + EQS), nie AItemBase-percepcji.

## 3. Decyzje do ZALOCKOWANIA (gate — NIE wybieram)
- **D1 — Kształt `IConsumable`:** które API wchodzi? Kandydaci: `GetFoodRowName()`/`GetFoodRow(FFoodItemRow&)`, `ConsumePortion(float)→float`, `GetRemainingPortion()`, `CanBeEatenBy(int32)`, `OnDepleted` (event), własność (`OwnerID`/`IsStolenBy`). Minimalny zestaw (jedzenie) vs pełny (własność/kradzież L5).
- **D2 — Nośnik implementacji:** interfejs na BP (`BlueprintNativeEvent`, BP_Food implementuje w grafie) **czy** nowy lekki `UConsumableComponent` (niesie `FoodTableRowName`/`RemainingPortion`/`ConsumePortion`, wpinany na BP_Food obok afordancji)? Komponent = data-driven, reużywalny, zgodny z „C++ mózg / BP ciało".
- **D3 — Los `AItemBase`:** zostaje (implementuje `IConsumable`, kompat dla ewentualnych world-itemów) czy deprecjonowany na rzecz `AffordanceSourceActor`+`IConsumable`? Gdzie ostatecznie żyje `FoodTableRowName`/`RemainingPortion` (interfejs/komponent/AffordanceSourceActor)?
- **D4 — Sygnatury C++:** `StartEatingItem(AItemBase*)` → `StartEatingItem(TScriptInterface<IConsumable>)` lub `(AActor* + Implements<UConsumable>())`? Zmiana publicznego API + wszystkich call-site'ów + `EatTargetFood` (dziś `TWeakObjectPtr<AItemBase>`).
- **D5 — Percepcja:** `PerceivedFood<AItemBase>` → `IConsumable`, czy **porzucić AItemBase-percepcję** całkiem na rzecz afordancji+EQS (i tak żywej)? Reconcyl dwóch ścieżek food-finding (dług z TECH-11 percepcji).
- **D6 — Graf BTTask_Eat:** `Cast To BP_Food` → cast/`Does Implement Interface` `IConsumable` + wywołanie. To realna naprawa `.uasset` (manual editor save, ryzyko RF_Transient — patrz higiena „no Monolith save for BP defaults").
- **D7 — Zakres slice'a:** minimalny (odblokować BTTask_Eat: BP_Food→IConsumable, `StartEatingItem` przez interfejs, graf) **vs** pełny (D5 percepcja + D3 deprecjacja AItemBase-as-food).

## 4. Powierzchnia implementacji (po „zatwierdzam", żeby architekt znał koszt)
- **Nowy:** `IConsumable.h` (`UINTERFACE`) [+ ew. `UConsumableComponent.{h,cpp}` jeśli D2=komponent].
- **C++ patch (nie regen):** `MaslowBiologicalComponent` (`StartEatingItem` sygnatura + cast + `EatTargetFood` typ), `NPCPerceptionComponent` (jeśli D5), nośnik impl. na BP_Food/AffordanceSourceActor. **Build editor-closed → UHT.**
- **Asset:** graf `BTTask_Eat` (re-cast na interfejs), BP_Food implementuje `IConsumable` (manual save).
- **Perf ×500:** interface-cast = O(1) na akcję, zero nowego ticka; afordancje już zarejestrowane. Negligible.
- **Verify:** BTTask_Eat kompiluje czysto + PIE pełna pętla jedzenia (jak L1-11b: głód→find→`StartEating`→`ConsumeBite`×N→`StopEating`→`OnDepleted`) **na BP_Food-jako-AffordanceSourceActor** — twarde liczby.

## 5. Rekomendacja executora (opinia, NIE decyzja)
D2=**`UConsumableComponent`** + D7=**minimalny slice** najpierw: komponent niesie food-data (zero zależności od bazy aktora, wpina się na BP_Food obok afordancji, reużywalny dla corpse/innych), odblokowuje BTTask_Eat bez ruszania percepcji. Percepcja (D5) + deprecjacja AItemBase (D3) = osobny, późniejszy slice z własnym verify. Minimalizuje powierzchnię i nie miesza z żywą ścieżką afordancji.

## 6. Czego NIE robię
Zero kodu/assetu do zatwierdzonego gate'u. Nie wybieram kształtu interfejsu, nośnika, sygnatur ani zakresu.
**Pending niezwiązane:** TECH-07 fix `GetHPPercent` naniesiony w źródle `E:\Game_58\...\MaslowBiologicalComponent.cpp` (odwracalny, niezbudowany) — czeka na zamknięcie edytora do builda.
