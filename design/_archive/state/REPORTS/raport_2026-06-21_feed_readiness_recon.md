# Raport — RECON gotowości pod FEED (zdarzenia / inspektor / kamera / HUD) · 2026-06-21

> RECON ONLY — zero kodu, zero projektowania. Architekt projektuje feed dopiero z tymi faktami.
> Źródła: żywy kod `Source/Stan_Pierwotny` (file:line) + Monolith live (PIE), CaldrethMap, Monolith 0.20.2, UE 5.7 CL-51494982.
> Proweniencja: `[SRC]` dysk/kod · `[LIVE]` live PIE ta sesja · `[DOC]` CODE_REGISTRY (PIE-verified 2026-06-18).

---

## 0) TL;DR dla architekta
1. **NIE istnieje żaden event bus** — zdarzenia rdzenia to albo per-NPC `BlueprintImplementableEvent` (C++→BP właściciela), albo goły `UE_LOG`. Feed nie ma się do czego podpiąć globalnie.
2. **Kamera→aktor już działa** przez zmienną `BP_RTSCamera.TrackedNPC` (twardy snap-follow w Tick). Hak dla „klik wpisu feedu → kamera" jest gotowy; brak „one-shot fly + puść".
3. **Inspektor = ciało-only, pull-snapshot.** Czyta wyłącznie `BodyConditionComponent`; **zero Maslow**. Per-NPC życiówka (głód/sen/temp/HP) dla feedu musi iść z `MaslowBiologicalComponent` bezpośrednio.
4. **RTS HUD skeleton niezbudowany** — HUD_CONSOLIDATION niewdrożony; `WBP_RTSHud` nie istnieje, `HUDReference=None`, zegar dalej PUSH z `BP_DayNightCycle`.

---

## 1) STRUKTURA ZDARZEŃ — brak busa `[SRC]`
- **Zero multicast/broadcast w rdzeniu.** Jedyne `DECLARE_*_DELEGATE` w `Source/` to szablony TP_ThirdPerson (`FOnEnemyDied` itd.), nie symulacja.
- **Subsystemy:** tylko `UNPCRegistrySubsystem` (rejestr ID↔NPC, `NPC/NPCRegistrySubsystem.h:22`) i `UCaldrethZoneWorldSubsystem` (indeks przestrzenny, `Map/CaldrethZone.cpp:183`). Żaden to nie kanał powiadomień. `NPCRegistrySubsystem.h:13`: „Nothing social (P2P L3, reputation L4...)".
- **Realne sygnały:** (a) `BlueprintImplementableEvent` C++→BP właściciela (point-to-point, niesubskrybowalne globalnie); (b) `UE_LOG`; (c) return/out-param. Jedyny multicast w projekcie: BP-side `BP_DayNightCycle.OnHourPassed/OnMinutePassed`.

## 2) ŹRÓDŁA ZDARZEŃ — realne punkty `[SRC]`
| Zdarzenie | Plik:funkcja | Mechanizm | Broadcast? |
|---|---|---|---|
| THEFT | `InventoryComponent.cpp:438 TryWithdraw()` | `bWasUnauthorized=!IsPublic()&&OwnerID!=RequesterID` → `UE_LOG(LogInventory,"THEFT: requester %d...")` | NIE (UE_LOG + bool) |
| Śmierć (głód/odwodnienie) | `MaslowBiologicalComponent.cpp:117 i :155` `OnStarvedToDeath()` | `BlueprintImplementableEvent` (`h:512`) + clear MetabolismTimer | NIE (BP event właściciela) |
| Autofagia | `MaslowBiologicalComponent.cpp:240` | `CurrentHungerPhase=Phase_3_Autophagy` (sam stan enuma) | NIE (brak eventu/logu) |
| Omdlenie (≥24h) | `MaslowBiologicalComponent.cpp:368 OnCollapse()` (+`StopLogic(BT)` :361, `bIncapacitated`) | `BlueprintImplementableEvent` (`h:330`) | NIE |
| Mikrosen | `MaslowBiologicalComponent.cpp:375 OnMicrosleep()` / `:388 OnMicrosleepEnd()` | `BlueprintImplementableEvent` (`h:326/328`) + `UE_LOG` | NIE |
| Hipotermia / Level_1_Temperature | `MaslowBiologicalComponent.cpp:194` | `CurrentTemp<=CriticalTempThreshold(34) && !bHypothermiaLogged` → `UE_LOG(LogMaslow,"[Temp] %s: HIPOTERMIA...")`, histereza +1°C | NIE (UE_LOG only) |
| Zerwany kontrakt P2P | — | **BRAK** systemu. Tylko `EEatAbortReason::SourceGone "(P2P stole/destroyed)"` (`h:101`, `.cpp:861`) — obsługa znikającego jedzenia, nie kontrakt | — |
| Eureka | — | **BRAK** mechaniki — tylko komentarz buffa Rested (`h:251`) + `OnRested()` | — |

Dodatkowe per-owner BP eventy: `OnInventoryChanged` (`InventoryComponent.h:259`), `OnItemDepleted` (`ItemBase.h:64`), `OnMentalFogStart/End`, `OnWakeUp`, `OnRested`.

## 3) INSPEKTOR (WBP_NPC_Inspector + WBP_Tab_Body) `[LIVE]`
- **Kontrakt ref:** `PC_RTSGameMode` klik→trace→Cast `BP_NPC_Character` → `CreateWidget(WBP_NPC_Inspector)` → `SetInspectedNPC(NPC)`. `SetInspectedNPC`: `Cast BP_NPC_Character → Set NPCRef → AddToViewport`. Kontroler PCHA ref RAZ.
- **Czyta TYLKO `BodyConditionComponent`:** 26-part `GetPartEffectiveHealth/GetPartInjury/GetPartDisplayName`; zmysły `GetVisionAcuity/GetHearingAcuity/GetSpeechClarity/GetHandPrecision(L/R)/GetMobility`. Wejście `WBP_Tab_Body.SetNPCRef → Get BodyConditionComp`.
- 🔴 **`Maslow` = 0 trafień** (live) → brak głodu/`FatigueState`/snu/`AmbientTemp`/`CurrentTemp`/HP/Hydration. Gettery istnieją (`GetFatigueState` `h:258`, `GetActionableNeed`, `bIsSleeping`, `AmbientTemp`...) — niepodpięte.
- **Push/pull:** **PULL on-demand, NIE per-tick** — `Tick/Construct/PreConstruct` w inspektorze **wyłączone (disabled)** mimo `has_tick=true`. Dane = snapshot z selekcji; brak auto-refresh przy otwartym oknie.
- **Cykl życia:** właściciel `PC_RTSGameMode` — `CreateWidget` na klik, `RemoveFromParent` na odznaczenie (4 węzły w PC).

## 4) SELEKCJA + FOKUS KAMERY `[LIVE graf]`
- **Selekcja:** `PC_RTSGameMode`, `bEnableClickEvents=true`, LMB → `Get Hit Result Under Cursor (Visibility)` → Cast NPC. Trace, nie EQS.
- **Kamera (BP_RTSCamera):** zmienna `TrackedNPC:Object`, **zero własnych funkcji**, brak `FocusOnActor`. Event Tick: `IsValid(TrackedNPC) → Cast Actor → SetActorLocation(self, X=NPC.X, Y=NPC.Y, Z=self.Z)` = **twardy snap-follow w poziomie, trzyma wysokość/zoom, bez lerp**. `Set TrackedNPC=NPC` na kliku (lock); `IA_MoveCamera` (WASD) → `Set TrackedNPC=None` (ręczny ruch zrywa lock).
- C++ `ARTSCameraPawn` (`RTS/RTSCameraPawn.h/.cpp`) ma tylko WASD/zoom/pan — **cały fokus jest w BP childzie**.
- **Hak feedu:** `Set BP_RTSCamera.TrackedNPC = aktor`. Follow ciągły, nie jednorazowy; brak wygładzania.

## 5) SZKIELET RTS HUD (po HUD_CONSOLIDATION) `[LIVE]`
- **`WBP_RTSHud` NIE istnieje** — konsolidacja niewdrożona (zatrzymana na pre-flight).
- **`PC_RTSGameMode.HUDReference = None`** — sierota niepodpięta, brak AddToViewport z kontrolera.
- Jedyny HUD na ekranie: `WBP_DebugInfo` (`in_viewport=True`), dalej PUSH z `BP_DayNightCycle` (`DNC.DebugUI=WBP_DebugInfo_C`). Brak timer-pull zegara.
- `PC_CLASS=PC_RTSGameMode_C`, `PAWN=BP_RTSCamera_C` (live). Stan niezmieniony od pre-flightu.

---

## Konsekwencje (dla projektu feedu — NIE projektowane tutaj)
- Feed nie ma globalnego punktu zaczepienia → wymaga **nowego cienkiego kanału zdarzeń** albo per-NPC hooków pod istniejące `BlueprintImplementableEvent`, albo skrobania logów (THEFT/HIPOTERMIA).
- „Klik wpisu feedu → kamera": gotowy hak `TrackedNPC` (follow). „One-shot fly" wymagałby nowego API kamery.
- Per-NPC życiówka feedu: **z `MaslowBiologicalComponent`**, nie z inspektora (ciało-only).

**STOP — recon zakończony. Zero kodu, zero projektowania.**
