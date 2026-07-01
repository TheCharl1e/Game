# L1 — Pragnienie (Thirst / Hydration)

**Oś:** L (NPC) · **Warstwa:** L1 · **Status:** ✅ · **Migrowano:** ETAP 2 (2026-07-01). Strzałka-w-dół: L1 zna tylko L0. Single-bridge BB = `BTService_MaslowBlackboardSync`.

> **Uwaga o kompletności:** w `DESIGN_how_it_works.md` pragnienie jest opisane **skąpo** — jeden akapit designowy (CZ.I, sekcja „Pragnienie", oznaczona ✅) plus kilka odwołań świat↔NPC. Poniższy destylat zawiera **wyłącznie** to, co jest w źródle. Wszystko, czego tu nie ma (np. dokładny wzór recovery przy piciu, cadence ticku, wartość progu krytycznego), **nie jest zdefiniowane w źródle** i wymaga osobnej bramki designowej.

## Rola w systemie
- **Co robi:** pragnienie/nawodnienie jest **paliwem dla staminy**; odwodnienie **blokuje regenerację** i **szybko zabija**.
- Status w źródle: ✅ (mechanika uznana za zaimplementowaną / zdefiniowaną).

## Model nawodnienia (CurrentHydration)
- Zasób nawodnienia ma **maksimum = 100** (Hydration max 100).
- Nawodnienie **spada w czasie** (drenaż), a tempo spadku jest **skalowane aktywnością NPC**.

## Skalowanie aktywnością (data-driven)
Mnożnik tempa spadku zależny od bieżącej aktywności, pobierany z **Data Table (DT)** — zero hardkodu:

| Aktywność | Mnożnik spadku |
|-----------|----------------|
| Rest (odpoczynek) | ×1 |
| Work (praca) | ×3 |
| Combat (walka) | ×5 |

## Picie ze źródła wody → recovery
- **Źródła wody są elementem mapy świata** — woda występuje **tylko przy rzekach i wybrzeżu** (wyspa Caldreth). Aspekt świata → [[C2_water]].
- Picie z takiego źródła odnawia nawodnienie (recovery). *Dokładny model/ilość recovery nie jest w źródle wyszczególniony.*
- **Most NPC↔świat:** woda stała = miejsce, przy którym NPC osiada (rzeki, wybrzeże).

## Odwodnienie → HP (próg krytyczny)
- Gdy nawodnienie osiągnie stan odwodnienia: **−10 HP / tick** (kara zdrowotna).
- W skrajności odwodnienie **szybko zabija** (patrz: „Rola w systemie").
- Odwodnienie dodatkowo **blokuje regenerację**.
- *Konkretna liczbowa wartość progu, przy której naliczana jest kara −10 HP/tick, nie jest podana w źródle.*

## Powiązania ze światem (susza / pogoda)
- **Susza (pogoda) wysusza wadi** → strumienie sezonowe znikają → NPC **musi migrować do wody stałej**.
- Deszcz → wilgoć stref → wadi płyną/wysychają; klimat = średnia z pogody.
- To jest jawnie nazwany **„most NPC↔świat"** — geografia i pogoda bezpośrednio sterują zachowaniem pragnienia.

## Haczyki do rozbudowy (nie zaimplementowane — z źródła)
- **Magazynowanie wody** (bukłaki) jako item **L2** → NPC planuje wyprawy w głąb lądu.
- **Skażona woda → choroba** (most do systemu medycznego).

**Źródło:** DESIGN_how_it_works.md CZ.I sekcja Hydration (Game_git)
