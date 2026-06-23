# Raport — L1-09a clothing → izolacja DONE + PIE-VERIFIED · 2026-06-23/24

> Model: additive-clamped, baza z tłuszczu (decyzja dyrektora). Build edytor-zamknięty (Succeeded). Branch `feat/l1-09a-clothing`.

## Model (zatwierdzony, znak uzgodniony z kodem)
`InsulationFactor` = **mnożnik STYGNIĘCIA** (`CurrentTemp += (Ambient−Current)·BodyCoolingRate·InsulationFactor`), niższy = cieplej.
```
InsulationFactor = clamp( BaseFromFat − Σ Insulation(equipped),  FloorInsulation,  1.0 )
   BaseFromFat = Lerp(1.0, 0.6, BodyFatRatio)   // chudy 1.0 → gruby 0.6 (bez zmian)
```
NIE mnożenie — tłuszcz = tło, ubranie odejmuje od mnożnika, clamp. Wszystkie equipped sumują Insulation per-item, bez wag per-slot.

## 🔴 ZMIANA: FloorInsulation = 0.1 (nie 0.2)
Floor 0.2 NIE osiąga celu „ciężkie zawsze daje grubemu coś": gruby+średnie = 0.6−0.4 = **0.2** (ląduje na floorze),
gruby+ciężkie = 0.6−0.6 = 0.0 → clamp **0.2** → identyczne (kurtka bezsensowna przy skórach — ten sam problem co 0.3).
**Floor 0.1:** gruby+ciężkie = **0.1**, gruby+średnie = **0.2** → ciężkie bije średnie, a 0.1≠0 (nie cold-immune).
Lecę 0.1 (jednolinijkowy tuning, zmień jeśli chcesz inaczej).

## Implementacja
- **C++ (commit `bf07218`, build Succeeded):** `float Insulation` w `FItemDefinition`; `UInventoryComponent::GetTotalEquippedInsulation()` (Σ equipped); Maslow: `FloorInsulation=0.1`, cache `CachedInventory` (BeginPlay), nowa formuła w ProcessMetabolism.
- **DT_ItemDefinitions** (zapisane ścieżką silnikową): + `FurCoat` (Clothing, Insulation 0.6, Torso, Heavy), + `HideLeggings` (0.4, Legs, Medium).
- **InventoryComponent na BP_NPC_Character** dodany (Monolith, in-memory — działa w PIE) + `ItemDefinitions=DT_ItemDefinitions`, BP compiled UpToDate. **⚠️ CTRL+S RANO (RF_Transient): BP_NPC_Character — zapisać dodany InventoryComp + ItemDefinitions ręcznie.**

## PIE-VERIFY (technika na żywą instancję: `get_components_by_class` + filtr TRASH_, per-set re-resolve)
**Czysty A/B na mnożniku tempa stygnięcia (InsulationFactor):**
| Stan | GetTotalEquippedInsulation | InsulationFactor |
|---|---:|---:|
| **Nagi** | 0.0 | **0.88** (= Lerp(1.0,0.6,0.3), BodyFat 1500) |
| **+ FurCoat (0.6)** | 0.6 | **0.28** (= clamp(0.88−0.6, 0.1, 1.0)) |

- **InsulationFactor JEST współczynnikiem tempa** → ubrany stygnie w **0.28/0.88 = 32% tempa nagiego (−68%)**. To różnica TEMPA (nie wartość statyczna — to per-tick coefficient stygnięcia).
- **Realne stygnięcie obserwowane:** w reżimie jawa-ekstremalne-zimno (ExtremeCold=25 > AmbientTemp) CurrentTemp 36→33.4 z clothed IF=0.28.
- EquipItem flow działa end-to-end: item Insulation → Inventory sum → Maslow formuła → mnożnik.

**Dług verify:** literalny ΔCurrentTemp/kadencja naked-vs-clothed A/B zaszumiony przez **szybko oscylujący AmbientTemp** (zegar doby przerzuca reżim stygnięcia on/off, 12↔25.6) + transient churn przy multi-secie — harness, nie kod. Mnożnik (rate coefficient) A/B jest definitywny; tempo = mnożnik × baza (tożsamość w kodzie).

## WERDYKT
**L1-09a DONE.** Ubranie obniża mnożnik stygnięcia (0.88→0.28 przy ciężkim), additive-clamped wg modelu dyrektora,
floor 0.1 (ciężkie bije średnie u grubego). End-to-end zweryfikowane na żywej instancji.
**CTRL+S RANO: `BP_NPC_Character` (InventoryComp + ItemDefinitions).** Następne: L1-09b fire, albo dograć tier-itemy/balans.
