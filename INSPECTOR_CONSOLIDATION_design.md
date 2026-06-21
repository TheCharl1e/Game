# INSPEKTOR NPC — KONSOLIDACJA (DESIGN GATE)
> Projekt do zatwierdzenia (architekt + Szymon, 2026-06-21). To robota BP/UMG (ciało) + podpięcie
> istniejących getterów C++ (mózg). Gate definiuje STRUKTURĘ, ŹRÓDŁO DANYCH, CYKL ŻYCIA i KOLEJNOŚĆ.
> Skin: Dark Souls minimal mrok (obsydian/żar/kość, hairline ramki). Portret: wariant A (capture w świecie).
>
> 🔴 SEDNO: dziś klik na NPC tworzy DWA pływające okna czytające BP-warstwę. Cel: JEDEN inspektor
> z głową + zakładkami, czytający C++ jako single source. Pre-flight #1 rozstrzyga, czy BP-metabolizm
> to lustro C++ czy drugi mózg (wzorzec Maslow/BT Bridge).

═══════════════════════════════════════════════════════════
# STAN WEJŚCIOWY (z reconów — twarde, live Monolith)
═══════════════════════════════════════════════════════════
- `WBP_NPC_Inspector` (+ `WBP_Tab_Body`): ciało 26-part + zmysły. PULL-snapshot, tick WYŁĄCZONY
  (Tick/Construct/PreConstruct disabled). Czyta TYLKO `BodyConditionComponent`
  (`GetPartEffectiveHealth/GetPartInjury/GetPartDisplayName`, `GetVision/Hearing/SpeechClarity/HandPrecision/Mobility`).
- `WBP_NPC` (ActiveNPCWindow): metabolizm (Kcal/Stamina/MuscleMass/FatMass/TotalWeight). EVENT TICK
  per-frame (łamie regułę „No Tick"). Czyta BP-warstwę: `ST_Metabolism`, `ST_BodyComposition`,
  `CalculateTotalWeight()` na `BP_NPC_Character` — NIE C++ Maslowa.
- Oba: właściciel `PC_RTSGameMode` (CreateWidget na klik LMB→trace Visibility→Cast `BP_NPC_Character`,
  RemoveFromParent na odznaczenie). Referencja NPC PCHANA raz (`NPCRef` / `TargetNPC`), dalej PULL.
- Kamera: `BP_RTSCamera.TrackedNPC` (Event Tick → `SetActorLocation` X/Y NPC, Z własne) — snap-follow.
  Set na kliku, WASD zrywa lock. Brak C++ API fokusa (cały fokus w BP childzie).
- 🔴 Długi (twardo z grafu `WBP_NPC`): Event Tick polling · Print String "Hello" na gałęzi !IsValid
  (debug spam co klatkę) · `ST_Time` sierota · `EditableTextBox` zamiast `TextBlock` (readout edytowalny) ·
  structy dają 6+4 pola, użyte 2+2.
- 🔴 Maslow = 0 trafień w obu oknach. Niepodpięte, ale ISTNIEJĄCE gettery C++:
  `GetFatigueState`, `GetActionableNeed`/`EvaluateCurrentNeed`, `GetWorkEfficiencyMultiplier`,
  props `bIsSleeping/bMicrosleeping/bIncapacitated/AmbientTemp/CurrentTemp/CurrentHP/Hydration/Glucose...`.

═══════════════════════════════════════════════════════════
# DECYZJE PROJEKTOWE (do zatwierdzenia — NIE zmieniać bez architekta)
═══════════════════════════════════════════════════════════
| # | Decyzja | Wartość |
|---|---|---|
| D-CONSOLIDATE | Liczba okien | **Scal DWA okna w JEDEN inspektor** (głowa + zakładki). `WBP_NPC` (metabolizm) i `WBP_NPC_Inspector` (ciało) → jeden shell. `WBP_Tab_Body` reużyty jako zakładka. |
| D-SOURCE | Źródło danych | **C++ gettery = single source.** Inspektor czyta `MaslowBiologicalComponent`/`BodyConditionComponent`/`InventoryComponent` bezpośrednio. BP-warstwa metabolizmu (`ST_Metabolism`) demotowana, jeśli to drugi mózg (pre-flight #1). |
| D-DATADIR | Kierunek danych | **PULL-snapshot, NIE Event Tick.** Odświeżenie na selekcji + na żądanie (timer ~0.5–1 s gdy okno otwarte, albo on-event). Zero ticka. |
| D-HEAD | Stała głowa | **Portret + use-name + bieżący priorytet (`GetActionableNeed`) + 5 pasków (głód/pragnienie/temp/sen/HP) ZAWSZE widoczne** nad zakładkami. Souls'owy rzut oka. |
| D-TABS | Zakładki | **11 zakładek (kolejność = wspinaczka Maslowa).** 4 budowalne teraz, 7 na kłódkę (roadmap). |
| D-LOCK | Puste zakładki | **Kłódka, nie pustka.** Zakładka bez danych = wyszarzona + ikona kłódki + „dochodzi (L#)". Kłódka znika DOPIERO gdy są realne gettery. Nigdy „zepsute", zawsze „nadchodzi". |
| D-PORTRET | Portret | **Wariant A — `USceneCaptureComponent2D` → `UTextureRenderTarget2D` celujący w PRAWDZIWEGO NPC w świecie.** Pokazuje realny stan ciała (ragdoll/rany/dreszcze). |
| D-SKIN | Wygląd | **Dark Souls minimal mrok.** Obsydian `#14110F` ~85%, żar `#C9603A` tylko alerty/krytyczne, kość/przygaszone złoto tekst, hairline brąz `#6B5A3E`, paski zdesaturowane, szeryf. |
| D-DEBT | Długi | **Sprzątnięte przy okazji:** usuń Print "Hello", `ST_Time` sierotę, zamień `EditableTextBox`→`TextBlock`, wytnij Event Tick. |
| D-CAMERA | Fokus | **Reużyj `BP_RTSCamera.TrackedNPC`** (już ustawiany na selekcji). Bez zmian — inspektor nie dotyka kamery. |

═══════════════════════════════════════════════════════════
# STRUKTURA — GŁOWA + 11 ZAKŁADEK (z gotowością danych)
═══════════════════════════════════════════════════════════
Legenda: ✅ żywe dziś · 🔨 getter gotowy / dane żyją, do podpięcia · ⬜ projekt (kłódka).

## STAŁA GŁOWA (nad zakładkami, zawsze widoczna)
- Portret (wariant A) · use-name (duży szeryf) · true-name (zredagowany, kłódka, L3) · rola · strefa.
- Bieżący priorytet: `GetActionableNeed()` — „co go teraz napędza" (panic/temp/thirst/sleep/hunger/None). 🔨
- 5 cienkich pasków: głód, pragnienie, temperatura, sen, HP. Żar tylko gdy krytyczne. 🔨

## ZAKŁADKI
| # | Zakładka | Status | Czyta (źródło) |
|---|---|---|---|
| 1 | **Ogólna** (kto/co/gdzie/reputacja) | 🔨 | use+true-name, strefa (`CaldrethZone` cache), `GetActionableNeed`, bieżąca akcja (BT). **Reputacja = pole „—" do L4.** |
| 2 | **Stan** (L1) | 🔨 ⚠️ | kaskada kataboliczna (`EHungerPhase`, Glucose/Glycogen/FatReserves/Protein), Hydration, Stamina, temp (`AmbientTemp`/`CurrentTemp`+hipotermia), sen (`GetFatigueState`/`HoursAwake`/`bIsRested`). **⚠️ z C++ getterów, NIE BP `ST_Metabolism` — patrz pre-flight #1.** |
| 3 | **Ekwipunek** | 🔨 | `InventoryComponent` kontenery — prywatne (`OwnerID`) vs publiczna spiżarnia (`PUBLIC_OWNER_ID=-1`). |
| 4 | **Stan ciała** | ✅ | `BodyConditionComponent` — 26-part + zmysły. **Istnieje jako `WBP_Tab_Body`.** |
| 5 | Osobowość (OCEAN) | ⬜ | brak danych (DataAsset z projektu L3) → kłódka. |
| 6 | Relacje | ⬜ | wymaga social `NPCRegistry` (dziś „nothing social") → kłódka. |
| 7 | Kontrakt (P2P) | ⬜ | system P2P nie istnieje → kłódka. |
| 8 | Wiedza | ⬜ | mapa wiedzy NPC (Campfire Sync) nie istnieje → kłódka. **Priorytet emergencji.** |
| 9 | Log | ⬜ | dziennik [czas, strefa, akcja, widziani] nie istnieje → kłódka. **Debug goldmine + rdzeń L5.** |
| 10 | Umiejętności | ⬜ | mistrzostwo/specjalizacje (L4) nie istnieje → kłódka. |
| 11 | Tech | ⬜ | znane receptury/Eureki (rama, nie kod) → kłódka. |

**Inspektor v1 = głowa + zakładki 1–4.** Zakładki 5–11 widoczne jako kłódki (roadmap).
Każda kłódka znika osobno, gdy jej system ożyje danymi — anti-silent dyscyplina.

═══════════════════════════════════════════════════════════
# PORTRET (wariant A) — perf
═══════════════════════════════════════════════════════════
- `USceneCaptureComponent2D` → `UTextureRenderTarget2D` (256×256) → `Image` w UMG przez materiał.
- JEDNA kamera capture, NIE per-NPC. Celuje w zaznaczonego NPC (ref z głowy = `NPCRef`).
- 🔴 `bCaptureEveryFrame = false` po zamknięciu okna. Capture co ~10–15 klatek albo on-event, NIE 60 fps
  (poza idle/ragdoll nic się nie dzieje wizualnie).
- Oświetlenie = świat (półmrok wulkanu to estetyka). Ew. jeden fill-light na rig gdy noc — opcjonalnie.
- Koszt: jeden capture aktywny tylko gdy inspektor otwarty → pomijalny niezależnie od liczby NPC w świecie.

═══════════════════════════════════════════════════════════
# PERF
═══════════════════════════════════════════════════════════
Inspektor to JEDEN widget on-demand gracza — NIE jest w ścieżce 500 NPC. Pull-snapshot na selekcji
+ odświeżenie timerem ~0.5–1 s gdy otwarty (NIE Event Tick). Portret: jeden capture, niska cadence,
wyłączany po zamknięciu. Kilka odczytów getterów + set text per refresh. Koszt lokalny, pomijalny.

═══════════════════════════════════════════════════════════
# PODZIAŁ C++ / BLUEPRINT
═══════════════════════════════════════════════════════════
## C++ (już istnieje — TYLKO czytane, zero nowej logiki):
- Gettery `MaslowBiologicalComponent` (głód/sen/temp/HP/priorytet), `BodyConditionComponent` (ciało/zmysły),
  `InventoryComponent` (kontenery). Inspektor NIE liczy nic — czyta.

## Blueprint/UMG (cała robota tu):
- Shell inspektora (głowa + tab switcher), zakładki, kłódki, skin Dark Souls.
- Pull na selekcji + timer-refresh. Portret (SceneCapture + render target + materiał).
- Sprzątanie długów (Hello/ST_Time/EditableTextBox/Tick).

═══════════════════════════════════════════════════════════
# WERYFIKACJA (Monolith live, PIE — twarde, zero placeholderów)
═══════════════════════════════════════════════════════════
1. **Jeden inspektor:** klik na NPC → JEDEN widget (nie dwa pływające). Głowa + zakładki 1–4 aktywne, 5–11 kłódka.
2. **Single source:** zakładka Stan i głowa pokazują liczby z C++ getterów (`GetFatigueState`/`AmbientTemp`/
   `EHungerPhase`...), zgodne z live odczytem komponentu — NIE z BP `ST_Metabolism` (chyba że pre-flight #1
   potwierdzi, że to lustro).
3. **Pull, nie tick:** brak Event Tick w grafie inspektora; refresh przez timer/selekcję (sprawdź, że dane
   aktualizują się przy otwartym oknie, ale nie per-frame).
4. **Portret żyje:** pokazuje zaznaczonego NPC; capture wyłączony po zamknięciu (`bCaptureEveryFrame=false`).
5. **Długi zniknęły:** brak „Hello" na ekranie przy NPC=None; brak `ST_Time`; readouty to `TextBlock`.
6. **Kłódki:** zakładki 5–11 czytają się jako „dochodzi (L#)", nie crashują, nie pokazują zer-duchów.
7. **Kamera:** klik dalej locka `TrackedNPC` (regresja-check — nie ruszaliśmy kamery).

═══════════════════════════════════════════════════════════
# CO TEN ETAP ROBI / CZEGO NIE
═══════════════════════════════════════════════════════════
✅ ROBI: scalenie dwóch okien w jeden inspektor (głowa + 11 zakładek, 4 żywe + 7 kłódka), single-source
   C++, pull-snapshot, portret wariant A, skin Dark Souls, sprzątanie długów, podpięcie Maslowa (głód/sen/
   temp/HP/priorytet) którego dziś inspektor NIE czyta.
❌ NIE ROBI:
- Zakładki 5–11 jako treść (Osobowość/Relacje/Kontrakt/Wiedza/Log/Umiejętności/Tech) → kłódki, budowane gdy
  systemy ożyją (osobne gate'y per system).
- Feedu zdarzeń (persistent HUD, poziom świata) → osobny tor, osobny gate (`UWorldEventBus`).
- HUD_CONSOLIDATION (zegar/własność RTS HUD) → osobny, istniejący gate.
- Trybu Character (Soulslike HUD wcielonego gracza) → osobny tor.

═══════════════════════════════════════════════════════════
# REGUŁA PATCHOWANIA
═══════════════════════════════════════════════════════════
Reużyj `WBP_NPC_Inspector` jako shell + `WBP_Tab_Body` jako zakładkę Ciało. Metabolizm z `WBP_NPC`
przenieś jako zakładkę Stan (przepisując źródło na C++ gettery). NIE regeneruj widgetów od zera.
Sprzątanie długów = usuwanie węzłów, nie przebudowa. ⚠️ Zapis defaultów/assetów BP ostrożnie
(RF_Transient strip przez Monolith) — jeśli ryzyko, zostaw Ctrl+S Szymonowi.

═══════════════════════════════════════════════════════════
# 🔑 INSTRUKCJA DLA CLAUDE CODE — PRE-FLIGHT (POTWIERDŹ PRZED EDYCJĄ)
═══════════════════════════════════════════════════════════
1. 🔴 **ROZSTRZYGNIJ FLAGĘ (najpierw, blokuje zakładkę Stan):** czy `BP_NPC_Character.MetabolismStats`
   (`ST_Metabolism`) to LUSTRO C++ Maslowa (jeden stan, BP mirror), czy DRUGI, rozłączny kanał liczony
   w BP? Prześledź, kto zapisuje `ST_Metabolism` — C++ pcha do BP, czy BP liczy sam? Jeśli DRUGI MÓZG →
   FLAGUJ (jak Maslow/BT Bridge), zanim cokolwiek zbudujesz. Inspektor MA czytać C++, nie BP-kanał.
2. POTWIERDŹ ścieżki getterów: dokładne nazwy/sygnatury dla głodu (faza+rezerwy), Hydration, temp,
   `GetFatigueState`, `GetActionableNeed`, HP. Realna lista `EHungerPhase` i `EFatigueState`.
3. POTWIERDŹ jak inspektor zna strefę NPC (do zakładki Ogólna „gdzie") — przez `CaldrethZone` cache czy
   `GetZoneAtLocation`. I czy istnieje getter use-name/true-name na NPC (jeśli nie — flaguj, true-name to
   mechanika społeczna, może jeszcze nie istnieć → pole kłódka).
4. POTWIERDŹ scene-capture: czy istnieje JAKIKOLWIEK `SceneCaptureComponent2D`/render target/materiał
   portretowy, czy budujemy od zera (recon blok 6).
5. Patch, nie regeneracja. STOP po weryfikacji bloków 1–7.
