# G1 — Żywy świat na poziomie zwierzęcym
Status: ⬜ w budowie. Oś: G (grywalny stan). Kompozycja: C(0-3) + L(0-1) + gracz-obserwator.
G1 to w ~60% INTEGRACJA istniejących systemów, nie greenfield.

## KILLER-DEMO (czym G1 jest)
Na zaludnionej CaldrethMap, bez skryptu, jednocześnie w logu widać: NPC pije/je/śpi wg potrzeb,
marznie i ucieka do ciepła, ucieka/broni się przed wilkiem, umiera przy niezaspokojonej potrzebie —
i pętla trzyma budżet przy 50/200/500 NPC.

## STAN WEJŚCIOWY (✅ już stoi)
Pragnienie, głód, temperatura (rdzeń strefowy + sprzężenie ciało + hipotermia stopniowa),
sen (ETAP1+2), body/senses, strefy Caldreth (18, GetZoneAtLocation), NPCRegistry (despawn-cleanup),
BP_DayNightCycle (2.4 min/doba).

## OŚ C w G1
AKTYWNE: strefy koncentryczne, woda stała (rzeki/wybrzeże), temp strefowa + doba, zegar dobowy.
POSTAWIĆ: BP_DayNightCycle na CaldrethMap (blokuje doba-temp i "jeden zegar" snu).
NIE w G1: pełna pogoda (chmury/deszcz), słońce sezonowe, dynamiczne wadi. (Track B / G2+)

## OŚ L w G1 (NPC = L0 + L1, priorytety 0-4)
Level_0_FightOrFlight [BUDUJ: model S1-S2] · L1_Temperature ✅ · L1_Hydration ✅ · L1_Rest ✅ · L1_Nutrition ✅.
L2+ śpi. Wybór potrzeby: arbitraż (najbardziej osiągalna w pobliżu, nie tylko najpilniejsza).

## FAUNA (RESOLVED)
Jeden agent, różne UDataAsset — NIE osobny system zwierzęcy.
- Człowiek-NPC: pełny Maslow, w G1 aktywne L0+L1.
- Wilk (predator): L0+L1, threat-emitter wysoki, poluje, dla NPC = zagrożenie.
- Jeleń (prey): L0+L1, threat-emitter niski, forage roślinny, ucieka przed wilkiem i NPC.
Budżet (500 ludzi + stada = 700+ agentów): LOD ticku po dystansie + okrojony zakres (zwierzę bez snu-debuffów/precyzji dłoni). Cel: O(N) mała stała.

## GRACZ w G1 — obserwator
WBP_NPC_Inspector ✅ (klik NPC -> staty/survival na żywo). Dołożyć: god-view kamera + debug-spawner
(postaw wilka, przeskocz czas) TYLKO do testów. Zero postaci gracza, zero ingerencji w gameplay.

## BRAKUJĄCE KLOCKI (zadania G1)
1. Caldreth spawner + NavMesh — zaludnienie (50->200->500 progresywnie).
2. L0 threat perception — S1 (event-bus + arbitration) + S2 (fight/flight economic).
3. Postaw BP_DayNightCycle na CaldrethMap.
4. Afordancje domknięte — woda/jedzenie interaktywne (EQS forage -> navigate -> consume -> stat recovery). = Track A Slice 1 domknięcie.
5. Fauna — wilk + jeleń profile + LOD tick.
6. Śmierć-cleanup — ragdoll -> EndPlay wyrejestrowuje + zwolnienie slotu afordancji (cancel-on-death, §8). Zwłoki znikają (padlina=G2).

## DEFINITION OF DONE (falsyfikowalne, log-backed)
Bez skryptu, jednocześnie: pije(Hydration↓->recover) | je(Glucose↓->forage) | śpi(HoursAwake↑->Rested) |
marznie->ucieka do ciepła | wilk w percepcji -> panika↑ -> flight/fight | umiera przy niezaspokojeniu -> clean cleanup.
DOWÓD SKALI: koszt percepcji O(P·local) nie O(N²) przy 50/200/500 (twarde liczby post-PIE, UE_LOG).

## GRANICE G1 (świadomie NIE)
Brak L2+ (własność/kradzież/spiżarnia), grup/ognisk (L3), reputacji/frakcji (L4), detektywa/Eurek (L5),
pełnej pogody/słońca sezonowego, narzędzi/craftingu, polowania (->G2), postaci gracza.

## DECYZJE
- Polowanie -> G2 (RESOLVED). W G1 wilk=zagrożenie (testuje L0); jeleń=żywy prey bez mechaniki mięsa.
- p_win w L0: bodziec niesie prawdę o T (Slice1-2); pamięć-estymator = Slice4.
