# Raport 2026-06-16 — MECHANICS verify + Monolith MCP install

## 1. MECHANICS.md verified against C++ (Maslow / Body / Inventory)

Files read: `Source/Stan_Pierwotny/MaslowBiologicalComponent.{h,cpp}`,
`Source/Stan_Pierwotny/Body/BodyConditionComponent.{h,cpp}`,
`Source/Stan_Pierwotny/InventoryComponent.{h,cpp}`. Edited: `MECHANICS.md`.

### 1a. [LOCKED]/spec values that DIFFER in code (corrected in MECHANICS)
| Variable | Spec said | Code actually | 
|---|---|---|
| MaxGlucose | [TBD] ~100 | **1000** |
| MaxGlycogen | [TBD] ~100 | **1000** |
| MaxBodyFat | [TBD] ~100 | **5000** |
| Hunger phase 2 name | `Phase_2_Fat` | `Phase_2_FatBurn` (burns 0.5× rate) |
| Hunger phase 3 name | `Phase_3_Protein` | `Phase_3_Autophagy` (MaxHP −=0.25×burn/tick) |
| Cold burn mult | [TBD] suggest ×2 | **×2 at CurrentTemp<10°C** (coded) |
| Thirst ×1/×3/×5 | [LOCKED] | NOT hardcoded — DT_ActionCosts/SetCurrentActionMultiplier; only **poison ×3** is coded |
| L0 panic trigger | threat via VisionAcuity [LOCKED] | **CurrentHP ≤ 25** (CriticalHPThreshold) |
| Dehydration | "blocks stamina regen" | only **MaxHP&HP −10/tick**; stamina-block NOT coded |

### 1b. [GETTER READY, ENGINE TODO] (declared, getter exists, engine empty)
- Sleep: `GetHoursAwake()`/`GetHoursAwakePercent()` exist, `MaxHoursBeforeCollapse=24`, but `HoursAwake` is NEVER incremented; fog −30% / rested +20% / passout: no engine.
- L0 vision-panic: `GetVisionAcuity()` exists; threat→panic wiring not built (L0-04).

### 1c. Verified MATCHING (no change)
- Body 100%: sense weights (Vision .5/.5, Hearing .5/.5, Speech 1.0, Hand .40/.30/.10/.08/.07/.05, Mobility .35/.15/.35/.15), cascade `min(own,parent)`, absent=1.0, 26 parts, injury enum.
- Inventory: `PUBLIC_OWNER_ID=-1`, int32 stacks, unequip-requires-empty, `TryWithdraw` "THEFT:" log.
- Hydration max=100, no-Tick, ranges 0–1.

### 1d. Code constants added to MECHANICS (ground-truth for tests)
`BaseBurnRate=10` (10s timer), `HydrationBurnRatePerTick=2.0`, fat 0.5×, autophagy 0.25×,
dehydration −10/tick, CriticalThresholds HP=25 / Temp=34 / Hydration=20 / Stamina=15 / Kcal=500.

### 1e. [TBD] — proposed values awaiting architect approval
PanicDuration 8.0s · FleeSpeed 1.5× · fog onset 16h · microsleeps 20h · HealPart +0.3 ·
raw-meat PoisonChance 0.35 · Rep start 0 / P2P +1 / rescue +5 · Mastery 50 turns ·
Log retention 1 day · lie −10 rep / banish ≤ −20. Design-needed (not just a number):
OCEAN→BT mapping, NPCRegistry structure, contract rules, innovation formula.

**Build result:** doc-only task; last C++ build green (Result: Succeeded, commit 6e91372).

## 2. Monolith MCP install (IN PROGRESS)
- Removed unfinished flopperam `Plugins/UnrealMCP/` (uncommitted); kept remiphilippe `MCPUnreal`.
- Verified repo via GitHub API: `tumourlove/monolith`, 166★, latest **v0.20.2** (2026-06-15), UE 5.7, native C++.
- Downloaded `Monolith-v0.20.2.zip` (15.38 MB), extracted, inspected.
- **Gates PASSED:** precompiled DLLs present (20× `UnrealEditor-Monolith*.dll`) + `monolith_proxy.exe`;
  build-ID **47537391 == our project 47537391** → NO REBUILD; engine 5.7 aligned (no version conflict).
- Placed plugin at `E:\Game\Plugins\Monolith\` (uplugin + 20 DLLs + proxy).
- Security: port **9316 binds all interfaces** (LAN-reachable) — firewall on untrusted nets.

### Pending (Monolith)
- Step 4: register `.mcp.json` server "monolith" → `Plugins/Monolith/Binaries/monolith_proxy.exe` (port 9316), alongside `mcp-unreal` (don't overwrite).
- Step 5: restart editor to load plugin → confirm `Monolith MCP server listening on port 9316` (LogMonolith). NOTE: Claude Code must reload `.mcp.json` (session restart) before monolith tools are callable in-session.

## 3. Docs/policy this session
- CLAUDE.md: rule 10 (CHANGELOG every change), rule 11 (REPORTS after major task), session-start adds MECHANICS.md + EXECUTION_PLAYBOOK.md.
- Created `CHANGELOG.md` (backfilled), this `REPORTS/` file.

## 4. Open / blocked
- CLAUDE.md "append pasted content" — 🔴 no content was attached; awaiting paste.
- SUP-01b Gap A/B — still not in graph; will retry via Monolith once it's live (its node-authoring may finally automate it).
