# GATE: BTTASK-EAT-WIRING-01 — kanoniczny C++ BTTask_Eat (rich appetite ↔ afordancja)

Data: 2026-06-30 (PIE-verified 2026-07-01) · Branch: `feat/bttask-eat-wiring` (od `game-58`) · Status: **✅ DONE — PIE-VERIFIED (pełna pętla, twarde liczby)** · Target: E:\Game_58 (UE 5.8)
Werdykt architekta: ścieżka A (okabluj rich model). Buduje na zshipowanym APPETITE_GRUBAS. Nadrzędne nad [[ICONSUMABLE_EAT_01]] (C++ parked-ready).

## RESOLVED (kontrakt zalockowany przez architekta)
- JEDEN kanoniczny `BTTask_Eat` (C++). BP `BTTask_Eat.uasset` → do deprecjacji (kolizja nazw ginie). NIE kasować bez potwierdzenia.
- **Claim/release = C++** (nie BP abort). Cancel-on-death slot release = inwariant §8; cleanup w `EndTask`/`OnTaskFinished` + bezpiecznik w NPC `EndPlay`.
- **Bites = BP** (D-EAT: `AnimNotify_EatBite` wybija takt). C++ odpala animację przez BIE `PlayEatMontage(FoodActor)`; AnimNotify → `ConsumeBite()`.
- **Afordancyjny claim = single source of truth celu.** `NearestFoodObject` (BB) KARMIONY z claimu (task zapisuje BB z zarezerwowanego slotu), nigdy odwrotnie.
- Finish/abort → `StopEating(Reason)` + `ReleaseAffordanceClaim()`.
- **Significance-gating (500+):** near/high → pełna pętla `PlayEatMontage→AnimNotify→ConsumeBite` (event-driven); far/low → jeden `SettleMealInstant()` (suma kęsów, zero montażu). Stary `ConsumeFood` przeżywa jako ścieżka LOD-far, NIE konkurencyjny task. Koszt O(near_eating × bites), ograniczony budżetem claimów, nie O(N).

## DEFAULTS (pre-answered — assume-log-continue)
- Significance: użyj istniejącego sygnału LOD jeśli jest; BRAK → stub `bool bIsHighSignificance=true` (TODO hook) + FLAG. → patrz F5.
- `SettleMealInstant`: net = SatietySetpoint capped, makra routowane jak w pętli bite (reuse `DepositBiteMacros`), jeden przebieg. Bez nowej matematyki.
- Branch: `feat/bttask-eat-wiring` (od game-58). ✔ utworzony.

## RECON (żywy kod — potwierdzone file:line)
- **C++ `UBTTask_Eat`** `AI/BTTask_Eat.cpp:16-45` — DZIŚ synchroniczny: `QueryNearest(Nourishment, PawnLoc, ConsumeRadius)` → `TryReserve` → `Consume` → `Release` (od razu) → `Maslow->ConsumeFood(0,0,Granted)`. Brak claim-persist, montażu, BB-write, StartEating. (`BTTask_Drink.cpp:16-45` = bliźniaczy wzorzec.)
- **Afordancja** `World/WorldAffordanceSubsystem.h` — `QueryNearest:101`, `TryReserve(Id,By):105`, `Release(Id,By):109`, `Consume(Id,Amt,OutGranted):113`, `GetAffordance(Id,OutHandle):117`. `FAffordanceHandle.Owner` (weak AActor, :54) = aktor zasobu (BP_Food). Death-failsafe claimu już jest (`ReservedByActor` weak, :61-62).
- **Maslow rich-eat** `MaslowBiologicalComponent`: `StartEating(AActor* Food, const FFoodItemRow&, int32 BiteCount)` BlueprintCallable `h:813/cpp:1033`; `StartEatingItem(TScriptInterface<IConsumable>, UDataTable*, int32)` `h:822/cpp:1055` (po ICONSUMABLE); `ConsumeBite()` `h:837/cpp:1093`; `StopEating(EEatStopReason)` `h:841/cpp:1147`; `OnMealEnd(...)` BIE `h:861`. Auto-stop w ConsumeBite przy sytości/wyczerpaniu `cpp:1137-1144`.
- **Bite-tick** `AI/AnimNotify_EatBite.cpp:17-21` — mesh owner → `FindComponentByClass<UMaslowBiologicalComponent>` → `ConsumeBite()`. GOTOWE.
- **Konsumowalny na celu**: afordancja `Owner` = BP_Food, który (ICONSUMABLE krok A) ma `UConsumableComponent` (impl `IConsumable`). Integracja: `GetAffordance(Id).Owner` → `FindComponentByClass<UConsumableComponent>` → `StartEatingItem(...)`.
- **AI controller C++** `AI/NPCAIControllerBase.{h,cpp}` (pawn = BP_NPC_Character, BP-only — brak C++ bazy pawna).

## IMPLEMENTACJA — C++ DONE (compile-validated, interim GAME build: Succeeded) — defaulty zatwierdzone „jedź"
- **Maslow** (`MaslowBiologicalComponent.{h,cpp}`): native delegat `FMaslowOnEatingStopped OnEatingStopped` broadcastowany w `StopEating` (F2); `SettleMealInstant(TScriptInterface<IConsumable>, UDataTable*, int32)` = reuse StartEatingItem+ConsumeBite loop (LOD-far).
- **Controller** (`NPCAIControllerBase.h`): BIE `PlayEatMontage(AActor* FoodActor)` + `StopEatMontage()` (F1 — host = kontroler C++, BP_NPC_AI implementuje).
- **`BTTask_Eat.{h,cpp}`** przepisany na **kanoniczny latentny task** (`bCreateNodeInstance=true`): Claim(Reserve) → resolve `UConsumableComponent` na afordancji Owner → BB `NearestFood`=Owner (F3) → near: `StartEatingItem`+bind `OnEatingStopped`+`PlayEatMontage` → InProgress; AnimNotify_EatBite→ConsumeBite→auto-StopEating→`HandleEatingStopped`→Release+`FinishLatentTask`. far: `SettleMealInstant`+Release→Succeeded. `AbortTask`: unbind→StopEating(Interrupted)→StopEatMontage→Release→Aborted. Cleanup idempotentny; claim też auto-wolny przy śmierci (subsystem `ReservedByActor`).
- **Property (F4):** `UDataTable* FoodTable`, `int32 BiteCount=4`, `FBlackboardKeySelector NearestFoodObjectKey`, `bool bIsHighSignificance=true` (F5 stub).
- **Log:** reuse istniejących kategorii — `LogWorldAffordance` (Claim/Release/task) + `LogMaslow` (StartEating/Bite/Stop/SettleInstant); pełna ścieżka grep-owalna (zamiast nowej kategorii — assume-log).

## ⚠️ FLAGI — rozwiązane defaultami (zatwierdzone „jedź"); F3 key = NearestFood (Object, BB_NPC)
- **F1 — `PlayEatMontage` BIE nie istnieje i nie ma C++ hosta na pawnie** (pawn = BP-only). Propozycja default: BIE `PlayEatMontage(AActor* FoodActor)` na **`ANPCAIControllerBase`** (C++; task ma `GetAIOwner()`), BP_NPC_AI implementuje i gra montaż na opętanym pawnie. Alternatywa: interfejs `IEatAnimation` na pawnie (wzór IConsumable). → potrzebny wybór.
- **F2 — mechanizm zakończenia taska latentnego nie istnieje.** ConsumeBite robi auto-`StopEating`, ale C++ task nie wie KIEDY skończyć węzeł BT. Propozycja default: dodać do Maslow multicast `FOnEatingStopped` broadcastowany w `StopEating`; task binduje → `FinishLatentTask(Succeeded)` + Release. → potwierdź podejście.
- **F3 — `NearestFoodObject` BB-key nie istnieje w C++.** Dziś task pyta afordancję wprost (zero BB-write). Propozycja default: `FBlackboardKeySelector NearestFoodObjectKey` na tasku, zapis `Owner` po Reserve. Wymaga, by BB_NPC miał Object-key. → potwierdź nazwę/obecność klucza.
- **F4 — źródło `FoodTable` (DT_FoodStats).** Dziś dostarcza je graf BP. C++ task potrzebuje `UPROPERTY UDataTable* FoodTable` (+ `int32 BiteCount`) ustawiane przez designera. → default: dodać property, FLAG do ustawienia w assecie taska.
- **F5 — brak sygnału significance** → stub `bIsHighSignificance=true` + TODO hook (zgodnie z DEFAULT).

## Definition of Done (FALSYFIKOWALNY)
1. Build editor-closed czysty.
2. PIE: NPC near → `Claim → StartEating → PlayEatMontage → ConsumeBite×N → StopEating(Full/Finished) → Release`; far → `SettleMealInstant → Release`. Twarde liczby z `Saved/Logs/` (osobna kategoria: Claim/Release/StartEating/Bite/Stop/SettleInstant). Cancel-on-death: NPC ginie w trakcie → slot zwolniony (log).
> Test zależy od SLEEP_FAINT_TRAP_01 (inaczej NPC mdleje z głodu snu przed turą jedzenia).

## POZOSTAŁO do zamknięcia bramki (manualne / wymaga edytora — STOP po mojej stronie)
1. **Editor-build** nowego C++ (zamknij edytor → zbuduję `Game_58Editor`).
2. **Okablowanie assetów (instruuję, Ty klikasz — MCP koruptuje piny):**
   a. `BT_Exploration`: w gałęzi jedzenia użyj C++ `BTTask_Eat` (już tam jest), ustaw na węźle: `FoodTable`=DT_FoodStats, `BiteCount`, `NearestFoodObjectKey`=**NearestFood**.
   b. `BP_NPC_AI` (kontroler): zaimplementuj event `PlayEatMontage(FoodActor)` → zagraj montaż jedzenia na pawnie; `StopEatMontage` → przerwij. Montaż MUSI nieść `AnimNotify_EatBite` ≥ BiteCount razy (inaczej sesja nie domknie się przez bites — domknie ją dopiero sytość).
3. **PIE verify** — DoD niżej. **Zależny od SLEEP_FAINT_TRAP_01** (NPC mdleje z głodu snu przed turą jedzenia).

## ✅ DOMKNIĘCIE 2026-07-01 (executor — dyrektor: „zrób to sam", ścieżka C++ zatwierdzona)
**F1 ZREWIDOWANE (zgoda dyrektora):** odtwarzanie montażu przeniesione z BIE-w-BP do **C++** na `ANPCAIControllerBase` — `PlayEatMontage`/`StopEatMontage` implementowane w C++, montaż jako data-driven `UPROPERTY UAnimMontage* EatMontage` (set na CDO `BP_NPC_AI`, nie pin grafu). Powód: graf BP z object-pinem nie da się okabluować przez MCP bez korupcji. Konwencja „anim w BP" nagięta świadomie; referencja zostaje data-driven, montaż = asset BP.

**Editor-build:** `Game_58Editor Win64 Development` → Result: Succeeded (po fixie C4458 hides member).

**Okablowanie (wszystko przez MCP/Python, zweryfikowane readbackiem):**
1. `AM_NPC_Eat` (`/Game/DocelowaGra/AI_NPC/`) zbudowany z `AS_NPC_Eat` (szkielet NPC `/Game/DocelowaGra/.../SK_Mannequin`), play_length 7.57 s, slot `DefaultSlot`, **4× notify `AnimNotify_EatBite`** @1.36/3.03/4.69/6.36 s.
2. `BP_NPC_AI` CDO `EatMontage`=`AM_NPC_Eat` (KLUCZOWE: wymaga `compile_blueprint` PO set — inaczej auto-compile PIE resetuje inherited-default do null).
3. `BT_Exploration`→`BTTask_Eat_0`: `FoodTable`=DT_FoodStats, `BiteCount`=4, `NearestFoodObjectKey`=**`NearestFoodObject`** (NIE `NearestFood` — gate mylił nazwę; realny klucz w BB_NPC).
4. `BP_Food` `ConsumableComponent` już istniał; `FoodTableRowName` poprawiony z błędnego „DT_FoodStats" → **`Berries`** (realny wiersz).

**PIE DoD #2 — SPEŁNIONE (twarde liczby, `Saved/Logs/Game_58.log`, oba NPC):**
`Maslow.SeedRestedHungry` + `slomo 0.3` → `[Eat] claimed id=20 food=BP_Food_C_19 sig=high` → `StartEating Volume=1.0 bites=4 (Carb=15)` → `playing EatMontage=AM_NPC_Eat (len=7.57)` → **ConsumeBite×4 (bitesLeft 3→2→1→0, mealSize 0.2→0.5→0.8→1.0, Glucose 181→185→188→192, BodyFat 1502.8→1508.2)** → `StopEating(Finished)` → `[Eat] released id=20`. Runtime value change udowodniony. Bez montażu (pierwszy PIE) meal wisiał → montaż = potwierdzony bite-clock.

## POZOSTAŁE DŁUGI (osobne, NIE blokują tej bramki)
- **Depletion gating:** NPC re-claimuje to samo `id=20` w nieskończoność — `ConsumePortion(Frac)` woła się 4×/meal, ale food nie znika (OnDepleted→Destroy NIE wpięte — w gate było „opcjonalnie"). Następny slice: bind `OnDepleted` + gating dostępności afordancji wg `RemainingPortion`.
- **Cancel-on-death w PIE:** ✅ UDOWODNIONE 2026-07-01. Zabity `BP_NPC_Character_C_1` w trakcie posiłku (montaż grał, `mealSize=0.2`) → `AbortTask` → `[Eat:C_1] StopEating(Interrupted)` (reason=Interrupted, nie Finished) + `[Eat] BP_NPC_Character_C_1 released id=18` (slot zwolniony, ta sama klatka [815]); sim leci dalej (`C_2 claimed id=13`), ZERO crasha. Także przy okazji potwierdzony EC-EAT-2: `StopEating(SourceGone)` gdy jedzenie zniszczone (depletion) w trakcie posiłku innego NPC.
- **Deprecjacja BP `BTTask_Eat.uasset`:** ✅ ZROBIONE 2026-07-01 (soft) — NIE była czystą sierotą: referował ją `BT_NPC` (osobne drzewo), a `BT_NPC` referują `BP_NPC_Character`+`BP_NPC_AI` przez MARTWY węzeł grafu (leftover po usuniętym Event OnPossess; żaden CDO NPC nie trzyma BT_NPC, w PIE odpala się tylko BT_Exploration). Zamiast ryzykownej kasacji (zerwałaby węzeł + gitignored) → **rename `BTTask_Eat`→`BTTask_Eat_DEPRECATED`** (kasuje kolizję nazw z klasą C++; redirector utrzymuje referencję; C++ task + BT_Exploration nietknięte). **Follow-up:** pełny cleanup martwego łańcucha `BT_NPC` (+ jego węzeł w BP) i twarda kasacja `BTTask_Eat_DEPRECATED` — osobny slice (dotyka grafów BP).
- **Montaż real-time pacing:** kęsy padają szybciej niż sugeruje rozstaw notify — kosmetyka (montaż napędza, liczba kęsów poprawna).
