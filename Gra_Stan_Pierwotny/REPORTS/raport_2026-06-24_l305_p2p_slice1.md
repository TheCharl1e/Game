# L3-05 P2P slice 1 (Tier 1 — wirtualny barter) — build + verify rdzenia

Data: 2026-06-24
Branch: `feat/l3-05-p2p-slice1` (commity 65e1be6, a28bd54)
Build: Stan_PierwotnyEditor Win64 Development, **edytor zamknięty (Build.bat/UHT) — Succeeded** (2×).

## Co zbudowane
- `Social/ContractTypes.h` — `EContractStatus` (Open/Fulfilled/Cancelled) + `FContract`
  (PosterID, OfferItem+amount, WantType = **kategoria** EItemType, AccepterID).
- `Social/ContractPoolSubsystem.{h,cpp}` — `UWorldSubsystem` (wzorzec jak NPCRegistry):
  `PostContract` (waliduje że poster realnie trzyma ofertę), `GetOpenContracts`,
  `HasOpenContractFrom`, `CancelContract`, **`AcceptAndFulfill`** = wirtualny swap
  (RemoveItem+AddItem obustronnie, **roll-back** przy częściowym pobraniu, self-cancel
  martwego listingu). Inwentarze rozwiązywane przez `UNPCRegistrySubsystem->GetNPCById`.
  Mutatory wystawione `BlueprintCallable` (debug-HUD/BP/verify).
- `Social/ContractTraderComponent.{h,cpp}` — driver per-NPC, timer (bez Tick): czyta
  potrzeby Maslow zimno→Clothing + głód→Food (kolejność drabiny), najpierw akceptuje
  kontrakt którego oferta jest typu jaki chce I ma czym zapłacić (double-coincidence),
  inaczej wystawia nadwyżkę.
- `InventoryComponent` — helpery barteru: `GetItemTypeForID`, `FindFirstHeldItemOfType`,
  `FindFirstHeldItemNotOfType`.
- `NPCIdentityComponent` — wpięte `Inventory.OwnerID = NPCId` przy rejestracji.

## VERIFY — rdzeń (live PIE, CaldrethMap, 2 NPC)

**OwnerID wiring** (live odczyt obiektów):
- NPC#1: NPCId=1, Inventory.OwnerID=**1**
- NPC#2: NPCId=2, Inventory.OwnerID=**2**

**Swap** (live `PostContract`+`AcceptAndFulfill`, odczyt qty przed/po):

| | FurCoat | Berries |
|---|---|---|
| BEFORE A(id1) | 1 | 0 |
| BEFORE B(id2) | 0 | 1 |
| AFTER A(id1) | **0** | **1** |
| AFTER B(id2) | **1** | **0** |

- `PostContract` → cid 1, open=1, kontrakt {poster 1, offer FurCoat, wantType FOOD, status OPEN}
- `AcceptAndFulfill(1, accepter=2)` → **True**; open after=0; `HasOpenContractFrom(1)=False` (→ Fulfilled)
- Log C++: `[P2P] PostContract #1: NPC 1 offers 1×FurCoat, wants 1×EItemType::Food`
  oraz `[P2P] FULFILLED #1: NPC 1 gave 1×FurCoat → NPC 2 gave 1×Berries`

Rdzeń (pula + cykl kontraktu Open→Fulfilled + wirtualny swap + rozwiązanie po NPCId
+ OwnerID) — w pełni zweryfikowany twardymi liczbami z live obiektów i logu.

## NIEZWERYFIKOWANE — pętla autonomiczna drivera (blocker)
`UContractTraderComponent` nie jest jeszcze na `BP_NPC_Character`, więc nie ma jeszcze
NPC który handluje SAM z potrzeby. Próba dodania komponentu w runtime (RF_Transient-safe)
zablokowana: Python tego buildu nie wystawia `add_component_by_class`/`register_component`.
Potrzeby wymuszone poprawnie (A: Glucose 100<EffKcalThr 600 → głodny, ciepły; B: 900>600
syty, CurrentTemp 36.6<ComfortMin 50 → zmarznięty), ale bez komponentu timer nie ruszy.

**Wymaga decyzji dyrektora:** dodać `UContractTraderComponent` do `BP_NPC_Character`
(strukturalna zmiana defaultów BP → Twój Ctrl+S, nie zapis ścieżką silnikową), potem
świeże PIE zweryfikuje autonomiczny obrót na timerze (NPC sam wystawia+akceptuje z potrzeby).

## Odłożone (slice 2+)
Fizyczna realizacja/MoveTo, ceny/negocjacje, reputacja, zonal pool, pełna generalizacja
need→item, persystencja/blockchain (L3-04). Flaga perf: skan puli O(open) na trader/kadencję
— OK slice 1, zone-scope+event-driven = slice 2.
