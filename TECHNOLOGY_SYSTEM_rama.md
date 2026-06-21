# SYSTEM TECHNOLOGII — RAMA PROJEKTOWA (notatki, NIE design gate)
> Status: WIZJA / RAMA do przemyślenia. To NIE jest jeszcze specyfikacja do kodowania.
> Cel dokumentu: utrwalić schemat odkrywania technologii, żeby nowa sesja miała pełny obraz.
> Data szkicu: 2026-06-19. Zakres docelowy pierwszego łuku: MAŁPA → PIERWSZY SIEW (osiadłość),
> NIE małpa→Rzym (świadome zawężenie — pierwszy siew to kompletny, grywalny pierwszy rozdział).

═══════════════════════════════════════════════════════════
# PROBLEM, KTÓRY ROZWIĄZUJEMY
═══════════════════════════════════════════════════════════
Chcemy emergentnego rozwoju technologicznego — NPC dochodzą do odkryć SAMI, kierowani
potrzebami (Maslow) i cechami (OCEAN), a NIE przez sztywne drzewko jak w Civilization.
Ale w pełni emergentna fizyka (NPC losowo eksperymentuje, świat liczy wszystko) jest
nieobliczalna dla 500+ NPC i niesterowalna.

ROZWIĄZANIE: hybryda przesunięta ku emergencji — rzeczy mają WŁAŚCIWOŚCI (tagi), receptury
żądają FUNKCJI (nie konkretnych przedmiotów), NPC eksperymentuje gdy ma POTRZEBĘ + CECHĘ,
a odkrycie powstaje gdy tagi się zgadzają. Produkt odkrycia dostaje WŁASNE tagi → staje się
składnikiem wyższych receptur → graf rośnie sam (A+B → A+B+C → ...).

═══════════════════════════════════════════════════════════
# RDZEŃ RAMY — JEDNO ZDANIE
═══════════════════════════════════════════════════════════
Rzeczy mają TAGI. Receptury żądają TAGÓW (nie rzeczy). Produkt dostaje własne TAGI → staje się
składnikiem. NPC próbuje gdy ma POTRZEBĘ + CECHĘ. Pasują tagi → odkrycie. Powtórzenie → nauka.
Ten sam mechanizm skaluje od kamienia po pierwszy siew.

═══════════════════════════════════════════════════════════
# WARSTWA 1 — RZECZY MAJĄ TAGI (właściwości, nie nazwy)
═══════════════════════════════════════════════════════════
Każdy obiekt w świecie ma zestaw tagów-właściwości. To jest też mechanika "dotyka rzeczy
i sprawdza co to jest — mech miękki" (poznawanie świata = odczyt tagów).

Przykłady (ilustracyjne, do rozbudowy):
| Obiekt | Tagi (właściwości) |
|---|---|
| Mały kamień | TWARDY, CIĘŻKI, RZUCALNY |
| Duży kamień | TWARDY, CIĘŻKI, NIERUCHOMY (kowadło) |
| Ostry kamień | OSTRY, TWARDY, RZUCALNY (PRODUKT odkrycia) |
| Patyk | DŁUGI, SZTYWNY, ŁAMLIWY |
| Lina | WIĄŻE, GIBKIE, DŁUGIE |
| Żywica | WIĄŻE, LEPKIE |
| Ścięgno | WIĄŻE, GIBKIE |
| Pnącze | WIĄŻE, GIBKIE, DŁUGIE |
| Mech | MIĘKKI, CHŁONNY |
| Włókno roślinne | WIĄŻE (po obróbce), CIENKIE |

🔑 KLUCZOWA OBSERWACJA (źródło całej skalowalności):
"Tak samo można związać coś liną jak i żywicą." → NPC nie łączy LINY z kamieniem, łączy
coś z tagiem WIĄŻE z czymś z tagiem OSTRY/CIĘŻKI. Lina, żywica, ścięgno, pnącze — wszystkie
mają tag WIĄŻE. Jedna receptura obsługuje wszystkie. To jest cały sekret.

═══════════════════════════════════════════════════════════
# WARSTWA 2 — RECEPTURY ŻĄDAJĄ FUNKCJI, NIE PRZEDMIOTÓW
═══════════════════════════════════════════════════════════
Receptura to UKRYTY WARUNEK na tagach, NIE pozycja w menu. NPC jej NIE zna z góry.

❌ ŹLE (sztywne, nieskalowalne — droga Civ):
   "kamień + lina + patyk → dzida"  (osobna receptura dla każdego materiału = setki receptur)

✅ DOBRZE (funkcyjne, skalowalne):
   RECEPTURA "broń obuchowa/drzewcowa" = [coś OSTRE] + [coś WIĄŻE] + [coś DŁUGIE]
   → lina ALBO żywica ALBO ścięgno ALBO pnącze (wszystkie WIĄŻĄ) działają w tej samej recepturze
   → jedna receptura, dziesiątki kombinacji materiałów

Przykłady receptur (ilustracyjne):
| Receptura | Wymagane tagi | Produkt (i jego nowe tagi) |
|---|---|---|
| Ostry kamień | [TWARDY/RZUCALNY] ⊕ [TWARDY/NIERUCHOMY] (uderz o kowadło) | ostry kamień: OSTRY, TWARDY |
| Broń obuchowa | [OSTRY] ⊕ [WIĄŻE] ⊕ [DŁUGI] | dzida: BROŃ, DŁUGI, OSTRY |
| Włókno | [roślina włóknista] ⊕ obróbka (czas/akcja) | włókno: WIĄŻE, CIENKIE |
| Ogień | [SUCHY] ⊕ [SUCHY] ⊕ akcja (tarcie) + warunek (nie mokro) | ogień: GRZEJE, ŚWIECI, GOTUJE |
| Pieczone mięso | [surowe mięso] ⊕ [GRZEJE/ogień] | jedzenie: ODŻYWCZE, BEZPIECZNE (mniej PoisonChance) |

═══════════════════════════════════════════════════════════
# WARSTWA 3 — GRAF ROŚNIE SAM (A+B → A+B+C)
═══════════════════════════════════════════════════════════
Pytanie użytkownika: "na początku A+B, ale dalej A+B+C — jak to zrobić?"
Odpowiedź: PRODUKT ODKRYCIA DOSTAJE WŁASNE TAGI → automatycznie staje się składnikiem wyższych receptur.

Łańcuch:
1. A+B:      kamień(TWARDY) ⊕ kamień(TWARDY) → ostry kamień [nowy tag: OSTRY]
2. A+B+C:    ostry kamień(OSTRY) ⊕ lina(WIĄŻE) ⊕ patyk(DŁUGI) → dzida [nowe tagi: BROŃ, DŁUGI]
3. A+B+C+D:  dzida(DŁUGI) ⊕ ... → jeszcze wyższa receptura

NIE projektujemy "poziomów technologii". Graf rośnie sam, bo każdy produkt jest potencjalnym
składnikiem następnego. "Kamień na linie = broń, której można się uczyć" (przykład użytkownika)
= ostry kamień + lina spotykają się, bo oba mają wymagane tagi. Nikt nie pisał "odblokuj broń po dzidzie".

═══════════════════════════════════════════════════════════
# WARSTWA 4 — JAK NPC DOCHODZI DO ODKRYCIA (potrzeba + cecha + rzut + nauka)
═══════════════════════════════════════════════════════════
NPC nie zna receptur. Eksperymentuje, gdy ma POWÓD i SKŁONNOŚĆ:

1. POTRZEBA (Maslow): głód → chce upolować → królik ucieka → porażka → brak broni = problem.
2. PRÓBA: cecha OCEAN (ciekawość/otwartość/desperacja) + RZUT KOŚCIĄ → czy w ogóle spróbuje
   akcji "użyj A na B" / "uderz A o B" / "rzuć A". Sumienny zbieracz NIE eksperymentuje (robi
   co umie). Otwarty/zdesperowany uderza kamieniem o kamień ("a nuż").
3. ODKRYCIE: jeśli tagi pasują do jakiejś (ukrytej) receptury → produkt powstaje, NPC ZNA recepturę.
4. NAUKA: powtórzenie → szybciej, pewniej, mniejszy rzut potrzebny (mistrzostwo, L4). "Będzie mógł
   się jej uczyć" (przykład użytkownika).

Pętla pierwszego plastra (PIERWSZY ETAP do zakodowania):
głód → polowanie na królika → królik ucieka → zmęczenie/porażka → eksperyment (kamień o kamień)
→ ostry kamień → następne polowanie się udaje (rzut z bronią > rzut bez broni) → 🍖

═══════════════════════════════════════════════════════════
# SPIĘCIE Z ISTNIEJĄCĄ PIRAMIDĄ MASLOWA (nic nie jest osobnym systemem)
═══════════════════════════════════════════════════════════
Rama technologii NIE jest osobnym "systemem technologii" — żywi się resztą:
- POTRZEBA eksperymentu = L1 (głód napędza polowanie/narzędzia).
- EKSPERYMENT / Eureka = zalążek L5 (Punkty Innowacji, odkrycia).
- NAUKA przez powtórzenie = mistrzostwo L4 (Mastery, tag "Mistrz X").
- DZIELENIE SIĘ odkryciem wieczorem = L3 (Campfire Sync — ktoś odkrył włókna, ktoś ma kamień,
  dzida powstaje gdy wiedza grupy się spotka). Przykład użytkownika: "jeśli kobiety nie odkryły
  lin i włókien, nie będzie dzidy" = odkrycia są ROZPROSZONE w grupie, propagują przez Campfire.
- CECHY decydujące o eksperymentowaniu = OCEAN (już w planie L3).

═══════════════════════════════════════════════════════════
# PIERWOTNE CZYNNOŚCI (baza, której NPC potrzebuje PRZED technologią)
═══════════════════════════════════════════════════════════
Z rozmowy — NPC potrzebuje najpierw bardzo pierwotnych akcji (to fundament pod pętlę plastra):
- Zbieranie, polowanie, chodzenie, bieganie, uciekanie, spanie (sen JUŻ jest).
- System komfortu i skuteczności (temperatura/AmbientTemp JUŻ jest; komfort wpływa na skuteczność).
- EQS od Safe Zone: NPC oznacza swój hub, robi search wokół ("idę w stronę słońca"), szuka jedzenia,
  odkrywa świat, dotyka rzeczy i sprawdza co to jest (= odczyt tagów właściwości).
- Maslow L1, każdy dla siebie (jedzenie własne, przed L3 współdzieleniem).
- Przykład pętli porażki: człowiek widzi królika → "poluję" → królik ucieka → człowiek się zmęczył
  → królik poza polem widzenia → PORAŻKA → wniosek (potrzebuję broni / lepszej kondycji).

═══════════════════════════════════════════════════════════
# OTWARTE PYTANIA (do rozstrzygnięcia w nowej sesji, PRZED design gate)
═══════════════════════════════════════════════════════════
1. JAK NPC fizycznie wykonuje "uderz A o B"? Akcja ogólna w Behavior Tree ("use item on item")
   z parametrami? Jak wygląda w BT/EQS? (C++ liczy wynik z tagów, BP gra animację — wzorzec sesji.)
2. Gdzie żyją TAGI? Na FoodItemRow/ItemRow (DataTable)? Nowy USTRUCT FItemProperties z FGameplayTagContainer?
   (GameplayTags to natywny system UE — pasuje idealnie do "tagów właściwości".)
3. Gdzie żyją RECEPTURY? DataTable receptur (każdy wiersz = wymagane tagi + produkt + tagi produktu)?
   Data-driven, zero hardkodu (zasada projektu).
4. Jak działa "odkrycie" obliczeniowo dla 500 NPC? NPC NIE skanuje wszystkich receptur co próbę —
   trzeba taniego dopasowania (indeks po tagach? tylko gdy akcja "użyj A na B" jawnie wywołana?).
5. Rzut kością + cecha: jaka formuła? (np. szansa = bazowa × MnożnikCechy × (1 + Desperacja)).
6. Nauka: jak reprezentujemy "NPC zna recepturę X na poziomie Y"? Per-NPC zbiór znanych receptur
   + licznik powtórzeń → skill.
7. Propagacja wiedzy (Campfire): jak odkrycie jednego NPC trafia do grupy? (już szkic w designie L3.)
8. WERYFIKACJA pierwszego plastra: jakie twarde liczby z logu potwierdzą, że pętla działa?
   (głód rośnie → polowanie → log porażki → eksperyment → produkt OSTRY powstaje → kolejne polowanie sukces.)

═══════════════════════════════════════════════════════════
# REKOMENDOWANY PIERWSZY KROK (gdy wrócimy do tego)
═══════════════════════════════════════════════════════════
NIE budować całej ramy naraz (poziomo = miesiące bez widocznego życia = śmierć projektu).
Budować PIONOWO — jeden cienki plaster od potrzeby do odkrycia, doprowadzony do działania
na żywym NPC, zweryfikowany twardymi liczbami (dyscyplina całej dotychczasowej pracy).

PIERWSZY PLASTER: głód → polowanie na królika → porażka (królik ucieka, NPC się męczy)
→ eksperyment (kamień o kamień, kierowany cechą+potrzebą) → ostry kamień (produkt z tagiem OSTRY)
→ następne polowanie udane (broń poprawia rzut). Gdy TO żyje na ekranie — rama jest sprawdzona
i ten sam wzorzec niesie aż do pierwszego siewu.

SPRAWDZIAN RAMY (do zrobienia na papierze przed kodem): weź 3-4 dowolne odkrycia z głowy
(rozpalenie ognia, wyplecenie kosza, upieczenie mięsa, obróbka włókna) i sprawdź, czy KAŻDE
pasuje do schematu tag→funkcja→produkt. Jeśli któreś nie pasuje → rama ma lukę, znaleźć ją
TERAZ (na papierze), nie w kodzie.

═══════════════════════════════════════════════════════════
# STAN PROJEKTU W MOMENCIE TEGO SZKICU (dla kontekstu nowej sesji)
═══════════════════════════════════════════════════════════
GOTOWE i zweryfikowane (commity w gicie):
- Silnik snu ETAP 1 (narastanie zmęczenia, drabina FatigueState).
- Silnik snu ETAP 2 (skutki: mgła liniowa, mikrosen, omdlenie z blokadą AI; + StartSleep/StopSleep
  /reset/Rested scalone; 2 bugi naprawione).
- AmbientTemp rdzeń strefowy (FZoneDef.BaseTemp, cache stref, sprzężenie otoczenie→ciało 3 reżimy,
  hipotermia stopniowa, cold-burn split — OŻYWIŁ 3 martwe od początku systemy temperatury).
NASTĘPNE w kolejce (przed/obok technologii):
- Warstwa doby (DayNightTempOffset z BP_DayNightCycle — wymaga postawienia zegara na CaldrethMap;
  źródło: SunIntensity/MaxSunIntensity; pytanie otwarte: czy SunIntensity niesie sezon → model
  proporcjonalny łapie pory roku za darmo). Zegar naprawi też "jeden zegar" snu (fallback znika).
- Caldreth spawner (zaludnienie wyspy NPC + NavMesh).
- NPCRegistry (keystone pod L3/L4/L5 + cache stref).
Architektura: C++ mózg / Blueprint ciało; bez Event Tick (timery/BT/EQS); data-driven (DataTable/
DataAsset); IsValid wszędzie; UE_LOG na przejściach stanów; patch nie regeneruj; staged delivery
z weryfikacją twardymi liczbami z logu PRZED zamknięciem etapu.
