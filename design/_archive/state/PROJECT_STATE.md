# PROJECT_STATE.md — Stan_Pierwotny

> Dokumentacja techniczna stanu projektu dla architekta systemu.
> Wygenerowano: **2026-06-07**. Skan automatyczny katalogu `E:\Game`.

---

## 0. Metryki ogólne

| Pozycja | Wartość |
|---|---|
| Plik projektu | `E:\Game\Stan_Pierwotny.uproject` |
| Silnik | **Unreal Engine 5.7** (`EngineAssociation: 5.7`, `E:\UE_5.7`) |
| Moduł C++ | `Stan_Pierwotny` (Runtime, LoadingPhase Default) |
| Skompilowany DLL | `Binaries\Win64\UnrealEditor-Stan_Pierwotny.dll` — **build 2026-06-07 13:48** |
| Najnowszy plik źródłowy | 2026-06-01 00:27 |
| **Status kompilacji** | ✅ **KOMPILUJE SIĘ** — DLL nowszy niż wszystkie źródła, brak błędów C++ w logach |
| Liczba klas C++ (.h/.cpp) | 45 par (~90 plików) |
| Edytor podłączony (MCP) | ❌ offline (`editor_online:false`); plugin MCP online (port 8090), headless build dostępny |

### Wtyczki włączone (`.uproject`)
`ModelingToolsEditorMode`, **`LudusAI`**, `VisualStudioTools`, **`StateTree`**, **`GameplayStateTree`**, `PythonAutomationTest`.

> `LudusAI` generuje w logu ciągłe `LogHttp: Warning: Sleeping 0.500s to wait for 1 outstanding Http Requests` oraz timeout do `google.com/generate_204` — to plugin AI „dzwoniący do domu", nieszkodliwe, ale zaśmieca log.

### Zależności modułu (`Stan_Pierwotny.Build.cs`)
`Core`, `CoreUObject`, `Engine`, `InputCore`, `GameplayStateTreeModule`, `StateTreeModule`, `EnhancedInput`, `AIModule`, `UMG`, `Slate`, `SlateCore`.
Definicja: `ENABLE_FORMAT_STRING_SANITIZATION=0`. Ręcznie dopisane `PublicIncludePaths` dla wszystkich podkatalogów wariantów (workaround na nagłówki bez pełnych ścieżek).

---

## 0.1. Tabela statusu komponentów C++ (rdzeń „DocelowaGra")

> Szybkie odniesienie — sprawdź TUTAJ, zanim poprosisz o nowy system.
> Legenda kompilacji: ✅ zweryfikowane (zielony build) · ⏳ wymaga pełnego buildu
> (UHT) — Live Coding NIE wystarcza · ❌ błąd.

| # | Komponent | Wersja | Plik | Istnieje | Kompiluje | Publiczne API (skrót) |
|---|---|---|---|---|---|---|
| 1 | `UMaslowBiologicalComponent` | v1.1 | `Source/Stan_Pierwotny/MaslowBiologicalComponent.h` | ✅ | ✅ *(build 2026-06-07; nagłówek mógł się zmienić od tego czasu)* | `ProcessMetabolism()`, `ConsumeFood(P,F,S)`, `ConsumeFoodItem(row)`, `SetCurrentActionMultiplier()`, `SetActionByRow(name)`, `EvaluateCurrentNeed()→EMaslowPriority`, `CalculateTotalKcalBurnRate()`, `GetHP/Hydration/Glucose/Fat/Glycogen/HoursAwakePercent()` |
| 2 | `UInventoryComponent` | **v2.0** (per-kontener; v1 wspólna pula **wycofana**) | `Source/Stan_Pierwotny/InventoryComponent.h` | ✅ | ✅ *(2026-06-16 pełny build)* | `AddItem/RemoveItem`, `AddItemToCompartment/RemoveItemFromCompartment`, `EquipItem/UnequipSlot`, `GetQuantity/HasItem/GetTotalWeight/Capacity/UsedSlots`, `GetCompartments()`, `TryWithdraw(id,item,n,&bUnauthorized)` |
| 3 | `UBodyConditionComponent` | v1.0 | `Source/Stan_Pierwotny/Body/BodyConditionComponent.h` | ✅ *(przeniesiony do modułu 2026-06-09)* | ✅ *(2026-06-16 pełny build)* | `GetPartHealth/GetPartEffectiveHealth/GetPartInjury(part)`, `GetVision/Hearing/SpeechAcuity()`, `GetHandPrecision(bRight)`, `GetMobility()`, `ApplyDamage(part,amt,injury)`, `HealPart(part,amt)`, `GetPartDisplayName(part)→FText` *(static, 2026-06-16)* |
| 4 | `UBTService_MaslowBlackboardSync` | v1.0 | `Source/Stan_Pierwotny/AI/BTService_MaslowBlackboardSync.h` | ✅ *(nowy od 2026-06-07)* | ✅ *(2026-06-16 pełny build)* | most C++→BB; klucze: MaslowPriority(int), Hydration%, Glucose%, HP%, IsInPanic, IsStarving (bindowane w edytorze BT) |

**Pomocnicze klasy własne** (nie-komponenty): `AItemBase` (przedmiot na mapie,
⚠️ konkurencyjny model własności), `ACorpseBase` (zwłoki/mięso). Patrz §2.1.

## 0.2. Status buildu C++ — 🟢 ZIELONY (rozwiązane 2026-06-16)

> **ROZWIĄZANE:** pełny `Build.bat` (edytor zamknięty) → **`Result: Succeeded`** (89 s).
> UHT zregenerował 98 nagłówków, moduł zlinkowany: `UnrealEditor-Stan_Pierwotny.dll`.
> Wszystkie 4 komponenty zielone. Wymagało **wymuszenia UHT**: usunięcie
> `Intermediate/Build/.../Inc/Stan_Pierwotny` + `Makefile.bin` + `*.uhtmanifest`,
> bo Live Coding zatruł makefile (logował „Generated code is up to date" i pomijał UHT).
> Patrz CLAUDE.md reguła 6. Poniżej zachowano oryginalny opis błędu (historyczny).

### (Historyczne, 2026-06-12) Czerwona próba — Live Coding przez MCP

Ostatnia próba (Live Coding przez MCP) **nie powiodła się** na `InventoryComponent`:
```
InventoryComponent.h(110,2): error C4430 ... GENERATED_BODY()   (FStorageCompartment)
InventoryComponent.h(135,26): error C2143: syntax error before UInventoryComponent
InventoryComponent.cpp(13,9): error C2039: 'BeginPlay' is not a member of 'UObject'
```
**To NIE jest błąd w kodzie.** Rewrite Inventory (nowy `USTRUCT`, nowe `UPROPERTY`,
zmiana klasy) to zmiana strukturalna nagłówka. Live Coding loguje „Generated code
is up to date" i **nie uruchamia UHT** — kompiluje nowy `.cpp` ze starym
`generated.h`. **Naprawa:** zamknij edytor UE → pełny `Build.bat`
(`Stan_PierwotnyEditor`). Komponenty #2/#3/#4 zzielenieją dopiero po tym buildzie
(ROADMAP T5/T6).

---

## 1. Pełna struktura `Source/` z listą klas C++

```
Source/Stan_Pierwotny/
│
├── Stan_Pierwotny.Build.cs            [konfiguracja modułu]
├── Stan_Pierwotny.h / .cpp            [IMPLEMENT_PRIMARY_GAME_MODULE — punkt wejścia modułu]
│
├── ===== KOD WŁASNY (rdzeń „DocelowaGra") =====
├── MaslowBiologicalComponent.h/.cpp   [UActorComponent] — metabolizm/biologia NPC
├── InventoryComponent.h/.cpp          [UActorComponent] — ekwipunek + własność + kradzież
├── ItemBase.h/.cpp                    [AActor]          — przedmiot na mapie + własność
├── CorpseBase.h/.cpp                  [AActor]          — zwłoki jako źródło mięsa
│
└── TP_ThirdPerson/  ===== SZABLON EPIC „Third Person" (3 warianty) =====
    ├── TP_ThirdPerson.h/.cpp
    ├── TP_ThirdPersonCharacter.h/.cpp
    ├── TP_ThirdPersonGameMode.h/.cpp
    ├── TP_ThirdPersonPlayerController.h/.cpp
    │
    ├── Variant_Combat/
    │   ├── CombatCharacter.cpp (+.h)
    │   ├── CombatGameMode.cpp (+.h)
    │   ├── CombatPlayerController.cpp (+.h)
    │   ├── AI/
    │   │   ├── CombatAIController.cpp (+.h)
    │   │   ├── CombatEnemy.cpp (+.h)
    │   │   ├── CombatEnemySpawner.cpp (+.h)
    │   │   ├── CombatStateTreeUtility.cpp (+.h)
    │   │   ├── EnvQueryContext_Danger.cpp (+.h)
    │   │   └── EnvQueryContext_Player.cpp (+.h)
    │   ├── Animation/
    │   │   ├── AnimNotify_CheckChargedAttack.cpp (+.h)
    │   │   ├── AnimNotify_CheckCombo.cpp (+.h)
    │   │   └── AnimNotify_DoAttackTrace.cpp (+.h)
    │   ├── Gameplay/
    │   │   ├── CombatActivationVolume.cpp (+.h)
    │   │   ├── CombatCheckpointVolume.cpp (+.h)
    │   │   ├── CombatDamageableBox.cpp (+.h)
    │   │   ├── CombatDummy.cpp (+.h)
    │   │   └── CombatLavaFloor.cpp (+.h)
    │   ├── Interfaces/
    │   │   ├── CombatActivatable.cpp (+.h)
    │   │   ├── CombatAttacker.cpp (+.h)
    │   │   └── CombatDamageable.cpp (+.h)
    │   └── UI/
    │       └── CombatLifeBar.cpp (+.h)
    │
    ├── Variant_Platforming/
    │   ├── PlatformingCharacter.cpp (+.h)
    │   ├── PlatformingGameMode.cpp (+.h)
    │   ├── PlatformingPlayerController.cpp (+.h)
    │   └── Animation/AnimNotify_EndDash.cpp (+.h)
    │
    └── Variant_SideScrolling/
        ├── SideScrollingCharacter.cpp (+.h)
        ├── SideScrollingGameMode.cpp (+.h)
        ├── SideScrollingPlayerController.cpp (+.h)
        ├── SideScrollingCameraManager.cpp (+.h)
        ├── AI/
        │   ├── SideScrollingAIController.cpp (+.h)
        │   ├── SideScrollingNPC.cpp (+.h)
        │   └── SideScrollingStateTreeUtility.cpp (+.h)
        ├── Gameplay/
        │   ├── SideScrollingJumpPad.cpp (+.h)
        │   ├── SideScrollingMovingPlatform.cpp (+.h)
        │   ├── SideScrollingPickup.cpp (+.h)
        │   └── SideScrollingSoftPlatform.cpp (+.h)
        ├── Interfaces/SideScrollingInteractable.cpp (+.h)
        └── UI/SideScrollingUI.cpp (+.h)
```

**Podział własnościowy:**
- **Kod własny (4 klasy):** `MaslowBiologicalComponent`, `InventoryComponent`, `ItemBase`, `CorpseBase` — to jest rzeczywisty rdzeń projektowanej gry survivalowej.
- **Szablon Epic (41 klas):** całe `TP_ThirdPerson/` + 3 warianty. To niezmodyfikowany boilerplate z template'u „Third Person". Kompiluje się, ale **nie jest używany przez docelową rozgrywkę** (gra działa na `BP_SoulslikeGameMode`, nie na żadnym z GameMode'ów szablonu).

---

## 2. Stan każdej klasy (kompilacja / błędy / funkcja)

### 2.1. Klasy własne (rdzeń gry)

#### `UMaslowBiologicalComponent` — ✅ kompiluje się, ⚠️ niezintegrowany z AI
Komponent metabolizmu NPC, zaprojektowany pod **500 NPC bez ticka** (timer co 10 s zamiast `Tick`).
- **Struktury:** `FActionCostRow` (mnożniki spalania kcal/wody per akcja — pod `DT_ActionCost`), `FFoodItemRow` (makroskładniki, szanse zatrucia/halucynacji, flaga morale — pod `DT_FoodStats`).
- **Enumy:** `EHungerPhase` (5 faz: Glukoza→Glikogen→Tłuszcz→Autofagia→Śmierć), `EMaslowPriority` (7 poziomów piramidy Maslowa).
- **Logika:** `ProcessMetabolism()` — katabolizm warstwowy (glukoza→glikogen→tłuszcz→białko/HP), spalanie wody, mnożnik temperatury (<10°C = x2), zatrucie (woda x3). `ConsumeFoodItem()` losuje zatrucie/halucynacje. `EvaluateCurrentNeed()` — „sędzia Maslowa" zwracający priorytet.
- **Eventy BP:** `OnFoodConsumed`, `OnStarvedToDeath`. Helpery UI: `GetHPPercent/GetHydrationPercent/GetGlucosePercent/GetFatPercent`.
- ⚠️ **Problem 1:** Struktura `FActionCostRow` zdefiniowana, ale **C++ nigdy nie czyta `DT_ActionCost`** — `CurrentActionKcalMultiplier` ustawia się tylko ręcznie przez `SetCurrentActionMultiplier()`. DataTable `DT_ActionCosts.uasset` istnieje, ale nie jest wpięty w kod.
- ⚠️ **Problem 2:** `FFoodItemRow`/`DT_FoodStats` — **brak pliku `DT_FoodStats.uasset`** w `Content/`. Struktura gotowa, tabela nie istnieje.
- ⚠️ **Problem 3(architektura):** komponent **nie jest wywoływany przez Behavior Tree** (patrz §3). AI używa własnego, prostego systemu `DaysOfHunger/DaysOfThirst` (Blueprint), a nie systemu Glucose/Glycogen/Maslow z C++. Dwa równoległe, niezsynchronizowane modele potrzeb.

#### `UInventoryComponent` — ✅ kompiluje się, ⚠️ brak DataTable, niezintegrowany
Ekwipunek data-driven z modelem własności i wykrywaniem kradzieży (warstwa „detektywistyczna" L5).
- **Struktury/enumy:** `EItemType` (None/Food/Resource/Tool/Clothing/Luxury), `FItemDefinition` (waga, wartość, odżywczość, perishable, stack — wiersz DataTable), `FItemStack` (ID+ilość, runtime).
- **API:** `AddItem/RemoveItem/TransferTo/TryWithdraw` (z flagą `bWasUnauthorized` = kradzież), zapytania `GetQuantity/HasItem/GetTotalWeight`. Własna kategoria logów `LogInventory`. `PUBLIC_OWNER_ID = -1` dla wspólnego magazynu wioski.
- Dobre praktyki: brak ticka, `RemoveAtSwap` O(1), refund przy nieudanym transferze, ostrzeżenia w logu.
- ⚠️ **Problem:** wymaga `ItemDefinitions` (UDataTable) — **żadna tabela definicji przedmiotów nie istnieje w `Content/`**. `BeginPlay()` sam loguje ostrzeżenie, gdy tabeli brak. Bez niej `AddItem` odrzuca każdy przedmiot (brak definicji = 0 dodanych).
- ⚠️ Brak referencji do tego komponentu w jakimkolwiek BTTask (patrz §3) — system własności/kradzieży **nie jest podpięty pod AI**.

#### `AItemBase` — ✅ kompiluje się, ⚠️ konkurencyjny model własności
Aktor przedmiotu na mapie (jabłko/mięso/jagody). Mesh statyczny, brak ticka.
- **Enum własny:** `EItemOwnership` (Public/Private/PlayerOwned) — **inny niż** model `OwnerID:int32` w `UInventoryComponent`. Dwa niespójne systemy własności w jednym projekcie.
- **API:** `SetOwnership`, `CanBeEatenBy(NPC)`, `IsStolenBy(NPC)`. `FoodTableRowName` wskazuje na `DT_FoodStats` (która nie istnieje — patrz wyżej).

#### `ACorpseBase` — ✅ kompiluje się
Zwłoki jako źródło mięsa/kalorii. SkeletalMesh, fizyka domyślnie OFF, brak ticka.
- Timer jednorazowy `ProcessRotting` (domyślnie 1200 s = „doba w grze") → `OnCorpseRotten`. `ExtractMeat(kcal)` ujmuje z `AvailableKcal` (start 1500) → `OnCorpseDepleted`.
- Klasa kompletna i samowystarczalna. Brak wykrytych problemów; jak reszta — nie wiadomo, czy podpięta pod AI (zależy od BP).

#### `Stan_Pierwotny.h/.cpp` — ✅ standardowy plik modułu (`IMPLEMENT_PRIMARY_GAME_MODULE`). Bez zmian.

### 2.2. Klasy szablonu Epic (`TP_ThirdPerson/` + warianty) — ✅ wszystkie kompilują się

To **niezmodyfikowany kod template'u Epic „Third Person"**. Wszystkie 41 klas są częścią DLL i kompilują się bez błędów. Skrótowo wg funkcji:

| Grupa | Klasy | Funkcja |
|---|---|---|
| Bazowy | `TP_ThirdPersonCharacter`, `...GameMode`, `...PlayerController` | postać 3rd-person z Enhanced Input |
| Combat | `CombatCharacter`, combo/charged attack, `CombatEnemy(+Spawner+AIController)`, `CombatStateTreeUtility`, `EnvQueryContext_Player/Danger`, volumes, `CombatLifeBar`, interfejsy `CombatDamageable/Attacker/Activatable` | wariant walki z StateTree + EQS |
| Platforming | `PlatformingCharacter` (dash), `AnimNotify_EndDash` | wariant platformowy |
| SideScrolling | `SideScrollingCharacter`, `...CameraManager`, `SideScrollingNPC(+AIController)`, `SideScrollingStateTreeUtility`, jump pad / moving platform / pickup / soft platform, `SideScrollingUI`, `SideScrollingInteractable` | wariant 2.5D |

> **Ocena:** martwy kod względem docelowej gry. Kandydat do usunięcia po potwierdzeniu, że żaden Blueprint nie dziedziczy z tych klas. Pozostawienie nie szkodzi kompilacji, ale myli architekturę (3 dodatkowe GameMode'y, 2 systemy AI w C++, których gra nie używa).

---

## 3. Behavior Trees, Blackboardy i EQS

> Cały gameplayowy AI NPC jest **Blueprintowy** (`BTTask_BlueprintBase`, `AIController` BP). Komponenty C++ (`Maslow`, `Inventory`) **nie są referencjonowane** w żadnym BTTask ani BT. To główna luka integracyjna.

### 3.1. Folder `Content/DocelowaGra/AI_NPC/` (główny NPC survivalowy)

**Behavior Tree:**
- **`BT_NPC`** — drzewo główne. Blackboard: `BB_NPC`. Kompozyty: Selector / Sequence / Decorator_Blackboard.
  Wywoływane zadania: `BTTask_Check`, `BTTask_Drink`, `BTTask_Eat`, **`BTTask_FindFood_Experyment`**, **`BTTask_FindWater_Experyment`**, `BTTask_MoveTo`.

**Blackboard `BB_NPC` — klucze:**
| Klucz | Typ | Uwaga |
|---|---|---|
| `CurrentNeed` | Enum (`E_NeedState`) | aktualna potrzeba |
| `DaysOfHunger` | Float | ⚠️ równoległy do C++ `Glucose/Glycogen` |
| `DaysOfThirst` | Float | ⚠️ równoległy do C++ `CurrentHydration` |
| `NearstFoodLocation` | Vector | literówka „Nearst" (powiela się w nazwach) |
| `NearstFoodObject` | Object | |
| `NearstWaterLocation` | Vector | |
| `NearstWaterObject` | Object | |
| `SelfActor` | Object | |
| `TargetActor` | Object | |
| `TargetSearchPoint` | Object/Vector | |
| `TargetVector` | Vector | |

**Zadania BT (wszystkie Blueprint):** `BTTask_Check`, `BTTask_Drink`, `BTTask_Eat`, `BTTask_FindFood`, `BTTask_FindFood_Experyment`, `BTTask_FindWater_Experyment`, `BTTask_Searching`.

**EQS w AI_NPC:** `EQS_TEST.uasset` i `EQC_SearchZone.uasset` — to **ObjectRedirectory** (puste przekierowania), nie realne assety.

**Enum:** `E_NeedState` — UserDefinedEnum. ⚠️ Enumeratory mają domyślne nazwy silnika (`NewEnumerator0/1/16..21`) — nienazwane porządnie. DisplayName m.in.: `Hunger`, `Thirst`, `SelfActualization` (model Maslowa po stronie BP, niezależny od C++ `EMaslowPriority`).

**Struktury (UserDefinedStruct):** `ST_CharacterStats`, `S_NPC_Biometrics`. (Prefiks `ST_` = struct, nie StateTree.)

**Blueprinty:** `BP_NPC_Character` (postać), `BP_NPC_AI` (AIController — `UseBlackboard(BB_NPC)` + uruchamia `BT_NPC`), `BP_DayNightCycle`.

**Podfolder `CharacterRandomizer/`:** `BP_CharacterRandomizerComponent` + enumy `E_Plec`, `E_ImieMeskie`, `E_ImieKobieta`, `E_Wlosy`, `E_KolorOczu`, `E_ZnakiSzczegolne` — losowanie wyglądu/imienia NPC.

### 3.2. Folder `Content/DocelowaGra/EQS/` (oddzielny system szukania stref)

Drugi, niezależny system AI (szukanie/eksploracja stref):
- **Behavior Tree:** `BT_NPC_Search`.
- **Environment Queries (EQS):** `EQS_ZoneSearch`, `EQS_ZoneSearch_Pawn`, `EQS_See`. (+ `DocelowaGra/EQS_ZoneSearch` na poziomie wyżej — możliwy duplikat.)
- **EQS Contexts (EQC):** `EQC_Zone_Location`, `EQC_Zone_Location_Vector`, `EQC_NPC_Location`, `EQC_NPC_Current_Location`, `EQC_NPC_Current_Location_Vector`.
- **Zadania BT:** `BTTask_Mark`, `BTTask_Test`, `BTTask_FilterVisted` (literówka „Visted").
- **BP:** `BP_SearchMark`.

### 3.3. Folder `Content/DocelowaGra/MOBA/` (osobny prototyp — tower defense/MOBA)

Kompletnie inny gatunek, własny stos AI:
- **Behavior Tree:** `BT_Enemy`. **Blackboard:** `BB_Enemy`. **Zadania:** `BTTask_Attack`.
- BP: `BP_Forteca`, `BP_Ludzik`, `BP_Enemy`, `BP_Spawner`, `BP_EnemySpawner`, `WBP_SpawnerDisplay`. Struktury: `ST_WaveConfig`, `ST_WaveNumberOfEnemy`.

### 3.4. Pozostałe foldery `DocelowaGra/`
- `DANGER/BP_NPC_WOLF`, `BP_NPC_DEER` — zwierzęta (drapieżnik/zwierzyna).
- `FOOD/BP_FoodSpawner`, `BP_Food` — spawnery jedzenia.
- `SHELTER/BP_Zone`, materiały stref. 
- `Gamemode/` — **`BP_SoulslikeGameMode`** (faktyczny GameMode gry), postać Soulslike (`SKM_SoulslikeCharacter`, `ABP_Soulslike`, `BS_WalkRun`, ataki), Enhanced Input (`IMC_Soulslike`, IA_*), `WBP_HUD`.
- `Characters/Mannequins/` — standardowe mannequiny UE5 + `ABP_Manny`.
- Globalne: `E_Season`, `ST_Time`, `ST_Season`, `BP_SoulslikeGameMode`.

---

## 4. Aktualne błędy buildu

**Brak błędów kompilacji C++.** Projekt buduje się poprawnie:
- DLL `UnrealEditor-Stan_Pierwotny.dll` ma datę **2026-06-07 13:48**, nowszą niż każdy plik źródłowy (najnowszy 2026-06-01).
- W `Saved/Logs/Stan_Pierwotny.log` brak wpisów `LogCompile: Error`, brak `Fatal`, brak błędów linkera.

Wpisy z logu, które **nie są** błędami buildu (dla porządku):
- `LogWindows: Failed to load 'aqProf.dll' / 'VtuneApi*.dll' / 'WinPixGpuCapturer.dll' / 'Wintab32.dll'` — brak opcjonalnych narzędzi profilujących (Intel VTune, PIX, tablet Wintab). Nieszkodliwe.
- `LogHttp: Warning: Sleeping 0.500s ...` (powtarzane setki razy) + timeout `google.com/generate_204` — aktywność pluginu **LudusAI**. Nieszkodliwe, ale zaśmieca log.
- `LogStreaming: Warning: Failed to read file '...VisionOS...png'` — brakujące ikony platformy VisionOS w silniku. Kosmetyczne.
- `LogAutomationTest: Error: Condition failed` (×6) — nieudane testy automatyczne (`PythonAutomationTest`), nie build. Wymaga osobnej diagnozy, jeśli testy są istotne.

> Weryfikacja headless build jest możliwa (MCP: `headless_build` dostępny, edytor offline). Jeśli architekt chce twardego potwierdzenia „od zera", można uruchomić `build_project` — obecne dane (DLL > źródła) już to potwierdzają.

---

## 5. Co jest „zepsute" lub niekompletne (wg struktury kodu)

### 🔴 Krytyczne / architektoniczne
1. **Rozłączone systemy potrzeb (C++ ↔ AI).** Bogaty model biologiczny w `MaslowBiologicalComponent` (Glucose/Glycogen/BodyFat/HP, fazy głodu, Maslow) **nie jest używany przez Behavior Tree**. BT operuje na prostym, BP-owym `DaysOfHunger/DaysOfThirst` w `BB_NPC` i enumie `E_NeedState`. Dwa równoległe modele, brak mostu C++→BB. To największy dług architektoniczny.
2. **`InventoryComponent` całkowicie odpięty.** System ekwipunku/własności/kradzieży (L5 „detektyw") nie jest wołany w żadnym BTTask. Brak też wymaganej `UDataTable` z `FItemDefinition` — bez niej `AddItem` odrzuca wszystko.
3. **Dwa niespójne modele własności przedmiotów.** `AItemBase::EItemOwnership` (Public/Private/PlayerOwned, oparte o `AActor* CurrentOwner`) vs `UInventoryComponent::OwnerID` (int32, `PUBLIC_OWNER_ID=-1`). Nie ma wspólnego kontraktu — trzeba ujednolicić.

### 🟠 Brakujące dane (DataTables)
4. **`DT_FoodStats` nie istnieje.** Referencjonowana przez `FFoodItemRow` (Maslow) i `AItemBase::FoodTableRowName`. W `Content/` jest tylko `DT_ActionCosts.uasset`.
5. **`DT_ActionCost` istnieje, ale niewpięta.** `FActionCostRow` zdefiniowana, jednak C++ nigdy nie czyta tabeli — mnożniki ustawiane wyłącznie ręcznie.
6. **Brak DataTable definicji przedmiotów** dla `UInventoryComponent.ItemDefinitions`.

### 🟡 Bałagan w assetach / WIP
7. **Duplikaty „_Experyment" w BT.** `BT_NPC` używa `BTTask_FindFood_Experyment` i `BTTask_FindWater_Experyment`, podczas gdy „czyste" `BTTask_FindFood` (i `BTTask_Searching`) są **osierocone** — `BTTask_FindFood` zawiera w środku resztki grafu `ExecuteUbergraph_BTTask_FindFood_Experyment1` (kopia eksperymentu). Do konsolidacji: zostawić jedną wersję, usunąć duplikaty.
8. **Martwe ObjectRedirectory.** `AI_NPC/EQS_TEST` i `AI_NPC/EQC_SearchZone` to redirectory wskazujące na `/Game/DocelowaGra/EQS/...`, gdzie assety o tych nazwach **już nie istnieją** (po kolejnym przeniesieniu) — przekierowania wiszą w próżni. Do usunięcia („Fix Up Redirectors").
9. **Literówki w nazwach kluczy/assetów** (utrwalają się w referencjach): `Nearst`(Food/Water)`Location/Object`, `BTTask_FilterVisted`. Zmiana nazwy teraz wymaga naprawy referencji.
10. **`E_NeedState` z domyślnymi nazwami enumeratorów** (`NewEnumerator0/1/16..21`) — nieczytelne, podatne na błędy przy zmianie kolejności.

### 🟡 Wielogatunkowy „śmietnik" prototypów (rozmycie zakresu)
11. Projekt zawiera **kilka rozłącznych prototypów** w jednym repo:
    - `DocelowaGra/AI_NPC` + `EQS` + `FOOD`/`DANGER`/`SHELTER` → docelowa **gra survivalowa z symulacją NPC**.
    - `DocelowaGra/MOBA` → osobny prototyp tower-defense/MOBA (własne BT/BB/spawner/fale).
    - `DocelowaGra/Gamemode` → postać **Soulslike** (faktyczny `BP_SoulslikeGameMode`).
    - `Source/.../TP_ThirdPerson` + 3 warianty → **niewykorzystany szablon Epic** (41 klas C++).
    - `Content/Free_Temper/TopDown` → resztki szablonu Top-Down (mapa `TopDownMap`, `BP_TopDownGameMode/Controller`).
    - `Content/Dodatki/Clementine...` → pojedynczy asset Megascan/Fab.
    Brak jednoznacznego „głównego" wektora — utrudnia ocenę, co jest produkcyjne, a co eksperymentem.

### ✅ Co jest solidne
- Wszystkie 4 klasy własne **kompilują się** i są poprawnie zaprojektowane pod wydajność (zero `Tick`, timery, `RemoveAtSwap`, clampy, fail-safe’y, dedykowane logi).
- Architektura „C++ liczy, Blueprint pokazuje" jest spójna (`BlueprintImplementableEvent` na warstwę wizualną/UI).
- Build i binaria są aktualne; pipeline AI (BT/BB/EQS/AIController) istnieje i jest spięty po stronie Blueprint.

---

## 6. Rekomendacje (kolejność dla architekta)
1. **Most C++→Blackboard:** w `BP_NPC_AI`/serwisie BT czytać `MaslowBiologicalComponent` i wypełniać `BB_NPC` (zamiast osobnego `DaysOfHunger`). Zlikwidować duplikację modelu potrzeb.
2. **Utworzyć brakujące DataTables:** `DT_FoodStats` (`FFoodItemRow`), tabelę `FItemDefinition`, oraz wpiąć `DT_ActionCosts` do `MaslowBiologicalComponent`.
3. **Ujednolicić własność** przedmiotów: jeden model (rekomendacja: `OwnerID` z `InventoryComponent`), `AItemBase` jako cienka warstwa wizualna.
4. **Sprzątanie assetów:** Fix Up Redirectors, usunąć osierocone `BTTask_FindFood`/`BTTask_Searching`/duplikaty `_Experyment`, poprawić literówki kluczy BB, nazwać enumeratory `E_NeedState`.
5. **Decyzja o zakresie:** wydzielić/usunąć prototypy MOBA, TopDown i szablon `TP_ThirdPerson` jeśli nie wchodzą do docelowej gry — odchudzi build i mapę zależności.
```
