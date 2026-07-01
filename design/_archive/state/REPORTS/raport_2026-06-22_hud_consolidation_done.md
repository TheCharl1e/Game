# Raport — HUD_CONSOLIDATION: wykonanie + weryfikacja · 2026-06-22

> Gate: jeden persistent HUD RTS, właściciel = PC_RTSGameMode; aktory środowiskowe tylko wystawiają dane.
> Wykonane przez Claude Code (Monolith live, CaldrethMap). Bez hasha (nie commitowane jako kod).
> Źródła weryfikacji: live PIE + compile. `[LIVE]`/`[SRC]`.

═══════════════════════════════════════════════
## WYNIK: HUD_CONSOLIDATION DZIAŁA ✅
═══════════════════════════════════════════════
Jeden persistent HUD RTS, właściciel = `PC_RTSGameMode`, aktor = czyste źródło (zero karmienia widgetu),
pull przez dispatcher (zegar/Day/Pora advansują live), EC-1 podpięte, inspektor nietknięty, brak ducha.

## Część A — przeniesienie własności
- **Rename `WBP_DebugInfo → WBP_RTSHud`** (EditorAssetLibrary.rename_asset) — czysto: 0 redirectorów, referencery (PC, DayNightCycle) zaktualizowane.
- **`PC_RTSGameMode.HUDReference` auto-przetypowany → `WBP_RTSHud_C`** (rename to załatwił).
- **`PC_RTSGameMode.BeginPlay`** (dobudowane): po `Set bEnableClickEvents` → `Branch IsValid(HUDReference)` → (False) `CreateWidget(WBP_RTSHud, owner=self)` → `Set HUDReference` → `AddToViewport`. Idempotent. Compile green.
- **`BP_DayNightCycle.BeginPlay` strip**: usunięte `Create WBP…Widget` + `Set DebugUI` + `AddToViewport`. Skutek: `DebugUI` permanentnie None → aktor nie karmi żadnego widgetu. Compile green.

## Część B+C — kierunek danych (pull) + odświeżanie
**ODKRYCIE w trakcie (gate §4 flaga):** `WBP_RTSHud` miał **już** zaimplementowany pull — ale **dispatcher-owy, nie timer 1s** z D-REFRESH:
- `Construct` → `GetActorOfClass(BP_DayNightCycle)` → `Set DayNightCycleRef` → `Bind OnTimeUpdated do OnMinutePassed`.
- `OnTimeUpdated` (co minutę gry) → pull `CurrentTimeWorld` → `ST_Time` → `{H}:{M}` → `SetText CurrentTime`.
- `BP_DayNightCycle.TIME` realnie broadcastuje `Call On Minute Passed` (potwierdzone).

**Decyzja (zatwierdzona przez dyrektora): zostawić dispatcher** — czystsze (zero pollingu), zgodne z „dispatcher upgrade" dopuszczonym w gate (§91–93). NIE budowano timera 1s (zdublowałby działający mechanizm). `has_tick=false` (zero Event Tick — reguła projektu zachowana).

**Dobudowane luki:**
- **EC-1 „--:--":** `Construct Cast.CastFailed → SetText CurrentTime "--:--"` (brak zegara na mapie → degradacja).
- **Day/Pora:** `OnTimeUpdated` rozszerzony — `DayOfYear`→ToText→`SetText Day`; `Pora`(string)→ToText→`SetText Pora` (foreign-reads z DayNightCycleRef przez add_property_access).
- **D-STRIP:** `SunIntens` + `MoonIntens` → Visibility=Collapsed (schowane, debug).

## Minute-fix (osobny ruch po weryfikacji)
PIE ujawnił błąd minuty: `CurrentTime="12:742"/"20:1251"` — `ST_Time.Minute` w `CurrentTimeWorld` to **suma minut doby (≈raw×60)**, nie minuta-godziny. To **pre-existing** w istniejącym łańcuchu dispatcher, nie z konsolidacji. **Fix:** wstawiono `Break.Minute % 60 → FormatText.M` (H bez zmian). PIE po fixie: `CurrentTime="10:56"` (poprawne 0–59). Compile green.

## Weryfikacja bloków 1–6 `[LIVE PIE, CaldrethMap]`
| Blok | Wynik | Dowód |
|---|---|---|
| 1. Sierota podpięta | ✅ | `HUDReference=WBP_RTSHud_C`, `in_viewport=True` |
| 2. Aktor odpięty | ✅ | `DNC.DebugUI=None` |
| 3. Zegar ciągnie + poprawny | ✅ | raw 12→20h advansuje; `CurrentTime` HH:MM poprawne (po %60), `Pora` Day→Night, `Day=100` |
| 4. EC-1 „--:--" | 🔶 wired, nie runtime-verified | CaldrethMap MA zegar → gałąź CastFailed nieosiągalna tutaj; test wymaga mapy bez DayNightCycle |
| 5. Inspektor nietknięty | ✅ | `SelectedNPC/ActiveNPCWindow/NPCInspectorWidget=None` (brak klika), ścieżka klik→inspektor nieruszona |
| 6. Brak ducha | ✅ | jedyny HUD = HUDReference (PC); DebugUI=None |

## Długi / residual (poza tym gate'em)
- **SUP-05 (P3, ROADMAP):** martwe `Get DebugUI`→SetText w `THE ATMOSPHERE`(3×)/`Debug`(2×) — behawioralnie martwe (DebugUI=None), do wycięcia **wizualnie w edytorze** (rdzeniowe funkcje słońca/atmosfery, zero blind).
- **EC-1 runtime-test:** na mapie bez DayNightCycle (przyszłościowo).
- **Minute-source:** docelowo lepiej poprawić populację `CurrentTimeWorld.Minute` w `BP_DayNightCycle` (źródło), nie tylko display `%60` — opcjonalne.

## Zapis
Część A, B/C, minute-fix — zapisane ręcznie (Ctrl+S) przez dyrektora (reguła RF_Standalone; Monolithem nie zapisywano). Bez hasha (docs/asset, nie commit kodu).
