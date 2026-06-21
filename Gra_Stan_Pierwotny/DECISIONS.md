# DECISIONS.md — mechanical / design decision log

Append-only. Newest at top. Each entry: what was decided, why, consequences.

---

### 2026-06-16 — Build gotcha: Live Coding poisons the UBT makefile
After a **structural** header change (new `USTRUCT`/`UPROPERTY`/`UCLASS`), a Live
Coding compile marks the header "processed" in UBT's `Makefile.bin` **without ever
running UHT**. A subsequent full `Build.bat` then logs *"Generated code is up to
date"*, skips UHT, and compiles against the stale `generated.h` → cascade of
`GENERATED_BODY()` "missing type specifier" + `BeginPlay is not a member of UObject`.
- **Fix (force UHT):** delete `Intermediate/Build/Win64/UnrealEditor/Inc/<Module>`,
  `Intermediate/Build/Win64/x64/<Target>/Development/Makefile.bin`, and
  `Intermediate/Build/Win64/<Target>/Development/*.uhtmanifest`, then build.
- Confirmed 2026-06-16: after forcing UHT → `Result: Succeeded`, all 4 components green.
- **Rule:** never trust Live Coding for structural changes; full build, editor closed.

### 2026-06-09 — Inventory: shared slot-pool RETIRED → per-container compartments
The old model (one flat pool of slots shared across all gear) is **withdrawn**.
New model: each container (innate body pockets at index 0, plus backpack/rig/
clothing) is its own `FStorageCompartment` with independent capacity.
- **Why:** Tarkov-style per-container storage; HUD renders separate grids;
  losing a bag should drop only its contents, not reshuffle a global pool.
- **Consequences:** new `USTRUCT FStorageCompartment`, `EEquipmentSlot`,
  `AddItem`/`RemoveItem` auto-distribute, plus targeted `*ToCompartment` APIs.
  This is a **structural header change** → needs a full UHT build, not Live Coding.

### 2026-06-09 — Body model: 26 parts with containment cascade
`EBodyPart` is a 26-node containment tree; `GetPartEffectiveHealth` caps a part by
its entire parent chain (a damaged arm caps its hand/fingers).
- **Why:** realistic injury propagation without per-part special-casing.
- **Consequences:** senses derive from *effective* health, so one upstream wound
  degrades everything downstream consistently.

### 2026-06-09 — Body state is sparse + senses cached, no Tick
Only damaged parts are stored (absent = 100%); derived senses recomputed only on
change, never per frame.
- **Why:** ~500 NPCs; a healthy NPC must cost ~nothing.

### 2026-06-09 — Maslow is authoritative; one C++→BB bridge
`UMaslowBiologicalComponent` owns needs/metabolism. `UBTService_MaslowBlackboardSync`
is the **single** path into the Behavior Tree Blackboard.
- **Why:** kill the duplicate Blueprint `DaysOfHunger`/`DaysOfThirst` model that
  ran in parallel and could desync from the C++ simulation.
- **Catabolism order:** Glucose → Glycogen → Fat → Autophagy(HP) → Death
  (`EHungerPhase`). Priority resolution via `EMaslowPriority` (panic overrides all).

### 2026-06-09 — File org: C++ under `Source/Stan_Pierwotny/<System>/`
`BodyConditionComponent` moved from repo root into `Source/Stan_Pierwotny/Body/`;
AI services under `Source/Stan_Pierwotny/AI/`.
- **Why:** root files are outside the module and never compiled. Subfolders keep
  systems separated; UBT globs them automatically.

### OPEN — Unify item ownership
Two models coexist: `UInventoryComponent::OwnerID` (int, `-1`=public) vs
`AItemBase::EItemOwnership` (enum). **Decision pending:** standardize on `OwnerID`,
make `AItemBase` a thin visual layer. (Tracked in PROJECT_STATE §5/ROADMAP.)
