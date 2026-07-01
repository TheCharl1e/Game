# L2 — Bezpieczeństwo i stabilność psychiczna

**Oś:** L (NPC) · **Warstwa:** L2 · **Status:** 🔨 · Strzałka-w-dół: L2 zna L0-L1. Single-bridge BB = `BTService_MaslowBlackboardSync`.

**Zakres:** bezpieczeństwo fizyczne + stabilność psychiczna NPC. Obejmuje ekwipunek per-kontener + model własności (`OwnerID`, kradzież), spiżarnię/magazyn wioski, oraz — jako **PODROZDZIAŁ, nie osobna warstwa** — **człowieczeństwo/maski** (głębia psychologiczna). W G1 warstwa śpi.

## Podrozdziały
- **L2.1 Bezpieczeństwo fizyczne** — ekwipunek (Tarkov-style per-kontener), własność `OwnerID` (int32, przeżywa śmierć właściciela), spiżarnia publiczna (`PUBLIC_OWNER_ID`), `DropContainer` (utrata ładunku przy ucieczce/kradzieży).
- **L2.2 Stabilność psychiczna / człowieczeństwo** — „maski"; restraint jako hamulec zachowań skrajnych (kradzież/kanibalizm z [[L1_hunger]] warstwa 2); erozja człowieczeństwa pod przewlekłym stresem/głodówką (efekt Minnesota). Źródło TBD — `NPC_DEEPENING_concepts.md` nie istnieje, treść powstanie później (hook zostawiony).

**Źródło treści:** `DESIGN_how_it_works.md` CZ.I (L2) + `InventoryComponent` (własność) + `NPC_DEEPENING_concepts.md` (człowieczeństwo — źródło TBD, patrz `_gates/MIGRATION_MAP.md` #7).

> SZKIELET (ETAP 1, nagłówek ujednolicony ETAP 2). Treść warstwy L2 migrowana gdy L2 dojrzeje (ETAP 3, osobna sesja per-warstwa).
