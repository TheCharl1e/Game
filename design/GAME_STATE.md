# GAME_STATE — stan gry Stan Pierwotny / Caldreth (konsolidacja)
> Cel: jedno miejsce z pełnym kontekstem stanu, żeby Claude (chat) nie zaczynał od zera.
> ⚠️ Commity/statusy spisane z naszych raportów i logów — **Claude Code: zweryfikuj wobec żywego `git log`**
> i uzupełnij różnice. Legenda: ✅ zbudowane+PIE · 🔨 częściowe/w toku · 📐 zaprojektowane (kod nie) · ⬜ nie zaczęte.
>
> **[WERYFIKACJA 2026-07-01, Claude Code, branch docs/pyramid-C-L-G]** — kolumna commitów sprawdzona
> wobec `git log` w **E:\Game (5.7)** (tam żyje kod build-systemów) i **E:\Game_58 (5.8)**. Legenda commitów:
> **✅hash** = osiągalny w żywym grafie (E:\Game, branch `feat/l3-05-p2p-slice1`); **❓hash** = obiekt istnieje
> tylko jako *dangling* (pre-orphan clean-start `573830d`) → przetrwał wyłącznie w `E:\Game-history-2026-06-21.bundle`,
> NIE na żadnym branchu → **do potwierdzenia**. **UWAGA:** 0 build-commitów jest w repo Game_58 — Game_58 ma osobną
> linię (migracja/tooling/eat-loop). Pełny DIFF: `_gates/DOCS_STATE.md`.

## ŚRODOWISKA
| Repo | Silnik | Rola |
|---|---|---|
| `E:\Game_58` | UE 5.8 | **KANON** (live target). `Content/` w `.gitignore` → assety/scena NIE w gicie. |
| `E:\Game` | UE 5.7 | frozen backup + env weryfikacyjny (część prac L3-05/afordancje weryfikowana tu). |
| `E:\Game_git` | — | design-docy (ARCHITECTURE, *_design.md, ROADMAP, MILESTONES). |
> MCP: MCPUnreal :8090 steruje żywym Game_58 (status potrafi „kłamać" 5.7/offline — znany efekt). Native 5.8 MCP umiera na cyklu PIE.

## ✅ ZBUDOWANE + PIE-VERIFIED
| System | Co | Commit (zweryfikowany — patrz legenda w nagłówku) |
|---|---|---|
| Głód | kaskada Glucose→Glycogen→Fat→Protein/HP + APPETITE (kęsy, makra→magazyn, żołądek, fat→izolacja) | ✅`eeba93c` (wiring) · ❓`545a95d` (dangling→bundle) |
| Pragnienie | Hydration, odwodnienie −HP/tick (woda przy rzekach/wybrzeżu) | ❓ brak wyraźnego commita w żywym gicie (do potwierdzenia; pre-orphan) |
| Temperatura | AmbientTemp (strefa) + doba + clothing (izolacja) + fire (heat-source) | ✅`0ab692b` (clothing) · ✅`2395d6d` (fire) · ❓`e2dd851` (rdzeń AmbientTemp, dangling→bundle) |
| Sen | ETAP1 (HoursAwake/FatigueState) + ETAP2 (mgła/mikrosen/omdlenie-ragdoll/Rested) | ❓`8326f14` (ETAP2) · ❓`a7efcdb` (ETAP1) — oba dangling→bundle |
| Metabolizm→ruch | prędkość skaluje kondycją; sen/mikrosen zamraża ruch | ✅`1e2d0a8` |
| Body/senses | 26 części, kaskada uszkodzeń; ESenseType (wzrok/słuch…) cached, recompute-on-change | ❓`e14fa24` (w initial-import, dangling→bundle; helper `6e91372`) |
| Strefy Caldreth | ACaldrethZone, 18 stref, GetZoneAtLocation, FZoneDef (BaseTemp/flagi) | ❓`571956b`+`f0e65d9`+`7d17603` (dangling→bundle) |
| NPCRegistry | int32 ID nie-recyklowane, O(1), despawn-cleanup na EndPlay | ❓`ba9c092` (dangling→bundle) |
| OCEAN slice1 | FOceanProfile (5 floatów) + Neurotyczność→panika (EvaluatePanicRoll, latch bIsInPanic) | ❓`48e1a73` (dangling→bundle) |
| P2P barter | ContractPool + Trader na BP_NPC_Character, autonomiczna pętla post→accept→fulfill | ✅`65e1be6` · ✅`a28bd54` · ✅`ba7ec7c` |
| Afordancje (L0-TA-S1) | WorldAffordanceSubsystem (rejestr+spatial hash+regen+atomic claim+cancel-on-death) + EQS generator + budget | ✅`60282fd` · ✅`0d6c1bf` |
| Flee/panika | damage-hook→zagrożenie→przerwanie BT→ucieczka (UBTTask_Flee) | ✅`3335fe2` · ✅`826b20e` |

## 🔨 W TOKU / CZĘŚCIOWE
- **L0 threat (L0-04):** dziś próg paniki = otrzymana rana. BRAK: wykrywanie DRAPIEŻNIKA przez zmysły/EQS (czeka na percepcję-jako-dane).
- **Afordancje pętla autonomiczna (L0-TA-S1e):** BTTaski + patche gotowe; pełna pętla homeless→wander→EQS→attach→forage NIEzweryfikowana (poddrzewo BT + PIE).
- **G1-K1 scena:** `ANPCSpawner` (C++, World/) napisany, czeka na rebuild z zamkniętym edytorem (nowa UCLASS→UHT). Redefinicja D: spawn @ origin. *(2026-07-01: K1 raportowany DONE+PIE w `design/_gates/G1_K1_SCENE.md`; src `NPCSpawner.cpp/.h` untracked w Game_58 working tree.)*
- **Safe Zone (L3-07):** EQS_ZoneSearch istnieje; przypisanie NPC do stref częściowe. ❓ (brak commita w żywym gicie — do potwierdzenia)

## 📐 ZAPROJEKTOWANE (kod nie) / ⬜ NIE ZACZĘTE
- **L0 Fight/Flight/Ignore** — pełny model 6-blokowy (📐, S1-S4). Trójstan `ENeedState` + arbitraż ważony (📐).
- **Fauna** (wilk/jeleń/puma) — profile, LOD tick (💡).
- **G1b map-gate** — teren + navmesh pod 18 strefami (⬜) — patrz BLOCKERY.
- **L1-10 thirst activity-scaling** — INERT: sterownik martwy (SetActionByRow bez wołającego, DT odpięty) → mnożnik stały 1.0. Naprawa = gate architekta. *(potwierdzone: `0915178` docs(l1-10) verify INERT — E:\Game ✅)*
- **L2+ / L3 (poza P2P/registry) / L4 / L5** — ⬜.

## 🔴 BLOCKERY
1. **Strefy bez terenu:** 18 stref to dane wiszące ±370k uu; jedyny chodliwy grunt = podłoga 8000×8000 @ origin. `0/18 stref w navmeshu`. → różnicowanie strefowe (temp/migracja) niemożliwe do G1b. **Następny map-gate.**
2. **Content Game_58 gitignored:** scena (spawner postawiony, GameMode, zegar) NIE wchodzi do gita → utrwalenie = ręczny Ctrl+S dyrektora, backup ręczny. Decyzja o wersjonowaniu contentu odłożona.
3. **Thirst driver martwy** (L1-10, patrz wyżej).

## ARCHITEKTURA / KONWENCJE (nienaruszalne)
- **C++ = mózg** (matematyka/pamięć/logika), **Blueprint = ciało** (wizualia/anim/particle przez BlueprintImplementableEvent).
- **Single-bridge:** tylko `BTService_MaslowBlackboardSync` pisze Blackboard.
- **O(N) religia:** hot paths ≤ O(N) przy 500+. Loop-inversion (bodziec broadcastuje, NPC nie skanuje). EQS budżetowany (12 concurrent).
- **Data-driven:** statystyki przez UDataAsset/UDataTable, zero hardkodów.
- **PIE-verify:** przez UE_LOG parsowany post-PIE, twarde liczby — NIGDY live MCP introspection.
- **Piramida os:** **C** (świat/substrat) · **L** (Maslow NPC L0-L5, strzałka-w-dół) · **G** (grywalny stan/roadmapa, komponuje C+L+gracz).
- **Gate workflow:** jeden problem/sesja → zapis do `_gates/*.md` (RESOLVED+DEFAULTS+OPEN) → czysty czat.
- **Bramki nieodwracalne (STOP+pytaj):** edycja/tworzenie BP, PIE, git push, hard-delete, set_pin_value object/enum.

## DOKUMENTACJA (piramida C/L/G, branch docs/pyramid-C-L-G)
- `design/C_world/` C0-C4 · `design/L_npc/` L0-L5 · `design/G_roadmap/` G0-G2 · `_gates/` · `_pipeline/PIPELINE.md`
- Warstwy ✅ zmigrowane (L1_*, C3); reszta = szkielety (ETAP 3, per-warstwa). `_gates/MIGRATION_MAP.md`.
- Backlog mechanik w kolejce: `design/MECHANICS_BACKLOG.md` (NEED-1 trójstan, ARB-1 arbitraż ważony, ruch/energia, stealth, reżim nocny).

## NASTĘPNY FRONT
- **G1a domknięcie:** ruch (Walk/Jog/Sprint) + stamina/wytrzymałość + trójstan + arbitraż ważony (na origin).
- **G1b map-gate:** teren pod strefami (wulkan+wyspa) → odblokowuje percepcję dzień/noc, faunę, cykl dobowy.
- Backlog mechanik: `MECHANICS_BACKLOG.md`.
