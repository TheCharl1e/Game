# ARCHITECTURE.md — Stan_Pierwotny

Survival NPC simulation. Design spine: **C++ computes, Blueprint displays**.
C++ components carry the heavy state/logic (no `Tick`; timers + events); Blueprint
reacts via `BlueprintImplementableEvent` and reads cached values.

---

## 1. Maslow needs hierarchy (the NPC "brain")
`UMaslowBiologicalComponent` is the authoritative metabolic/needs model. A single
C++→Blackboard bridge service feeds the Blueprint Behavior Tree — nothing else
should duplicate this state.

**`EMaslowPriority`** (highest unmet need, written to BB as int):
| Value | Level | Meaning |
|---|---|---|
| 0 | `Level_0_FightOrFlight` | Panic — overrides everything |
| 1 | `Level_1_Temperature` | Physiological: temperature |
| 2 | `Level_1_Hydration` | Physiological: hydration |
| 3 | `Level_1_Rest` | Physiological: rest/sleep |
| 4 | `Level_1_Nutrition` | Physiological: hunger |
| 5 | `Level_2_Safety` | Safety (shelter/weapons) |
| 6 | `Satisfied` | All needs met (idle/social) |

**`EHungerPhase`** (layered catabolism): `Phase_0_Glucose` → `Phase_1_Glycogen`
→ `Phase_2_FatBurn` → `Phase_3_Autophagy` (HP/muscle) → `Phase_4_Death`.

**Bridge:** `UBTService_MaslowBlackboardSync` (`Source/Stan_Pierwotny/AI/`) — the
ONLY place C++ Maslow data enters the AI layer. Add to the BT_NPC root; writes
keys: MaslowPriority(int), Hydration%, Glucose%, HP%, IsInPanic, IsStarving.
It supersedes the legacy Blueprint `DaysOfHunger`/`DaysOfThirst` keys.

---

## 2. Body / sense damage model
`UBodyConditionComponent` (`Source/Stan_Pierwotny/Body/`). Sparse, cached, no Tick.

- **`EBodyPart`** — 26 parts in a containment hierarchy (Body → Head → eyes/ears/
  tongue; Torso → arms → hands → 5 fingers each; legs → feet).
- **Cascade rule:** `GetPartEffectiveHealth(part)` = own health capped by the
  whole parent chain. A 40%-functional arm caps its hand and fingers at 40%
  regardless of their own health.
- **`ESenseType`** — Vision, Hearing, Speech, HandPrecision (L/R), Mobility.
  Senses are a weighted average of effective health over contributing parts,
  **cached** and recomputed only on change (read by BT/EQS).
- **`EInjuryType`** — None, Wound, Fracture, Sprain, Burn, Sever (HUD + treatment).
- **Storage:** `PartHealth`/`PartInjury` are sparse maps — absent key = healthy.
- **Data:** optional shared `UBodyHierarchyAsset`; falls back to a built-in default.

---

## 3. Inventory + ownership
`UInventoryComponent` (`Source/Stan_Pierwotny/`). Pure logic, no Tick. Visuals via
`OnInventoryChanged`. **Per-container compartment model (Tarkov-style)** — see
DECISIONS.md (the old shared slot-pool model is retired).

- **`FStorageCompartment`** — a discrete storage space provided by the body
  (innate pockets, always index 0) or by an equipped container item. Removing the
  source gear removes the (empty-only) compartment.
- **`EEquipmentSlot`** — None/Pockets, LeftHand, RightHand, Head, Torso, Legs,
  Feet, Back (backpack), Rig (vest). Back/Rig/Torso/Legs items may open their own
  compartment (`ContainerCapacity`).
- **`FItemDefinition`** (DataTable row) — Type, weight, value, nutrition,
  perishable, stack, `SlotSize`, `ContainerCapacity`, `ValidEquipSlot`.
- **`EItemType`** — None, Food, Resource, Tool, Clothing, Luxury.
- **Ownership:** `OwnerID:int32`, `PUBLIC_OWNER_ID = -1` (village storehouse).
  `TryWithdraw(..., bWasUnauthorized)` feeds the L5 "detective"/theft system.

> ⚠️ A competing ownership model exists in `AItemBase::EItemOwnership`
> (Public/Private/PlayerOwned). Unify on `OwnerID` (see DECISIONS.md).

---

## 4. Debug UI widgets
`Content/DocelowaGra/UI/`:
- **`WBP_Tab_Body`** (`UI/Inventory/`) — per-part panel + senses strip.
  `PopulatePartRow(RowIndex, Part)` selects one of 7 rows via a comparator-driven
  `Select` mux; `RefreshSensesStrip` drives 6 progress bars + `Pct_*` labels from
  the cached senses.
- **`WBP_NPC_Inspector`** (`UI/Debug/`) — `SetInspectedNPC(NPC)` finds the
  `WBP_Tab_Body` via `Get All Widgets Of Class` → `[0]` → cast → `SetNPCRef(NPC)`.

---

## 5. Performance posture
Designed for ~500 NPCs: zero `Tick` in gameplay components; metabolism on a ~10 s
timer; senses cached & event-driven; `RemoveAtSwap` for O(1) removals; sparse maps
so healthy/empty state costs nothing.
