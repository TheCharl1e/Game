# Stan_Pierwotny (Caldreth) — Claude Code operating rules

> This file auto-loads at the start of every Claude Code session (and after `/clear`).
> You are the EXECUTOR. The director (Szymon) and Chat-Claude (ARCHITECT) hand you approved
> design gates; you implement, verify with hard numbers, and report. You do NOT design new
> mechanics or proceed past a gate without explicit approval.

## What this project is
UE5 (5.7.x) survival simulation: 500–1000+ autonomous NPCs on a full Maslow hierarchy.
Emergent behavior from biology + geography + scarcity. NO LLM per NPC — hard C++ math, timers, BT, EQS.
World: volcanic island **Caldreth** (concentric geological zones).

## Where code lives
- Game module: `Source/Stan_Pierwotny/`
- Map generation tooling: `Tools/MapGen/` (Python; produces caldreth_* artifacts)
- Living state docs (READ THESE before acting): `PROJECT_STATE.md`, `ROADMAP.md`, `MECHANICS.md`,
  `CODE_REGISTRY.md`, `DECISIONS.md`, `CHANGELOG.md`, `REPORTS/`

## Non-negotiable conventions
- **No Event Tick.** Timers / BehaviorTree / EQS only. Everything rides the existing metabolism timer where possible.
- **C++ is the brain, Blueprint is the body.** C++ = math, memory, system logic. Visuals/anim/particles via
  `BlueprintImplementableEvent` / `BlueprintNativeEvent`. C++ never loads animations or knows about visuals.
- **Data-driven, zero hardcoded values.** Use `UDataAsset` / `UDataTable` (e.g. DT_ZoneDefs, OCEAN assets).
- **`IsValid()` / `if (Ptr)` everywhere.** Casts to AIController, components, world — all guarded. Designed for
  hundreds of interacting actors; assume any actor can die mid-action (P2P contracts, sleep, collapse).
- **`UE_LOG` generously, but only on state transitions** (threshold crossings, decisions) — not per tick.
  Use category `LogMaslow` (and project categories) with the exact log format the design gate specifies.
- **Patch, don't regenerate.** Add fields/functions to existing files; wire calls into existing functions
  (e.g. `UpdateFatigue`, `ProcessMetabolism`). NEVER regenerate whole `.h`/`.cpp`. Give the modified fragment
  with a clear insertion point.
- **Separate `.h` and `.cpp`** clearly. Correct `UPROPERTY` / `UFUNCTION` / `UENUM` macros with Blueprint categories.
- **500+ NPC is the ceiling.** Before writing any per-NPC system, state its memory + CPU cost ×500 ×tick.
  Nothing per-NPC that doesn't aggregate. Flag perf risks before coding (cache, spatial index — not TActorIterator per tick).

## Verify before closing anything (hard numbers, zero placeholders)
- Build with editor CLOSED → UHT. Then PIE.
- Read truth from `Saved/Logs/` on disk AND Monolith live object reads (`pie_get_object_properties`).
- Every number in a verification report comes from a live log / live object. Never from a placeholder or assumption.
- Two real bugs in the Sleep Engine were caught ONLY during hard PIE verification. Verification is not optional.
- Dead subsystems can hide for a long time (temperature was inert since project start). Confirm values actually
  CHANGE at runtime, not just that code exists.

## When to plan / ask / stop
- A design gate (from ARCHITECT) is required before implementing a defining mechanic. No gate → ask, don't guess.
- If reality (live code) contradicts the design doc → FLAG it before coding (as with the cold-burn diagnosis). Don't silently reinterpret.
- Staged delivery: finish a stage, verify, report, WAIT for explicit approval before the next stage.
- Use `/branch` before risky/experimental changes. Use `/clear` when switching topics (rules reload from this file).

## Tools / environment
- MCP: Monolith (port 9316), MCPUnreal (port 8090). If tools go silent, check for a blocking modal window first.
- Git: commit discipline is enforced — see OPERATING_PROTOCOL.md (comms repo). Code commits get a hash; the
  ARCHITECT records it in MILESTONES.

## Language
Converse with the director in **Polish**. All code, identifiers, comments, log strings, and commit messages in **English**.
