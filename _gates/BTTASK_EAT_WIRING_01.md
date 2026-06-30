# GATE: BTTASK-EAT-WIRING-01 — kanoniczny C++ BTTask_Eat (rich appetite ↔ afordancja)

Data: 2026-06-30 · Branch: `feat/bttask-eat-wiring` (od `game-58`) · Target: E:\Game_58 (UE 5.8)
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

## ⚠️ FLAGI — rozjazdy z kontraktem (kontrakt zakłada plumbing, którego NIE ma; proszę o GO / korektę)
- **F1 — `PlayEatMontage` BIE nie istnieje i nie ma C++ hosta na pawnie** (pawn = BP-only). Propozycja default: BIE `PlayEatMontage(AActor* FoodActor)` na **`ANPCAIControllerBase`** (C++; task ma `GetAIOwner()`), BP_NPC_AI implementuje i gra montaż na opętanym pawnie. Alternatywa: interfejs `IEatAnimation` na pawnie (wzór IConsumable). → potrzebny wybór.
- **F2 — mechanizm zakończenia taska latentnego nie istnieje.** ConsumeBite robi auto-`StopEating`, ale C++ task nie wie KIEDY skończyć węzeł BT. Propozycja default: dodać do Maslow multicast `FOnEatingStopped` broadcastowany w `StopEating`; task binduje → `FinishLatentTask(Succeeded)` + Release. → potwierdź podejście.
- **F3 — `NearestFoodObject` BB-key nie istnieje w C++.** Dziś task pyta afordancję wprost (zero BB-write). Propozycja default: `FBlackboardKeySelector NearestFoodObjectKey` na tasku, zapis `Owner` po Reserve. Wymaga, by BB_NPC miał Object-key. → potwierdź nazwę/obecność klucza.
- **F4 — źródło `FoodTable` (DT_FoodStats).** Dziś dostarcza je graf BP. C++ task potrzebuje `UPROPERTY UDataTable* FoodTable` (+ `int32 BiteCount`) ustawiane przez designera. → default: dodać property, FLAG do ustawienia w assecie taska.
- **F5 — brak sygnału significance** → stub `bIsHighSignificance=true` + TODO hook (zgodnie z DEFAULT).

## Definition of Done (FALSYFIKOWALNY)
1. Build editor-closed czysty.
2. PIE: NPC near → `Claim → StartEating → PlayEatMontage → ConsumeBite×N → StopEating(Full/Finished) → Release`; far → `SettleMealInstant → Release`. Twarde liczby z `Saved/Logs/` (osobna kategoria: Claim/Release/StartEating/Bite/Stop/SettleInstant). Cancel-on-death: NPC ginie w trakcie → slot zwolniony (log).
> Test zależy od SLEEP_FAINT_TRAP_01 (inaczej NPC mdleje z głodu snu przed turą jedzenia).

## STOP — nie ruszam (czeka na zatwierdzenie / wymaga edytora / koruptuje MCP)
- NIE edytuję `BT_Exploration` (przepięcie taska) — ręczne kliknięcia po zatwierdzeniu.
- NIE kasuję BP `BTTask_Eat.uasset`.
- NIE odpalam PIE.
