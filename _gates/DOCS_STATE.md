# GATE: DOCS_STATE — wciągnięcie backlogu + game-state (single source of truth)
> Wykonawca: Claude Code · Data: 2026-07-01 · Branch: `docs/pyramid-C-L-G` (już istniał, checked-out) · Repo: `E:\Game_58`
> Bramka: umieść 2 pliki dyrektora w `design/`, zweryfikuj GAME_STATE wobec żywego gita, zlinkuj (nie duplikuj), commit BEZ push.

## 1. UMIESZCZONE
| Plik | Cel | Źródło | Status |
|---|---|---|---|
| `design/MECHANICS_BACKLOG.md` | backlog mechanik (single source) | `E:\Game_git\MECHANICS_BACKLOG.md` | ✅ verbatim (byte-identical, `cmp` OK) |
| `design/GAME_STATE.md` | konsolidacja stanu | `E:\Game_git\GAME_STATE.md` | ✅ umieszczony + SKORYGOWANY (kolumna commitów) |

> Pliki dyrektora znalezione w `E:\Game_git\` (zapis 2026-07-01 17:32), nie w `design/` — po potwierdzeniu ścieżki przez dyrektora.

## 2. LINKOWANIE (nie duplikacja)
- `design/G_roadmap/G1_animal_world.md` → dodana sekcja **„## MECHANIKI W KOLEJCE"** z linkiem do `../MECHANICS_BACKLOG.md` (mapowanie E1-E3/NEED-1/ARB-1 → G1a; S1-S3/N1-N3/FAUNA/PERC-DN → G1b).
- `design/L_npc/L_README.md` → dodana 1 linijka **KOREKTA arbitrażu**: trójstan `ENeedState` + arbitraż ważony (`waga × intensywność`), link do backlog **NEED-1 / ARB-1**.
- `design/GAME_STATE.md` → sekcja DOKUMENTACJA wskazuje na `design/MECHANICS_BACKLOG.md` (bez kopiowania treści).

## 3. WERYFIKACJA GAME_STATE — DIFF (żywy git)
**Metoda:** `git cat-file -e <hash>^{commit}` (istnienie obiektu) + `git for-each-ref --contains` (osiągalność z refów) w `E:\Game` (5.7) i `E:\Game_58` (5.8).
**Kluczowy fakt:** wszystkie build-commity żyją w **E:\Game (5.7)**, branch `feat/l3-05-p2p-slice1` — **ŻADEN nie jest w repo Game_58** (Game_58 ma osobną linię migracja/tooling/eat-loop: `51e5d95..59f2b2b`).
**Legenda:** ✅ = osiągalny (żywy graf) · ❓ = *dangling* (obiekt istnieje, ale odcięty przez orphan clean-start `573830d` → przetrwał tylko w `E:\Game-history-2026-06-21.bundle`) → wymaga potwierdzenia, NIE zmyślam.

### 3a. Commity POTWIERDZONE (✅ reachable, E:\Game / feat/l3-05-p2p-slice1)
| Hash | Subject (git) | System w GAME_STATE |
|---|---|---|
| `eeba93c` | feat(npc): StartEatingItem — BP-facing eat wiring helper | Głód (wiring) |
| `0ab692b` | docs(l1-09a): clothing insulation DONE + PIE-verified | Temperatura (clothing) |
| `2395d6d` | docs(l1-09b): fire heat-source DONE + PIE-verified | Temperatura (fire) |
| `1e2d0a8` | feat(maslow): L1-08 metabolic state -> movement speed | Metabolizm→ruch |
| `65e1be6` | feat(p2p): L3-05 slice 1 — virtual barter contract pool | P2P barter |
| `a28bd54` | feat(p2p): expose pool mutators as BlueprintCallable | P2P barter |
| `ba7ec7c` | docs(l3-05): commit P2P slice-1 report + reconcile ROADMAP | P2P barter |
| `60282fd` | feat(l0-ta-s1): exploration spine + affordance registry + EQS budget | Afordancje |
| `0d6c1bf` | fix(l0-ta-s1): ratify reconcile + 5.8 build enablement | Afordancje |
| `3335fe2` | docs(task3): flee/panic DONE + PIE-verified | Flee/panika |
| `826b20e` | feat(flee): TASK 3 generic threat + damage-hook + UBTTask_Flee | Flee/panika |

### 3b. Commity NIEPOTWIERDZONE (❓ dangling → tylko w bundle) — SKORYGOWANE w pliku na ❓
| Hash w GAME_STATE | Subject (git, w bundle) | System | Akcja |
|---|---|---|---|
| `545a95d` | feat(npc): APPETITE grubas slice 1 — eating process | Głód | ❓ oznaczony (dangling→bundle); zachowany ✅`eeba93c` |
| `e2dd851` | AmbientTemp (zone core): environment temperature revives 3 dead temp systems | Temperatura | ❓ oznaczony (dangling→bundle) |
| `8326f14` | Sleep engine ETAP 2: fatigue effects + sleep/reset/Rested | Sen | ❓ oznaczony; dodano ETAP1 `a7efcdb` (też dangling) |
| `ba9c092` | feat(npc): NPCRegistry keystone (L3-01) - int32 ID subsystem | NPCRegistry | ❓ oznaczony (dangling→bundle) |
| `48e1a73` | feat(npc): OCEAN L3-02 slice #1 — Neuroticism -> stochastic L0 panic | OCEAN slice1 | ❓ oznaczony (dangling→bundle) |

### 3c. Pozycje „—" UZUPEŁNIONE (znalezione w dangling/bundle, oznaczone ❓)
| System | Było | Znaleziono (git, dangling) | Akcja |
|---|---|---|---|
| Pragnienie | — | brak wyraźnego commita (thirst/hydration nienazwane w historii) | ❓ „brak commita, do potwierdzenia" |
| Body/senses | — | `e14fa24` (initial import) + helper `6e91372` GetPartDisplayName | ❓ initial-import (dangling→bundle) |
| Strefy Caldreth | — | `571956b` (DT_ZoneDefs 12 biomes) + `f0e65d9` (bake 18 stref) + `7d17603` (GetZoneAtLocation) | ❓ wypełnione (dangling→bundle) |

### 3d. Weryfikacja sekcji tekstowych „W TOKU" / statusów
| Twierdzenie GAME_STATE | Ustalenie | Akcja |
|---|---|---|
| L1-10 thirst driver martwy (INERT) | ✅ potwierdzone commitem `0915178` docs(l1-10) verify INERT (E:\Game, reachable) | dodano ref w pliku |
| G1-K1 scena: ANPCSpawner napisany, czeka na rebuild | częściowo nieaktualne: K1 raportowany DONE+PIE w `design/_gates/G1_K1_SCENE.md`; `NPCSpawner.cpp/.h` untracked w Game_58 WT | dopisano notkę w pliku |
| Safe Zone (L3-07) częściowe | brak commita w żywym gicie | ❓ „do potwierdzenia" dodane |
| L0 threat / afordancje pętla / architektura / blockery | spójne z raportami i pamięcią; brak sprzeczności | bez zmian |

**Podsumowanie DIFF:** 11 hashy ✅ potwierdzonych · 5 hashy przeklasyfikowanych na ❓ (dangling) · 3 pozycje „—" uzupełnione (❓) · 1 status skorygowany (K1). Zero zmyślonych — każdy ❓ ma realny subject z bundle/dangling, ale nie jest w żywym grafie.

## 4. GIT
- Branch: `docs/pyramid-C-L-G` (już był checked-out, nie tworzyłem).
- Commit **tylko** plików docs (NIE tknąłem untracked C++ `NPCSpawner.*`, `SurvivalPlayerController.*` — poza zakresem bramki).
- **NIE push** (zgodnie z bramką).
- Hash commita: *(uzupełniony po commit — patrz niżej)*.

## 5. BRAMKI / STOP
- ✅ nie push · ✅ nie dotykałem kodu ani sceny · ✅ nie zmyśliłem commitów (❓ gdy niepewne) · ✅ linkowanie zamiast duplikacji.
- OPEN dla dyrektora/architekta:
  1. **Repo-mismatch:** GAME_STATE prezentuje Game_58 jako KANON, ale 0 build-commitów tam jest — cały dowód buildów żyje w E:\Game (5.7) + bundle. Czy skonsolidować historię (import bundle do Game_58) czy zostawić E:\Game jako env-weryfikacyjny?
  2. **Pragnienie bez commita** — czy istnieje gdzieś (bundle głębiej / lokalne zmiany nietknięte) czy build był przed-git?
