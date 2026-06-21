# PROTOKÓŁ OPERACYJNY — sesje, reset, Git (Stan_Pierwotny / Caldreth)
> Jak wznawiać pracę po resecie/clear i jak utrzymać dyscyplinę gitową w dwóch repo.
> Skrót prawdy: w Claude Code ZASADY wracają same (CLAUDE.md auto-load). Komenda resetu ma
> wczytać aktualny STAN i ustawić task — to jedyne, co nie ładuje się automatycznie.

═══════════════════════════════════════════════════════════
# DWA REPO — MAPA
═══════════════════════════════════════════════════════════
| Repo | Gdzie | Co trzyma | Kto czyta |
|---|---|---|---|
| **Komunikacyjne** (to) | osobne | designy, MILESTONES, README, EXPERT_LENSES, ten protokół | Chat-Claude (Projekt) auto-ładuje |
| **Gry** | `E:\Game`, moduł `Source/Stan_Pierwotny/` | KOD + żywe docs (PROJECT_STATE/ROADMAP/MECHANICS/CODE_REGISTRY/DECISIONS/CHANGELOG/REPORTS) + `CLAUDE.md` | Claude Code auto-ładuje CLAUDE.md |

Łącznik repo: commit z repo gry → jego hash trafia do `MILESTONES.md` w repo komunikacyjnym. Bez submodułów — prościej.

═══════════════════════════════════════════════════════════
# 🔄 KOMENDA RESETU — ARCHITEKT (Chat-Claude, nowy czat w Projekcie)
═══════════════════════════════════════════════════════════
> Wklej na początku nowego czatu. Pliki Projektu (designy, EXPERT_LENSES) ładują się SAME —
> brakuje tylko żywego stanu z repo gry. Po `<temat>` wpisz dzisiejszy temat.

```
BOOT ARCHITEKT — Stan_Pierwotny / Caldreth.
Jesteś ARCHITEKTEM (design + spec, nie kod). Wiedzę Projektu masz w kontekście: Gra.txt, designy,
EXPERT_LENSES.md. Zasady (skrót): C++ mózg / BP ciało, zero Event Tick, data-driven, IsValid wszędzie,
patch nie regeneruj, weryfikacja twardymi liczbami, staged delivery z moim approvalem, 500+ NPC = analiza
złożoności przed kodem. Czego NIE masz w kontekście: żywych docs z repo gry (PROJECT_STATE/ROADMAP/
MECHANICS/CODE_REGISTRY) — poproś o wklejenie fragmentu, jeśli go potrzebujesz.

Dzisiejszy temat: <temat>.

Zanim ruszysz: (1) jednym zdaniem powiedz, gdzie jesteśmy wg MILESTONES; (2) dobierz radę soczewek do tematu
(tabela w EXPERT_LENSES); (3) jeśli to mechanika definiująca rozgrywkę — najpierw DESIGN GATE, kod dopiero po
moim „zatwierdzam". Nie generuj kodu przed approvalem.
```

═══════════════════════════════════════════════════════════
# 🔄 KOMENDA RESETU — EXECUTOR (Claude Code, po `/clear`)
═══════════════════════════════════════════════════════════
> `/clear` czyści rozmowę, ale `CLAUDE.md` (zasady) wczytuje się PONOWNIE sam na starcie sesji.
> Ta komenda NIE ładuje zasad (są z auto) — wczytuje ŻYWY STAN i ustawia task. Wklej po `/clear`:

```
BOOT EXECUTOR. Zasady masz z CLAUDE.md (auto) — nie powtarzaj ich.
1. Przeczytaj żywe docs: PROJECT_STATE.md, ROADMAP.md, MECHANICS.md, CODE_REGISTRY.md, DECISIONS.md
   oraz ostatnie wpisy CHANGELOG.md.
2. Sprawdź MCP: Monolith :9316, MCPUnreal :8090 (jeśli cisza — najpierw szukaj blokującego modala).
3. W maks. 3 liniach: gdzie jesteśmy, jaki jest następny task wg ROADMAP, czy są blokery.
4. NIE pisz kodu. Czekaj na design gate / instrukcję ode mnie (architekt). Patch nie regeneruj.
```

**Opcjonalnie — zadrutuj jako `/boot`** (rekomendowane: skill zamiast starej komendy):
utwórz `.claude/skills/boot/SKILL.md` z treścią powyżej. Wtedy po `/clear` wpisujesz tylko `/boot`.
(Pliki w `.claude/commands/boot.md` też nadal działają, ale `.claude/skills/` to obecna rekomendacja.)

═══════════════════════════════════════════════════════════
# RYTUAŁ `/clear` — co wraca samo, co trzeba dociągnąć
═══════════════════════════════════════════════════════════
| Po `/clear` w Claude Code | Wraca samo? | Jak |
|---|---|---|
| Zasady projektu | ✅ TAK | `CLAUDE.md` auto-reload na starcie sesji |
| Żywy stan (ROADMAP/MECHANICS/...) | ❌ NIE | komenda `BOOT EXECUTOR` / `/boot` |
| Historia rozmowy | ❌ NIE (i dobrze) | celowo — czysta kartka, mniej tokenów |

| Nowy czat w Chat-Claude (Projekt) | Wraca samo? | Jak |
|---|---|---|
| Designy + EXPERT_LENSES + zasady | ✅ TAK | wiedza Projektu auto-ładuje |
| Żywy stan z repo gry | ❌ NIE | wklej fragment, gdy poproszę |
| Moja pamięć „gdzie jesteśmy" | ✅ częściowo | pamięć scoped do Projektu (recency bias) |

Reguła kciuka: **przeskakujesz temat → `/clear` (Code) / nowy czat (Chat) → wklej BOOT.** Nie ciągnij starego kontekstu.

═══════════════════════════════════════════════════════════
# GIT WORKFLOW
═══════════════════════════════════════════════════════════

## Gałęzie (solo dev)
- Pracuj na `main` (trunk-based). Krótkie gałęzie `feat/<nazwa>` TYLKO na ryzykowne etapy (duży refactor,
  eksperyment). W Claude Code: `/branch` przed ryzykowną zmianą, merge po verify.

## Format commitów (oba repo)
```
<typ>(<zakres>): <opis po angielsku>
```
- **typ:** `feat` / `fix` / `perf` / `refactor` / `docs` / `chore`
- **zakres:** podsystem — `sleep`, `temp`, `zone`, `map`, `npc`, `registry`, `econ`, ...
- Przykłady: `feat(sleep): ETAP2 fatigue effects (fog/microsleep/collapse)`,
  `fix(temp): split ambient vs body threshold`, `perf(zone): cache CurrentZone, drop per-tick iterator`.

## `.gitignore` repo gry (UE5 + artefakty pochodne)
```
# UE5
Binaries/
Intermediate/
Saved/
DerivedDataCache/
.vs/
*.VC.db
# Pochodne MapGen (regenerowalne z Tools/MapGen — NIE commitować)
caldreth_data.npz
caldreth_map.png
caldreth_biome.png
```
Trzymamy w gicie: źródła (`Source/`, `Tools/MapGen/*.py`, `caldreth_manifest.json` jako wejście), `Config/`, `.uproject`, żywe docs.
NIE trzymamy: buildów, cache, ciężkich artefaktów pochodnych (regenerowalne ze skryptu).

## Rytuał aktualizacji dokumentów (po KAŻDEJ zmianie kodu)
> CLAUDE.md to kontekst, nie wymuszenie — więc to jest LISTA, nie hook. Jeśli któryś krok ma się
> wykonywać twardo przed każdym commitem, zrób z niego hook (`PreToolUse`/git hook), nie wpis w CLAUDE.md.

1. **CHANGELOG.md** — wpis zmiany + realne wartości zmiennych.
2. **MECHANICS.md** — ground-truth wartości (do testów), jeśli się zmieniły/doszły.
3. **CODE_REGISTRY.md** — nowe klasy/struktury/enumy/funkcje.
4. **DECISIONS.md** — jeśli zapadła decyzja mechaniczna.
5. Commit kodu → **zapisz hash**.
6. Po zamknięciu ETAPU: wpis na górze **MILESTONES.md** (repo komunikacyjne) z tym hashem.

## Domknięcie etapu = brama
Nie zamykaj etapu (ani wpisu w MILESTONES) bez verify twardymi liczbami z logu/live. „Zbudowane" ≠ „zweryfikowane".

═══════════════════════════════════════════════════════════
# GITHUB ACTIONS (CI) — NA RAZIE NIE (świadomie)
═══════════════════════════════════════════════════════════
> Soczewka RTS (koszt/zysk): pełny build UE5 5.7 w GitHub Actions wymaga ciężkich runnerów
> (RAM/dysk), długiego czasu i obejścia licencji Epic na obrazie. Przy solo-devie i weryfikacji
> przez PIE+Monolith — CI nic dziś nie dokłada, a kosztuje. **Pomijamy.**
> Wrócić, GDY: (a) wejdzie druga osoba, albo (b) powstanie warstwa testów C++ (np. Automation Specs),
> którą da się odpalić headless taniej niż pełny PIE. Wtedy minimalny workflow: lint/format + testy jednostkowe,
> nie pełny build edytora.
