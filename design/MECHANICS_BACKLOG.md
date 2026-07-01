# MECHANICS_BACKLOG — luźne mechaniki, pogrupowane i przypisane do etapów
> Zbiór wszystkich pomysłów mechanik z sesji projektowych, których jeszcze NIE ma w kodzie.
> Cel: nic nie ginie, wszystko ma warstwę (C/L/G) + etap (G1a/G1b/G2/G3) + uziemienie.
> Status pozycji: 💡 pomysł · 📐 zaprojektowane (model gotowy) · 🔨 w budowie · ✅ zrobione.
> Etapy: **G1a** origin sandbox (mechaniki na podłodze) · **G1b** zaludniona wyspa (teren+cykl+fauna)
>        · **G2** polowanie/narzędzia · **G3** społeczność/instytucje.

---

## GRUPA 1 — RUCH I ENERGIA (dwa zasoby) · L1 · G1a
| # | Mechanika | Status | Uziemienie |
|---|---|---|---|
| E1 | **Stamina (souls-like):** krótki pasek, sprint/atak, wyczerpuje→regeneruje w sekundach. Beztlenowa. | 💡 | fast-twitch / anaerobic, kwas mlekowy, zmęczenie w ~20-30s |
| E2 | **Wytrzymałość dzienna:** duży rezerwuar zużywany krokami/aktywnością przez dobę; odbudowa snem. Wyczerpanie → wolniejszy, słabszy. | 💡 | aerobic capacity; cost of transport; to ona wykańcza jelenia |
| E3 | **Ruch: Walk / Jog / Sprint (gaits):** rosnący koszt transportu; sprint drenuje staminę (E1) + szybciej wytrzymałość (E2). | 📐 | zmiana chodu minimalizuje koszt transportu (kursorialne) |
> Rdzeń: `UStaminaComponent` (E1 krótka) + pole `DailyEndurance` na Maslow (E2 długa). Data-driven progi.
> Emergent: jeleń top-speed > wilk, ale mały SprintBudget → po wyczerpaniu spada do walk → wilk (endurance) dopada.

## GRUPA 2 — ADAPTACJA PRZEZ UŻYCIE (zalążek progresji) · L1→L4 · G1a-lite / G2
| # | Mechanika | Status | Uziemienie |
|---|---|---|---|
| P1 | **Use-based tolerance:** powtarzanie aktywności → mniejszy koszt + lepsze wykonanie (tempo↑). Kroki budują tolerancję na chodzenie, bieg na bieg. | 💡 | SAID (Specific Adaptation to Imposed Demands); adaptacja mięśni do obciążenia |
| P2 | **Motor learning:** wykonanie rośnie z powtórzeniami (krzywa uczenia, plateau). | 💡 | uczenie motoryczne; krzywa wprawy |
> Rdzeń fizjologiczny (mięsień adaptuje się) = L1. Pełne mistrzostwo/specjalizacje = L4 (osobny system).
> W G1a-lite: tani zalążek — `ActivityXP[type]` → obniża koszt staminy/wytrzymałości tej aktywności. Ring-buffer, nie rosnąca lista.

## GRUPA 3 — SKRADANIE I ZASADZKA (percepcja ofensywna) · L0 · G1b
| # | Mechanika | Status | Uziemienie |
|---|---|---|---|
| S1 | **Stealth:** niski profil hałasu/widoczności; walk cicho, sprint głośno. Kompromis szybkość↔cichość. | 💡 | odwrotność percepcji — obniżasz własną salience u innych |
| S2 | **Ambush (zasadzka):** drapieżnik czeka w ukryciu, atak z zaskoczenia zamiast pościgu. Mechanika pumy. | 💡 | puma = stalk-and-pounce, krótki wybuch, nie długi pościg |
| S3 | **Kroki = dźwięk:** ruch emituje sygnał słuchowy (głośność wg gait); włączane poza safe zone. | 💡 | słuch przejmuje gdy wzrok zawodzi (cathemeral) |
> To ROZSZERZA model percepcji L0 o stronę ofensywną (dotąd tylko defensywna: NPC wykrywa zagrożenie).
> Skradanie = warunek ambushu pumy. Emitter salience = f(gait, światło, dystans).

## GRUPA 4 — REŻIM NOCNY (modyfikator dobowy na arbitraż) · L1/L0 · G1b
| # | Mechanika | Status | Uziemienie |
|---|---|---|---|
| N1 | **Noc → zmęczenie narasta szybciej.** | 💡 | rytm dobowy — noc = tryb odpoczynku |
| N2 | **Noc → potrzeby tłumione (tłumik na wagi arbitrażu).** | 💡 | sygnały głodu/pragnienia przygaszone nocą |
| N3 | **Wyjątek: `Critical` omija tłumik → „wchodzi w mrok"** (wychodzi poza safe zone mimo ryzyka). | 📐 | desperacja: wartość zaspokojenia > koszt ryzyka |
> To modyfikator dobowy: noc mnoży wagi potrzeb × TŁUMIK, ale `ENeedState.Critical` omija tłumik.
> Spina trójstan (NEED-1) z cyklem dobowym i percepcją nocną.

---

## Z WCZEŚNIEJSZYCH SESJI (jeszcze nie w kodzie) — przypisane do etapów
| # | Mechanika | Warstwa | Etap | Status |
|---|---|---|---|---|
| NEED-1 | **Trójstan potrzeby** `ENeedState {Satisfied, Lacking, Needing, Critical}` — intensywność skaluje priorytet | L1 | G1a | 📐 |
| ARB-1 | **Arbitraż ważony (nie sztywna drabina):** najwyższy `waga × intensywność` wygrywa | L0/L1 | G1a | 📐 (KOREKTA) |
| L0-THREAT | **Fight/Flight/Ignore** — 6 bloków (salience→panic→arbitration→economic→OCEAN→memory) | L0 | G1a→G1b | 📐 (S1-S4) |
| PERC-DN | **Percepcja dzień/noc:** VisionRange↓ nocą, źródła światła wykrywalne, słuch↑ poza safe zone | L0/L1 | G1b | 💡 |
| FAUNA | **Wilk (endurance), jeleń (sprint-fatigue), puma (nocna, ambush)** — jeden agent, różne profile | fauna | G1b | 💡 |
| TERR | **Terytorium / home-range / central-place foraging** (chodzę tak daleko, by wrócić do bazy) | L3 | G2/G3 | 💡 |
| SAFE-ZONE | **Przypisanie NPC do safe zone + zanoszenie jedzenia do bazy** (caching) | L3 | G2/G3 | 🔨 (L3-07 częściowo) |
| CALENDAR | **Kalendarz dzienny NPC** (wpisuje decyzje, plan dnia) | L3 | G2/G3 | 📐 (L3-04 plan) |
| HUNT | **Polowanie/mięso/padlina** (combat ofensywny + ostry kamień) | — | G2 | 💡 |

## PĘTLA DOBOWA (rdzeń powtarzalności — spina to wszystko, G1b)
Świt: budzi się w safe zone (Rested), wzrok wraca, nocne łowce wycofują.
Dzień: żeruje/pije w HomeRadius; jeleń pasie; wilk testuje stado, poluje wytrzymałością.
Zmierzch: powrót do safe zone; percepcja przełącza (wzrok↓, słuch↑); puma wchodzi w okno.
Noc: NIE wychodzi poza safe zone — CHYBA że `Critical` → mrok; widzi tylko światła, słyszy kroki;
     puma poluje z zasadzki; sen odbudowuje wytrzymałość. → świt.
