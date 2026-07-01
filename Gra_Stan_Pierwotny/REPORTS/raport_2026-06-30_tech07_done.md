# Raport — TECH-07: GetHPPercent dzieli przez CurrentMaxHP · DONE + PIE-verified · 2026-06-30

> C++-only (zero assetów). Build editor-closed → CDO/PIE verify w Game_58 (5.8, projekt żywy).
> Fix naniesiony identycznie w obu repo (E:\Game 5.7 + E:\Game_58 5.8).

## Fix
`UMaslowBiologicalComponent::GetHPPercent()`:
- **było:** `return FMath::Clamp(CurrentHP / 100.0f, 0.0f, 1.0f);` (literał 100 = `AbsoluteMaxHP`)
- **jest:** `if (CurrentMaxHP <= 0.0f) return 0.0f; return FMath::Clamp(CurrentHP / CurrentMaxHP, 0.0f, 1.0f);`

Autofagia/odwodnienie OBNIŻA `CurrentMaxHP` (a `CurrentHP` jest do niego klampowane). Stary getter dzielił przez stałą 100 → wyniszczony NPC przy PEŁNYM aktualnym HP czytał `0.5`; paski HUD kłamały po wyniszczeniu. Teraz czyta względem aktualnego max + guard `<=0 → 0`. Lustrzane do logiki paniki (`MaslowBiologicalComponent` ~:949, która już liczyła `CurrentHP/CurrentMaxHP`).

## Build (oba editor-closed, wymuszony recompile: skasowany obj+DLL)
- **Game_58 (5.8):** target `Game_58Editor` → `[1/5] Compile MaslowBiologicalComponent.cpp` + `[4/5] Link UnrealEditor-Stan_Pierwotny.dll` → **Result: Succeeded** (43.9s), DLL 14:51:56.
- **E:\Game (5.7):** target `Stan_PierwotnyEditor` → **Result: Succeeded** (identyczny fix).

## Verify — Game_58, świeży edytor (PID 5700, cmdline potwierdza `Game_58.uproject`, start PO buildzie)
**CDO (`Default__MaslowBiologicalComponent`, deterministyczny, zero tykania) — 5/5 OK:**

| Max / HP | get_hp_percent | stary (HP/100) | uwaga |
|---|---|---|---|
| 100 / 100 | 1.0000 | 1.0 | — |
| **50 / 50** | **1.0000** | 0.5 | **KLUCZ** — wyniszczony NPC przy pełnym aktualnym HP |
| 40 / 80 | 1.0000 | 0.8 | HP>Max → clamp 1.0 |
| 0 / 80 | 0.0000 | 0.8 | guard `CurrentMaxHP<=0` |
| 50 / 25 | 0.5000 | 0.25 | gradient częściowego HP względem aktualnego max |

**Żywa instancja PIE** (GHOST_SCAN, unpaused, jedyny `MaslowBiological`, zero `TRASH_`): `Max=50 HP=50 → 1.0000` ✓.
Artefakt do protokołu: iniekcja na żywym, odwodnionym NPC bywa zerowana przez metabolizm (autofagia ↓`CurrentMaxHP`, odwodnienie ↓`CurrentHP`) między `set` a odczytem; **pauza** dodatkowo zaburzała → CDO użyty jako czysty oracle.

## Pułapka procesu (utrwalona w pamięci [[game58-is-live-58-project]])
Najpierw edytowałem/budowałem **E:\Game (5.7)**, a żywy edytor to **Game_58 (5.8)** → fix „nie dochodził" (CDO pokazywał stary kod), aż potwierdziłem `.uproject` z cmdline procesu. MCPUnreal `status.project_root` jest mylący (raportuje E:\Game, proxuje do edytora Game_58). **Reguła:** przed buildem pod runtime-verify potwierdź `.uproject` z cmdline procesu edytora, nie ze statusu MCP.

## Status
ROADMAP TECH-07 ⬜ → ✅. Zero assetów, zero ryzyka regresji (panika L3-02 nie używa tego gettera — liczy własne HP%; HUD czyta poprawnie po wyniszczeniu). CTRL+S rano: brak (C++-only).
