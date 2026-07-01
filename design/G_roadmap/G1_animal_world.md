# G1 — Żywy świat na poziomie zwierzęcym

**Oś:** G (grywalny stan) · **Status:** ⬜ w budowie · Kompozycja: C(0-3) + L(0-1) + gracz-obserwator.
G1 to w ~60% INTEGRACJA istniejących systemów, nie greenfield.

> **PODZIAŁ (ETAP 2, 2026-07-01):** G1 rozbite na **G1a (origin sandbox)** + **G1b (zaludniona wyspa)** wskutek blockera terenu z K1 (18 stref = dane bez terenu pod nimi; jedyna podłoga = 8000×8000 @ origin). G1a dowodzi że MECHANIKI żyją na podłodze origin; G1b przenosi je na teren pod strefami (różnicowanie strefowe) i wymaga osobnego MAP-GATE.

## KILLER-DEMO (czym G1 jest)
Na zaludnionej CaldrethMap, bez skryptu, jednocześnie w logu widać: NPC pije/je/śpi wg potrzeb, marznie i ucieka do ciepła, ucieka/broni się przed wilkiem, umiera przy niezaspokojonej potrzebie — i pętla trzyma budżet przy 50/200/500 NPC. *(Część „marznie/ucieka do ciepła" wymaga różnicowania stref → G1b; reszta → G1a.)*

## STAN WEJŚCIOWY (✅ już stoi)
Pragnienie, głód, temperatura (rdzeń strefowy + sprzężenie ciało + hipotermia stopniowa), sen (ETAP1+2), body/senses, strefy Caldreth (18, GetZoneAtLocation), NPCRegistry (despawn-cleanup), BP_DayNightCycle (2.4 min/doba). **K1 DONE 2026-07-01:** `ANPCSpawner` (C++), 50 NPC spawn/register na navmeshu origin, zegar tyka, AmbientTemp strefowe, fizjologia-trend, śmierć→cleanup — PIE-verified (patrz `_gates/G1_K1_SCENE.md`).

## OŚ C w G1
AKTYWNE: strefy koncentryczne, woda stała (rzeki/wybrzeże), temp strefowa + doba, zegar dobowy. POSTAWIĆ: BP_DayNightCycle na CaldrethMap (✅ już stoi — wariant WORLD/). NIE w G1: pełna pogoda (chmury/deszcz), słońce sezonowe, dynamiczne wadi. (Track B / G2+)

## OŚ L w G1 (NPC = L0 + L1, priorytety 0-4)
Level_0_FightOrFlight [BUDUJ: model S1-S2] · L1_Temperature ✅ · L1_Hydration ✅ · L1_Rest ✅ · L1_Nutrition ✅. L2+ śpi. Wybór potrzeby: arbitraż (najbardziej osiągalna w pobliżu, nie tylko najpilniejsza).

## FAUNA (RESOLVED)
Jeden agent, różne UDataAsset — NIE osobny system zwierzęcy.
- Człowiek-NPC: pełny Maslow, w G1 aktywne L0+L1.
- Wilk (predator): L0+L1, threat-emitter wysoki, poluje, dla NPC = zagrożenie.
- Jeleń (prey): L0+L1, threat-emitter niski, forage roślinny, ucieka przed wilkiem i NPC.
Budżet (500 ludzi + stada = 700+ agentów): LOD ticku po dystansie + okrojony zakres (zwierzę bez snu-debuffów/precyzji dłoni). Cel: O(N) mała stała. *(Fauna = klocek K4/K5, nie w G1a rdzeniu.)*

## GRACZ w G1 — obserwator
WBP_NPC_Inspector ✅ (klik NPC → staty/survival na żywo). Dołożyć: god-view kamera (`GM_RTSGameMode`/`BP_RTSCamera` ✅ istnieją) + debug-spawner TYLKO do testów. Zero postaci gracza, zero ingerencji w gameplay.

---

## 🟩 G1a — ORIGIN SANDBOX (dowód, że MECHANIKI żyją)
**Zakres:** pełna pętla L0+L1 na podłodze **8000×8000 @ origin**, **JEDNA strefa** (GetZoneAtLocation origin). K1-K6 lecą tutaj. AmbientTemp = **jednostrefowa** (dług jawny — brak różnicowania). Spawn przy origin na istniejącym navmeshu (decyzja K1 = opcja D).

**DoD G1a (falsyfikowalne, log-backed):** bez skryptu, jednocześnie:
- **pije** (Hydration↓ → recover ze źródła wody),
- **je** (Glucose↓ → forage → recover),
- **śpi** (HoursAwake↑ → mikrosen/omdlenie → Rested),
- **panika-flee** (zagrożenie → L0 przerwanie → ucieczka),
- **śmierć-cleanup** (niezaspokojona potrzeba → EndPlay → NPCRegistry unregister + slot afordancji zwolniony),
- **dowód skali** O(P·local) nie O(N²) przy **50/200/500** NPC na origin (twarde liczby post-PIE, UE_LOG, frametime baseline).

**Klocki G1a:** spawner ✅(K1) · afordancje domknięte (woda/jedzenie interaktywne, densyfikacja pod 50+) · L0 threat perception (S1 event-bus+arbitration, S2 fight/flight economic) · śmierć-cleanup ✅(K1 częściowo) · rozrzut startowy HoursAwake.

## 🟦 G1b — ZALUDNIONA WYSPA (różnicowanie strefowe)
**Zakres:** te same mechaniki na **TERENIE pod 18 strefami** → różnicowanie strefowe. **WYMAGA: teren + navmesh pod strefami = osobny MAP-GATE** (nie w G1a).

**DoD G1b (przeniesione z G1 — punkty wymagające różnicowania stref):**
- **AmbientTemp reaguje na strefę** (marznięcie Ocean 8°C / Mountain 4°C vs upał AshSlope 28°C / Lava 80°C),
- **migracja do wody** (susza/pragnienie → NPC idzie do rzeki/wybrzeża),
- **stoki popiołu jako ucieczka od zimna** (marznący NPC → cieplejsza strefa),
- **„blisko lawy = blisko śmierci"** (gradient przeżywalności geograficzny).

**Klocki G1b:** MAP-GATE terenu+navmesh pod strefami · spawn per-strefa bSpawnable (zamiast origin) · migracja strefowa w BT.

## GRANICE G1 (świadomie NIE)
Brak L2+ (własność/kradzież/spiżarnia), grup/ognisk (L3), reputacji/frakcji (L4), detektywa/Eurek (L5), pełnej pogody/słońca sezonowego, narzędzi/craftingu, polowania (→G2), postaci gracza.

## DECYZJE
- Polowanie → G2 (RESOLVED). W G1 wilk=zagrożenie (testuje L0); jeleń=żywy prey bez mechaniki mięsa.
- p_win w L0: bodziec niesie prawdę o T (Slice1-2); pamięć-estymator = Slice4.
- **Podział G1a/G1b (2026-07-01):** origin sandbox dowodzi mechanik; różnicowanie strefowe czeka na teren (MAP-GATE).

## ZNANE DŁUGI (G1)
- **(a) strefy bez terenu** → różnicowanie strefowe niemożliwe na origin; przeniesione do **G1b** (wymaga MAP-GATE terenu+navmesh pod 18 strefami). W G1a AmbientTemp jednostrefowa.
- **(b) stan sceny Game_58 NIE w gitcie** — Content gitignored → spawner@origin + GameMode override + placement żyją tylko w `.umap`; **backup ręczny** (Ctrl+S dyrektora), nie wersjonowane.
- **(c) HoursAwake sync** — wszystkie NPC startują HoursAwake identycznie → zsynchronizowane omdlenia; dodać **rozrzut startowy** (per-instancja losowy offset).
- Długi szczegółowe z K1 (recover-at-scale, mikrosen-capture, frametime-baseline, Inspector-klik, jednostrefowość) → `_gates/G1_K1_SCENE.md` (d1-d7).
