# HUD KONSOLIDACJA — DESIGN GATE (jeden persistent HUD RTS, właściciel = kontroler)
> Zatwierdzony projekt do implementacji (architekt + Szymon, 2026-06-21).
> To robota BP/UMG (ciało), NIE C++. Gate definiuje WŁASNOŚĆ, KIERUNEK DANYCH, CYKL ŻYCIA — reszta to edytor.
> Bazuje na RECON HUD (Monolith, live, CaldrethMap) — mapa wiringu potwierdzona.
>
> 🔴 SEDNO: re-own to POŁOWA. Druga połowa = ODWRÓCENIE KIERUNKU DANYCH (push→pull).
> Dziś BP_DayNightCycle WPYCHA dane do widgetu → aktor trzyma HUD. Cel: HUD CIĄGNIE → aktor = czyste źródło.

═══════════════════════════════════════════════════════════
# STAN WEJŚCIOWY (z reconu — twarde)
═══════════════════════════════════════════════════════════
- World Settings CaldrethMap: GM_RTSGameMode / PC_RTSGameMode / BP_RTSCamera ✅ (kontroler RTS żyje).
- `WBP_DebugInfo` (UserWidget, has_tick=false): pola CurrentTime, Day, Pora, SunIntens, MoonIntens;
  ma `DayNightCycleRef : Object` (instance-editable) + `Get_CurrentTime_Text`. ŻYWY na ekranie.
- Tworzy/trzyma/karmi go `BP_DayNightCycle` (aktor środowiskowy): BeginPlay → Create WBP_DebugInfo →
  Set DebugUI → AddToViewport; funkcje THE ATMOSPHERE (3×)/Debug (2×) wpisują wartości (PUSH).
- `PC_RTSGameMode.BeginPlay`: tylko Set bShowMouseCursor → Set bEnableClickEvents. Zmienna
  `HUDReference` (typ WBP_DebugInfo_C) = **sierota, 0 referencji** (nigdy nie tworzona).
- `WBP_NPC` / `WBP_NPC_Inspector`: klik-on-demand inspektor (ActiveNPCWindow/NPCInspectorWidget),
  DZIAŁA. To OSOBNA warstwa (pojawia się na zaznaczenie). **NIE RUSZAMY.**
- `WBP_HUD` (Soulslike): obcy asset, referencowany tylko przez BP_SoulslikeCharacter. **Poza zakresem.**

═══════════════════════════════════════════════════════════
# DECYZJE PROJEKTOWE (zatwierdzone — NIE zmieniać bez architekta)
═══════════════════════════════════════════════════════════
| # | Decyzja | Wartość |
|---|---|---|
| D-REUSE | Asset HUD | **Rename `WBP_DebugInfo` → `WBP_RTSHud`** (recykling, nie nowy asset). |
| D-OWNER | Właściciel | **`PC_RTSGameMode` tworzy/trzyma/dodaje** HUD w BeginPlay → podpina istniejącą sierotę `HUDReference` (retype na WBP_RTSHud_C). |
| D-DATADIR | Kierunek danych | **PULL (HUD ciągnie), NIE push.** Aktor przestaje wpisywać w widget. To jest właściwa naprawa ownership. |
| D-SOURCE | BP_DayNightCycle | **Czyste źródło danych** — przestaje dotykać viewportu (usuń Create/Set DebugUI/AddToViewport + write'y w THE ATMOSPHERE/Debug). Zostaje zegar + ekspozycja czasu. |
| D-REFRESH | Odświeżanie | **Timer ~1s w widgecie** (Construct → looping), pull + set text. **ZERO Event Tick.** |
| D-CLOCK | Pierwszy element | **Zegar (godzina/dzień)** = pierwszy legalny element HUD RTS. Reszta (staty osady itd.) = miejsce na przyszłość. |
| D-STRIP | Co zostaje | Zostaw clock (CurrentTime, Day, Pora). Surowy debug (SunIntens/MoonIntens) USUŃ albo schowaj — to był debug, nie HUD. |
| D-INSPECTOR | WBP_NPC | **NIETKNIĘTY.** Persistent HUD ≠ on-demand inspektor. Dane per-NPC (głód/sen/temp/HP) żyją w inspektorze, osobna warstwa. |

═══════════════════════════════════════════════════════════
# CZĘŚĆ A — PRZENIESIENIE WŁASNOŚCI (kto tworzy HUD)
═══════════════════════════════════════════════════════════
## PC_RTSGameMode.BeginPlay (DODAJ po istniejących Set kursora):
```
... Set bShowMouseCursor → Set bEnableClickEvents
   → CreateWidget(class=WBP_RTSHud, owner=self)
   → Set HUDReference = (utworzony widget)         // sierota przestaje być sierotą
   → AddToViewport (HUDReference)
```
- `HUDReference` retype: WBP_DebugInfo_C → **WBP_RTSHud_C**.
- AddToViewport RAZ. Guard: jeśli HUDReference już IsValid → nie twórz drugiego (idempotencja przy ew. re-BeginPlay).

## BP_DayNightCycle.BeginPlay (USUŃ):
```
... (łańcuch Set zegara/słońca — ZOSTAJE) → THE CALLENDAR
   → ❌ Create WBP_DebugInfo   → ❌ Set DebugUI   → ❌ AddToViewport
```
- Usuń też zmienną `DebugUI` z aktora (po usunięciu write'ów).
- Aktor NIE wie już nic o HUD. To jest test poprawności: po fixie `BP_DayNightCycle` ma ZERO węzłów widgetowych/viewportowych.

═══════════════════════════════════════════════════════════
# CZĘŚĆ B — ODWRÓCENIE KIERUNKU DANYCH (push → pull) — RDZEŃ
═══════════════════════════════════════════════════════════
Dziś: aktor robi `Get DebugUI` + wpisuje text-boxy (THE ATMOSPHERE 3× / Debug 2×). To USUWAMY.
Cel: HUD sam się odpytuje o czas.

## WBP_RTSHud — self-resolve źródła (Construct, RAZ):
```
Construct → GetAllActorsOfClass(BP_DayNightCycle) → [0] → IsValid?
   tak  → Set DayNightCycleRef = znaleziony aktor
   nie  → DayNightCycleRef = None (HUD pokaże "--:--", patrz edge-case)
```
- `DayNightCycleRef` JUŻ ISTNIEJE w widgecie — reużywamy, nie dodajemy.
- HUD zna klasę BP_DayNightCycle (akceptowalne — HUD wyświetla stan świata). Zegar to singleton świata.

## BP_DayNightCycle — usuń write'y do widgetu:
- THE ATMOSPHERE / Debug: wytnij wszystkie `Get DebugUI → Set Text (CurrentTime/Day/...)`.
- Zostaje sama logika zegara + ekspozycja czasu (aktor już wystawia SunIntensity dla systemu temp przez
  reflection — ten sam wzorzec „aktor jako źródło"). Jeśli `Get_CurrentTime_Text` żyło po stronie widgetu
  i czytało aktora — zostaje; jeśli liczyło z DebugUI-push — przenieś odczyt na DayNightCycleRef.

═══════════════════════════════════════════════════════════
# CZĘŚĆ C — ODŚWIEŻANIE (timer, nie tick)
═══════════════════════════════════════════════════════════
```
Construct → (po self-resolve) → Set Timer by Function Name "UpdateClock", time=1.0, looping=true
UpdateClock():
   IsValid(DayNightCycleRef)?
     tak → odczytaj czas/dzień (Get_CurrentTime_Text / gettery aktora) → Set Text CurrentTime/Day/Pora
     nie → Set Text "--:--" (degradacja, nie crash)
Destruct → Clear Timer "UpdateClock"
```
- 1s wystarcza dla zegara (gracz widzi minuty). ZERO Event Tick (reguła projektu).
- Alternatywa (jeśli aktor i tak nadaje zdarzenie zmiany czasu): event dispatcher OnTimeChanged → bind
  w widgecie, zero pollingu. NA TERAZ pull-timer (prostsze, self-contained, niezależne od wnętrza aktora).
  Dispatcher = możliwy upgrade dla zdarzeń dyskretnych (świt/zmierzch/zmiana dnia) później.

═══════════════════════════════════════════════════════════
# 🛡️ EDGE-CASES (fail-safe)
═══════════════════════════════════════════════════════════
- **EC-1: brak BP_DayNightCycle na mapie** (mapy bez zegara — istnieją, sen/temp lecą fallbackiem).
  GetAllActorsOfClass → pusto → DayNightCycleRef None → UpdateClock pokazuje "--:--". Zero crasha.
  IsValid przed KAŻDYM odczytem aktora.
- **EC-2: kolejność BeginPlay** (kontroler vs aktor). Aktory są w świecie w BeginPlay — GetAllActorsOfClass
  w Construct widgetu (po AddToViewport z BeginPlay kontrolera) znajdzie już-osadzony zegar. Jeśli timing
  okaże się wyścigiem → retry self-resolve w pierwszym UpdateClock (IsValid-guard i tak chroni).
- **EC-3: cykl życia widgetu** — AddToViewport raz, ref w HUDReference, RemoveFromParent + Clear Timer na
  EndPlay/Destruct. Brak duplikatów po re-PIE.
- **EC-4: redirector po rename** — rename WBP_DebugInfo→WBP_RTSHud zostawia redirector; HUDReference retype
  + usunięcie referencera w BP_DayNightCycle muszą wyczyścić, by nie wisiał martwy link. Fixup redirectors.

═══════════════════════════════════════════════════════════
# PERF
═══════════════════════════════════════════════════════════
Jeden widget gracza, timer 1s, kilka odczytów + set text. **NIE jest w ścieżce 500 NPC** — to HUD lokalny.
Timer (nie Event Tick) zgodny z regułą projektu. Koszt pomijalny.

═══════════════════════════════════════════════════════════
# WERYFIKACJA (Monolith live, PIE)
═══════════════════════════════════════════════════════════
1. **Sierota podpięta:** PC_RTSGameMode.HUDReference != None (był None), klasa = WBP_RTSHud_C, in_viewport=True.
2. **Aktor odpięty:** BP_DayNightCycle nie ma już DebugUI / nie wisi z niego żaden widget na viewport
   (live: jedyny HUD na ekranie należy do kontrolera, nie do aktora).
3. **Zegar żyje i ciągnie:** CurrentTime/Day pokazują poprawny czas i ADWANSUJĄ w PIE (pull-timer działa).
4. **EC-1 degradacja:** mapa bez BP_DayNightCycle (lub usuń aktora w teście) → HUD "--:--", zero crasha/spamu.
5. **Inspektor nietknięty:** klik na NPC → WBP_NPC/WBP_NPC_Inspector dalej działają (regresja-check).
6. **Brak ducha:** żaden drugi widget HUD nie wisi (debug zegara zniknął, nie ma dwóch nakładek).

═══════════════════════════════════════════════════════════
# CO TEN ETAP ROBI / CZEGO NIE
═══════════════════════════════════════════════════════════
✅ ROBI: rename→WBP_RTSHud, własność→PC_RTSGameMode (sierota podpięta), kierunek danych push→pull,
   BP_DayNightCycle→czyste źródło (zero viewportu), zegar jako pierwszy element HUD, timer-refresh, EC-1..4.
❌ NIE ROBI:
- Treść per-NPC (głód/sen/temp/HP) → to ISTNIEJĄCY inspektor WBP_NPC (osobna warstwa on-demand), nie ten HUD.
- WBP_HUD / Soulslike → poza zakresem (obcy system).
- Staty osady / tryby / alerty na persistent HUD → miejsce zostawione, treść później.
- Event dispatcher OnTimeChanged → możliwy upgrade later (teraz pull-timer wystarcza).

═══════════════════════════════════════════════════════════
# 🔑 INSTRUKCJA DLA CLAUDE CODE — POTWIERDŹ PRZED EDYCJĄ
═══════════════════════════════════════════════════════════
Recon zrobiony — masz mapę. Przed edycją grafów (świeży edytor, czysty bind 9316 — attempt 1):
1. POTWIERDŹ jak `Get_CurrentTime_Text` dziś liczy czas: z DebugUI-push czy z odczytu aktora? Jeśli z pushu
   — przepisz na odczyt z DayNightCycleRef (pull). Podaj gettery czasu/dnia na BP_DayNightCycle (jakie pola
   wystawia — CurrentTime? Day? Pora?).
2. Rename asset przez edytor (redirectors), retype HUDReference, fixup referencerów. NIE zostaw martwego linku.
3. ⚠️ DYSCYPLINA ZAPISU: zapis defaultów/assetów BP — uwaga na RF_Transient strip przez Monolith. Edycje
   grafu/struktury rób ostrożnie; jeśli ryzyko — flaguj i zostaw zapis Szymonowi (Ctrl+S ręcznie).
4. Jeśli cokolwiek w żywym grafie przeczy temu gate'owi (np. zegar advansuje przez Event Tick, albo
   Get_CurrentTime_Text ma inny kontrakt) — FLAGUJ przed edycją, nie zgaduj.
5. Patch, nie regeneracja widgetu. Reużyj DayNightCycleRef + Get_CurrentTime_Text + pola clock. STOP po
   weryfikacji bloków 1–6.
