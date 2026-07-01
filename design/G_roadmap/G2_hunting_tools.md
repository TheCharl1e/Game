# G2 — Polowanie i narzędzia (stub)

Oś: G (grywalny stan) · Status: ⬜ (stub — pełny gate w osobnej sesji)

## Czym G2 będzie (skrót)
Kolejny grywalny stan ponad G1: **polowanie na jelenia** (combat ofensywny — NPC/wilk atakuje prey),
**pierwszy tech-plaster** (ostry kamień jako prymitywne narzędzie/broń), oraz **mięso/padlina**
(zwłoki jako źródło kalorii — `ACorpseBase`/`ExtractMeat`, `RawMeat` z `DT_ItemDefinitions`).

## Kompozycja (wstępnie)
C (zasoby: zwierzyna) + L (L0 walka ofensywna, L1 głód→mięso) + gracz-obserwator (jak G1).

## Wchodzi z G1 (odłożone tam świadomie)
- Polowanie (w G1 wilk = tylko zagrożenie testujące L0; jeleń = żywy prey bez mechaniki mięsa).
- Padlina/zwłoki jako jedzenie (w G1 zwłoki znikają po śmierci).
- Narzędzia/crafting (poza G1).

## DoD (do doprecyzowania w gate G2)
Killer-demo: NPC/wilk poluje na jelenia → zabija → mięso → głód spada; pierwsze narzędzie zwiększa
skuteczność. Falsyfikowalne, log-backed.

> Źródło treści: PROGRESSION_DESIGN.md + RPG_DESIGN_CANON.md (Game_git) + ROADMAP (L2-10 crafting, L2-09 raw meat). Szkielet — bez migracji.
