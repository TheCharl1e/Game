# GATE: PORZADEK-01 — Game_58 = kanon (audyt migracji + git baseline + kanon)

Data: 2026-06-30 · Wykonawca: Claude Code · Target: E:\Game_58 (UE5.8) · Backup: E:\Game (5.7)
Akcje nieodwracalne wykonane: ŻADNE (zero push / zero hard-delete / zero nadpisania Content).

---

## RESOLVED (zrobione, z dowodem)

### 1. Audyt migracji — KOMPLETNA, 0 braków
Liczone rekursywnie `.uasset + .umap`:
| | .uasset | .umap | RAZEM |
|---|---|---|---|
| 5.7 `E:\Game\Content`    | 2998 | 15 | **3013** |
| 5.8 `E:\Game_58\Content` | 3002 | 15 | **3017** |

- **Braki w 58 względem 5.7: 0** (`comm -23` listy posortowanej = pusta). Patrz sekcja DIFF.
- 58 ma **+4 assety**, których nie ma w 5.7 (nowa praca AI, nie braki) — patrz DIFF.
- Oczekiwane ~3014 ref → faktycznie 3017 w 58. Migracja Content kompletna. NIC nie kopiowano.

### 3. Kanon / backup — UTWORZONE
`E:\Game_58\README_CANON.md` zapisany: 58=kanon dev, E:\Game(5.7)=frozen backup, ścieżka MCP-on-5.7 porzucona (native MCP tylko na 5.8). Zawiera też checklist ręczny MCP (deliverable PORZADEK-02).

### 5. Weryfikacja reparentu BP pod VC
- C++ baza **`AffordanceSourceActor`** JEST w git (tracked): `Source/Stan_Pierwotny/World/AffordanceSourceActor.{h,cpp}` (+ AffordanceType.h, EnvQueryGenerator_Affordances, WorldAffordanceSubsystem).
- Reparent POTWIERDZONY LIVE (blueprint_query, introspekcja statyczna — nie PIE):
  - `BP_Food` → `parent_class: AffordanceSourceActor` ✔
  - `BP_WaterSource` → `parent_class: AffordanceSourceActor` ✔
  - *(odczyt z żywej instancji edytora 5.7 na :8090; Content 58 = identyczna kopia, 0 braków → reparent niesie się do 58.)*
- Pliki na dysku 58: `Content/DocelowaGra/FOOD/BP_Food.uasset`, `Content/DocelowaGra/DRINK/BP_WaterSource.uasset` — OBECNE.
- ⚠️ **Reparent NIE jest pod git** — `.gitignore` celowo ignoruje `Content/` (repo = code+docs). Pod VC jest tylko klasa-rodzic C++, nie sam `.uasset`. Patrz OPEN-3.

---

## DEFAULTS (gapy domknięte założeniem — assume-log-continue)

- **D1. git init / baseline commit — POMINIĘTE (już istnieje).** `E:\Game_58\.git` istnieje, branch `master`, realna historia (m.in. `51e5d95 chore(58): bootstrap git safety-net for live UE 5.8 project`). Tworzenie drugiego „baseline" byłoby duplikatem → NIE wykonano. Rzeczywistość ≠ założenie briefu (brief zakładał brak .git).
- **D2. `.gitignore` — ZACHOWANY ISTNIEJĄCY, nie nadpisany.** W repo jest bogatszy, celowy `.gitignore` (ignoruje `Content/`, `.mcp.json`, `*.sln`, `Plugins/*` z wyjątkiem `Plugins/MCPUnreal/`, MapData npz, stray duplikaty). Wersja z briefu jest jego podzbiorem (brak reguł Content/.mcp.json/MCPUnreal). Nadpisanie = regresja → NIE nadpisano. Decyzja do potwierdzenia: OPEN-2.
- **D3. Commit governance** zrobiony tylko na NOWYCH plikach (README_CANON, _pipeline, _redteam, _gates) — patrz MCP_WIRING_01. Stray `Source/.../SurvivalPlayerController.{h,cpp}` (untracked) NIE dotknięte (możliwe WIP) → OPEN-4.

---

## OPEN (wymaga Twojej decyzji)

- **OPEN-1 (krok 4, KWARANTANNA LEGACY).** Monolith: NIE obecny w 58 (brak pluginu, brak portu :9316) → nic do kwarantanny. **MCPUnreal: obecny, ENABLED w `.uproject`, tracked w git (38 plików), opisany jako „load-bearing 5.8 verify tooling".** To NIE jest porzucony legacy — to aktywne narzędzie. Przeniesienie do `_quarantine\` wymagałoby edycji `.uproject` (zakaz briefu) i zepsułoby ładowanie edytora → **NIE ruszam, czekam na decyzję.** Opcje: (a) zostaw MCPUnreal jako tor weryfikacji obok native 8000; (b) wyłącz w .uproject + kwarantanna (powiedz „tak" — zrobię oba kroki). `_quarantine\` NIE utworzony (nic nie zakwalifikowane).
- **OPEN-2 (`.gitignore`).** Zostawić istniejący bogatszy (rekomendacja) czy podmienić na wersję z briefu (utrata reguł Content/MCPUnreal/.mcp.json)? Rekomendacja: zostawić istniejący.
- **OPEN-3 (reparent poza VC).** `Content/` jest gitignored → reparenty BP żyją tylko na dysku. Ryzyko utraty przy awarii dysku. Opcje: (a) zaakceptuj (repo=code+docs, backup Content osobno); (b) odignoruj wybrane krytyczne `.uasset` (BP_Food/BP_WaterSource) i wersjonuj. Decyzja Twoja.
- **OPEN-4 (stray source).** `Source/Stan_Pierwotny/RTS/SurvivalPlayerController.{cpp,h}` są untracked. Commit teraz czy to WIP do dokończenia?

---

## DIFF (manifest braków assetów)

### Braki w 58 (są w 5.7, brak w 5.8): **0** — żaden.

### Nadmiar w 58 (są w 5.8, brak w 5.7): 4 (nowa praca AI, NIE braki migracji)
```
Content/DocelowaGra/AI_NPC/BT_Exploration.uasset
Content/DocelowaGra/AI_NPC/EQS_FindResource.uasset
Content/DocelowaGra/AI_NPC/EQS_FindSafeZone.uasset
Content/DocelowaGra/AI_NPC/EQS_FindWater.uasset
```

> Wniosek: migracja Content jest kompletna i bezstratna; nie ma czego dokopiowywać. NIC nie kopiowano automatycznie (zgodnie z bramką).
