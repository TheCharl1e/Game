# HERITAGE — historia pre-orphan 5.7 (rejestr, bez blobów)
> **Cel:** zachować *najważniejszą informację* z pre-orphan historii 5.7 (44 commity, `e14fa24`..`b4edc36`)
> BEZ wciągania ~3.1 GB starego Contentu do KANON-a. Bloby zostają zimnym archiwum w
> `E:\Game-history-2026-06-21.bundle`. **Pliki źródłowe tej ery są już w 5.8** (zweryfikowane — patrz mapa niżej),
> więc potrzebny jest tylko REKORD: hashe (cytowalne), daty, tematy, ścieżki kodu.
> Wygenerowano 2026-07-01 z object-DB E:\Game (`git log b4edc36`), gate DOCS_STATE Opcja LEAN.

## MAPA: 5 kluczowych build-systemów pre-orphan → pliki w 5.8
| System | Commit (oryg.) | Data | Pliki źródłowe (żyją w E:\Game_58) |
|---|---|---|---|
| **Głód / APPETITE** (kaskada + kęsy + makra→magazyn + fat→izolacja) | `545a95d` | 2026-06-21 | `Source/Stan_Pierwotny/MaslowBiologicalComponent.{h,cpp}` ✅ · `AI/AnimNotify_EatBite.{h,cpp}` ✅ · `ItemBase.{h,cpp}` ✅ |
| **Temperatura** (AmbientTemp strefowy, rewitalizuje 3 martwe systemy temp) | `e2dd851` | 2026-06-19 | `MaslowBiologicalComponent.{h,cpp}` ✅ · `Map/ZoneDef.h` ✅ |
| **Sen** ETAP2 (mgła/mikrosen/Rested) · ETAP1 (HoursAwake→EFatigueState) | `8326f14` · `a7efcdb` | 2026-06-19 · 06-18 | `MaslowBiologicalComponent.{h,cpp}` ✅ |
| **NPCRegistry** (int32 ID subsystem + identity component) | `ba9c092` | 2026-06-20 | `NPC/NPCRegistrySubsystem.{h,cpp}` ✅ · `NPC/NPCIdentityComponent.{h,cpp}` ✅ |
| **OCEAN slice1** (Neuroticism → stochastic L0 panic) | `48e1a73` | 2026-06-20 | `MaslowBiologicalComponent.{h,cpp}` ✅ · `NPC/NPCIdentityComponent.h` ✅ |
| **Strefy Caldreth** (FZoneDef seed · bake 18 stref · GetZoneAtLocation) | `571956b`·`f0e65d9`·`7d17603` | 2026-06-17 | `Map/CaldrethZone.{h,cpp}` ✅ · `Tools/MapGen/zone_defs.json` ⚠️ (brak w 5.8 — do potwierdzenia) |
| **Body/senses** (26 części, EBodyPart helper) | `6e91372` + init `e14fa24` | 2026-06-16/12 | (w initial-import; obecne w 5.8) |

> ⚠️ Jedyna wykryta luka plikowa: `Tools/MapGen/zone_defs.json` nieobecny w `E:\Game_58` — reszta źródeł ✅ potwierdzona na dysku 5.8.

## PEŁNY REJESTR (44 commity, `e14fa24` → `b4edc36`, chronologicznie malejąco)
Format: `hash | data | temat`. Bloby (Content/Binaries/Plugins) NIE w KANON — tylko w bundlu.

| Hash | Data | Temat |
|---|---|---|
| `b4edc36` | 2026-06-21 | docs(npc): record APPETITE slice 1 commit hash 545a95d in ROADMAP L1-11 |
| `545a95d` | 2026-06-21 | feat(npc): APPETITE grubas slice 1 — eating + macro→storage + gastric stretch + fat→insulation |
| `1cd6611` | 2026-06-21 | Create APPETITE_GRUBAS_design.md |
| `308b106` | 2026-06-21 | 2026.06.21_ |
| `2a381d5` | 2026-06-20 | docs(npc): TRASH_ recon - state persistence is H2 (artifact), TECH-09 closed |
| `32c6b81` | 2026-06-20 | feat(npc): Maslow→BT bridge slice #1 - C++ drives concrete need (Thirst) |
| `8951d65` | 2026-06-20 | docs(npc): correct L3-02 TECH-08 misdiagnosis — no HP pin exists |
| `48e1a73` | 2026-06-20 | feat(npc): OCEAN L3-02 slice #1 — Neuroticism → stochastic L0 panic |
| `ba9c092` | 2026-06-20 | feat(npc): NPCRegistry keystone (L3-01) - int32 ID subsystem + identity component |
| `1e2540b` | 2026-06-19 | fix(bb): rename Nearst → Nearest blackboard keys (TECH-03) |
| `efb0a8f` | 2026-06-19 | fix(clock): revive seasonal interpolation in THE CALLENDAR (int-div → float) |
| `8dba663` | 2026-06-19 | Day1ofgit |
| `e2dd851` | 2026-06-19 | AmbientTemp (zone core): environment temperature revives 3 dead temp systems |
| `1333b90` | 2026-06-19 | uproject: remove AIAssistant plugin (kept Water + DynamicWind) |
| `8326f14` | 2026-06-19 | Sleep engine ETAP 2: fatigue effects + sleep/reset/Rested (A+B merge) |
| `a2dea26` | 2026-06-18 | docs: fix BP_NPC_Character component drift + record Maslow runtime verification |
| `c22d414` | 2026-06-18 | BP_NPC_Character: add MaslowBiologicalComponent (fixes Maslow→BB→BT pipeline) |
| `0f2804f` | 2026-06-18 | docs(Sleep ETAP 1): MECHANICS values + CODE_REGISTRY symbols + CHANGELOG |
| `a7efcdb` | 2026-06-18 | Sleep engine ETAP 1: fatigue accrual (HoursAwake → EFatigueState ladder) |
| `23c02f3` | 2026-06-18 | IMC_RTS: keep RTS camera WASD mappings (IA_MoveCamera) |
| `62b405a` | 2026-06-18 | docs(TECH-06): ROADMAP →done + CHANGELOG entry |
| `0091100` | 2026-06-18 | TECH-06: EditorStartupMap → CaldrethMap (island at editor startup) |
| `125d808` | 2026-06-18 | Game.umap: LudusAI level-blueprint edit (HUD click flow) |
| `19ec243` | 2026-06-18 | BP_NPC_Character: disconnect old HUD (SelectNPC_1) from LMB click chain |
| `da6bfdf` | 2026-06-18 | docs(SUP-01): correct the record — RF_Standalone was the real flaky-save cause |
| `2b42b6c` | 2026-06-18 | SUP-01: persist NPC capsule Visibility=Block to disk (+RF_Standalone repair) |
| `b2226cd` | 2026-06-18 | SUP-01 DONE: NPC click→inspector works (PIE-confirmed) |
| `a001a69` | 2026-06-18 | HUD click diagnosis: correct root cause + record Visibility-block debt |
| `591c5bb` | 2026-06-18 | DESIGN: defer Moore-trace outlines into the NPC-consumer perf bundle |
| `294bd95` | 2026-06-18 | docs reconcile (cont.): ROADMAP/MECHANICS/EXECUTION_PLAYBOOK → Gra_Stan_Pierwotny/ |
| `4661dd5` | 2026-06-18 | Mark TActorIterator query as temporary (PERF TODO) + record perf plan |
| `87761c4` | 2026-06-18 | docs reconcile: Gra_Stan_Pierwotny/ canonical; merge root dupes |
| `c10d85f` | 2026-06-18 | River zone → no/no (water channel, not bank); re-bake CaldrethMap |
| `f0e65d9` | 2026-06-17 | Bake 18 zones + 5 POI markers into CaldrethMap (with DT_ZoneDefs flags) |
| `571956b` | 2026-06-17 | DT_ZoneDefs: seed FZoneDef table from zone_defs.json (12 biomes, verified) |
| `4f8cf31` | 2026-06-17 | docs: ETAP 5 runtime-verified + registry/changelog updates |
| `7d17603` | 2026-06-17 | ETAP 5: GetZoneAtLocation runtime query + CODE_REGISTRY docs |
| `fa01492` | 2026-06-17 | Add CaldrethMap startup level (5 POI markers) + TECH-06 roadmap note |
| `546e6e9` | 2026-06-17 | ETAP 4: Caldreth POI import (ACaldrethPOIMarker + ImportCaldrethPOIs) |
| `22e550d` | 2026-06-17 | ETAP 3: commit Caldreth import editor widget (EUW_CaldrethImport) |
| `957075a` | 2026-06-17 | Baseline: green build — Caldreth map-import layer + HUD fixes + repo hygiene |
| `6e91372` | 2026-06-16 | Add GetPartDisplayName(EBodyPart)→FText C++ helper (runtime-safe) |
| `8e08403` | 2026-06-16 | Checkpoint: build green + roadmap canonical, before SUP-01b graph surgery |
| `e14fa24` | 2026-06-12 | Initial commit: Stan_Pierwotny current state (UE 5.7) |

## ODZYSK SUROWYCH BLOBÓW (gdyby kiedyś potrzebne)
Historia z pełnymi plikami (w tym Content) żyje w `E:\Game-history-2026-06-21.bundle` (3.1 GB) oraz w object-DB
`E:\Game` (dangling). Podgląd bez importu do KANON: `git -C E:\Game show <hash>:<ścieżka>` lub
`git -C E:\Game log <hash>`. Import do KANON świadomie odrzucony (×3500 balon). Design-docy tej ery
(`APPETITE_GRUBAS_design.md`, `AMBIENT_TEMP_design.md`, `SLEEP_ENGINE_*`, `OCEAN_L3-02_design.md`) → `E:\Game_git\`.
