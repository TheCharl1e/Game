# NPC — POGŁĘBIENIE: człowieczeństwo, dzieciństwo, maski potrzeb (RAMA / WIZJA)
> Status: WIZJA/RAMA — **NIE design gate, NIE do kodowania.** Trzy koncepty pogłębiające rdzeń
> Maslow+OCEAN, zrodzone w sesji L3-02 (2026-06-20). Każdy: co robi · jak (szkielet) · gdzie się wpina ·
> haczyki. Wracają jako osobne gate'y, GDY dojrzeją ich zależności. Cel dokumentu: nie zgubić pomysłów.
> Wzorzec opisu jak DESIGN_how_it_works.md — docelowo można je tam wmerge'ować jako "haczyki do rozbudowy".

═══════════════════════════════════════════════════════════
# 1. DZIECIŃSTWO JAKO ZALĄŻEK OSOBOWOŚCI (archetype origin) · ⬜
═══════════════════════════════════════════════════════════
**Co robi:** archetyp OCEAN nie jest nadawany z palca designera — jest UFORMOWANY wczesnym życiem.
To odpowiedź na pytanie "skąd NPC bierze charakter".

**Jak (szkielet):**
- Pełna wersja (daleka): faktyczna symulacja dzieciństwa NPC.
- Tani odpowiednik (realny, D&D "background"): **proceduralny backstory-roll przy narodzinach** —
  kilka zdarzeń wczesnego życia (deterministycznie od `NPCId` + strefa urodzenia) przesuwa 5 floatów
  OCEAN od średniej archetypu. Powtarzalne (debug), unikalne per NPC.
- **Spina z geografią Caldreth:** dzieciństwo w górach (Mountain/AshSlope, twarde) → wyższa odporność /
  niższa ugodowość; nizinne/Oasis → łagodniejszy charakter. **Geografia formuje charakter** — wprost
  zgodne z naczelną zasadą projektu "geografia napędza zachowanie".

**Gdzie się wpina:** zasila `float[5]` OCEAN na `UNPCIdentityComponent` (L3-02). Wymaga **spawnera**
(zaludnienie + strefa urodzenia jako wejście).

**Haczyki do rozbudowy:**
- Rodzeństwo dzieli zdarzenia → podobne OCEAN (klastry rodzinne, most do pokrewieństwa L3).
- Trauma dzieciństwa = startowy podbity Neuroticism (most do dryfu osobowości).
- Pamięć miejsca urodzenia (most do terytorialności / Safe Zone L3).

**Zależność / status:** ⬜ czeka na spawner + strefy urodzenia.

═══════════════════════════════════════════════════════════
# 2. CZŁOWIECZEŃSTWO — RZUT OBRONNY PRZECIW INSTYNKTOWI (6. oś / meta-warstwa) · ⬜
═══════════════════════════════════════════════════════════
**Co robi:** zdolność NPC do PRZECIWSTAWIENIA SIĘ instynktowi w imię idei/wartości. Warstwa
"człowiek, nie zwierzę" — szósty wymiar PONAD OCEAN. To jest kręgosłup tematyczny gry.

**Jak (szkielet):**
- Per-NPC stat: **Humanity** (nazwa robocza "człowieczeństwo"; ang. Humanity / Conviction / Will — do ustalenia).
- Gdy instynkt (niższy poziom Maslowa, `EvaluateCurrentNeed`) dyktuje akcję X — ukraść, uciec, zjeść
  trupa — pada **DODATKOWY rzut obronny**: szansa override w imię wartości.
- `SzansaOverride = f(Humanity, AltitudeMaslow, SiłaIdei)`. **Kluczowe: AltitudeMaslow** — im WYŻEJ
  NPC aktualnie jest (im więcej niższych potrzeb zaspokojonych), tym większa szansa. Głodujący
  (L1 dominuje) ≈ 0 override (instynkt rządzi). Samorealizujący się (L5) = wysoka zdolność override.
- **Teza:** moralność/idealizm to LUKSUS zaspokojonych potrzeb. (Zgodne z filozofią samego Maslowa.)

**Gdzie się wpina:**
- Modyfikator NAD arbitrażem Maslowa (`EvaluateCurrentNeed` / logika override L0–L5).
- **true-name / Mowa Pradawna** (nie skłamiesz = zasada ponad interes) = człowieczeństwo W AKCJI.
- **Grey Spring** (erozja JA / hive-mind) = PRZECIWIEŃSTWO — obniża Humanity, zejście do czystego
  instynktu / roju. (Naturalna oś napięcia: idea ↔ utrata siebie.)
- Reputacja L4 / sprawiedliwość L5: czyny ponad instynkt (poświęcenie, uczciwość mimo głodu) = skok reputacji.
- Kanibalizm / kradzież jako tabu (antropolog): złamanie INSTYNKTEM mimo wysokiego Maslowa = silniejsza hańba.

**Haczyki do rozbudowy:**
- Humanity rośnie przez czyny zgodne z ideą (jak mistrzostwo L4); spada przez kapitulacje przed instynktem.
- Idee/wartości jako per-NPC zbiór (most do OCEAN Openness / macierzy preferencji L4).
- Konflikt wartości: dwie idee naraz → rozdarcie (dramatis personae emergentny).

**Zależność / status:** ⬜ DUŻY gate w przyszłości — czeka aż dojrzeją: pełny arbitraż Maslowa
(Altitude), system wartości/idei, oraz Grey Spring (jego przeciwwaga).

═══════════════════════════════════════════════════════════
# 3. POTRZEBA ODCZUWANA vs STAN PRAWDZIWY + MASKI (arbitraż "którą najpierw") · ⬜
═══════════════════════════════════════════════════════════
**Co robi:** rozdziela SYGNAŁ potrzeby (na który NPC reaguje) od PRAWDZIWEGO stanu (metabolizm pod
spodem). Potrzeby da się tymczasowo ZAMASKOWAĆ tym, co pod ręką — bez naprawy stanu. Odpowiedź na
"którą potrzebę zaspokoić najpierw": **tę najłatwiej dostępną teraz, łącznie z prowizorką.**

**Jak (szkielet):**
- Dwie warstwy na ukrytych statach: **FeltNeed** (odczuwana, steruje BT) vs **TrueState** (metabolizm).
- Maski obniżają FeltNeed na czas; TrueState leci dalej (lub się pogarsza):
  - **Woda** → rozpycha żołądek → FeltHunger ↓ chwilowo, kalorie = 0 → kaskada kataboliczna
    (glukoza→glikogen→tłuszcz) leci pod spodem. **Fałszywa sytość.**
  - **Owoc kawy / kofeina** → blokuje ODCZUCIE zmęczenia → FeltFatigue ↓, ale adenozyna (`HoursAwake`)
    ROŚNIE dalej → potem zjazd (crash). **IDEALNIE zgodne z istniejącym modelem snu** — adenozyna już
    jest, kofeina = antagonista receptora, NIE sprzątanie długu.
- Arbitraż: NPC adresuje najłatwiej DOSTĘPNĄ potrzebę (proximity / EQS — "co mam pod ręką"), nie zawsze
  najpilniejszą prawdziwie. **Geografia steruje wyborem.**

**Gdzie się wpina:**
- Tier 1 arbitrażu (`EvaluateCurrentNeed`) — wybór wg FeltNeed + dostępności, nie czystej pilności TrueState.
- Silnik snu (`HoursAwake` / adenozyna) — kofeina jako maska.
- Kaskada głodu — woda jako maska sygnału.
- EQS / strefy — dostępność zasobu pod ręką decyduje, którą potrzebę NPC "załatwia".

**Haczyki do rozbudowy:**
- Nadużycie maski = dług rośnie SKRYCIE aż do nagłej zapaści (survival: fałszywy komfort zabija).
- Uzależnienie (kawa) jako pętla zachowań (most do L1/medycznego L2).
- Maska jako WIEDZA/Eureka ("odkrycie: woda tłumi głód") — most do systemu technologii.
- Różne maski per kultura/strefa (antropolog).

**Zależność / status:** ⬜ zasada arbitrażu + przyszłe itemy. Czeka aż dojrzeje system itemów/craftingu
+ dostępność EQS.

═══════════════════════════════════════════════════════════
# MAPA — GDZIE TE TRZY ŻYJĄ W PROJEKCIE
═══════════════════════════════════════════════════════════
| Koncept | Plug-in | Czeka na |
|---|---|---|
| Dzieciństwo → archetyp | OCEAN seed (L3-02) + geografia | spawner + strefy urodzenia |
| Człowieczeństwo | meta-warstwa nad arbitrażem Maslowa; true-name; Grey Spring | pełny Maslow-altitude, system wartości, Grey Spring |
| Maski (felt vs true) | Tier 1 arbitraż; sen (adenozyna); głód; EQS | system itemów/craftingu, EQS dostępności |

Reguła projektu: każdy z tych konceptów → najpierw dojrzewa tu (co robi/jak/gdzie), POTEM osobny
DESIGN GATE z twardymi wartościami, POTEM kod. Żaden nie jest gotowy do kodowania.
