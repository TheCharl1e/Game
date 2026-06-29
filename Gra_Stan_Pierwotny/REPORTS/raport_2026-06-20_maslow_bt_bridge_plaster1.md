# Raport — Most Maslow→BT, PLASTER #1 (pragnienie): kod + weryfikacja PIE

**Data:** 2026-06-20
**Gate:** `MASLOW_BT_BRIDGE_design.md` — „OK" dyrektora na plaster #1, decyzje #1–#4 zatwierdzone.
**Wynik:** ✅ **Most działa — C++ steruje wyborem akcji.** Zepsucie C++ Hydration (BP ThirstLevel NIETKNIĘTY) → `CurrentNeed=Thirst` (pisany przez C++) → BT wchodzi w Handle Thirst (FindWater). Build zielony, PIE-verified twardymi liczbami.

---

## Co zrobione (patch, bez regen)

### C++ (build edytor-zamknięty, Build.bat UHT → **Result: Succeeded**, 59.85 s)
- **`UMaslowBiologicalComponent::GetActionableNeed()`** (`MaslowBiologicalComponent.{h,cpp}`) — nowy `UFUNCTION(BlueprintPure)`, zwraca `uint8` (E_NeedState: 0 None/1 Hunger/2 Thirst/3 Sleep/4 Flee). **Deleguje do `EvaluateCurrentNeed()`** (jedno źródło drabiny priorytetów, piramida top-down, pierwsza trafiona) i tylko MAPUJE poziom→konkret. `EvaluateCurrentNeed` NIETKNIĘTY (zwraca POZIOM, dla HUD). Surowy uint8 = brak drugiego enuma do lockstepu.
- **`UBTService_MaslowBlackboardSync`** (`.h/.cpp`) — usunięte 6 mis-bindowanych selektorów (`MaslowPriority/Hydration/Glucose/HP/IsInPanic/IsStarving`), JEDEN klucz `CurrentNeedKey`. TickNode: jeden poprawnie typowany zapis `BB->SetValueAsEnum(CurrentNeedKey, GetActionableNeed())`. Skasowane: pchanie 6 kluczy w CurrentNeed + zerowanie CurrentNeed.

### Assety (zapis ścieżką silnikową `EditorAssetLibrary.save_asset`, NIE Monolith — patrz pamięć RF_Standalone; brak ostrzeżenia o utracie danych)
- **BT_NPC** — serwis `CurrentNeedKey` ⟶ związany z kluczem `CurrentNeed` (read-back potwierdzony). Dekoratory Handle Thirst (`==Thirst`/2), Handle Sleep (`==Sleep`/3) nietknięte.
- **BP_NPC_Character / `EvaluateNeeds`** — **ZAMROŻENIE (odwracalne):** odpięte exec-wejścia 4× `SetValueAsEnum(CurrentNeed)` (węzły `CallFunction_20/25/30/0` zostają, `connected_to:[]` potwierdzone). Kompilacja czysta (0 błędów). BP nie pisze już klucza potrzeby; zostaje przy wykonaniu.
- **find-references (precondition #3):** BB `CurrentNeed` czytany WYŁĄCZNIE przez 2 dekoratory; zmienna BP `CurrentNeed` — 0 odczytów; BT-taski nie czytają. **Zero driftu, freeze bezpieczny.**

---

## Dowód PIE (twarde liczby, mapa `/Game/DocelowaGra/Game`)

### Dowód GŁÓWNY — C++ Hydration steruje (sesja z realnym deficytem)
Zepsuto C++ `CurrentHydration` (utrzymane na 0), **BP `ThirstLevel=0.0` przez cały czas**:
| Sygnał | Wartość |
|---|---|
| `CurrentHydration` (C++) | **0.0** (próg `CriticalHydrationThreshold=20`) |
| `EvaluateCurrentNeed()` | **LEVEL_1_HYDRATION (2)** |
| `GetActionableNeed()` | **2 (Thirst)** |
| BB `CurrentNeed` (czytane runtime) | **2 (Thirst)** ← pisane przez C++ serwis |
| BT aktywny węzeł | **`BTTask_FindWater_Experyment`** (gałąź Handle Thirst), index 4, is_running=true |
| **BP `ThirstLevel`** | **0.0 (NIETKNIĘTY)** |

→ Steruje **C++ Hydration**, nie BP ThirstLevel. Dekorator enumowy przeszedł, NPC wszedł w łańcuch picia.

### Stan zdrowy (steady-state, read-only, wielokrotnie potwierdzony)
`CurrentHydration` wysokie (86–94) → `EvaluateCurrentNeed=SATISFIED` → `GetActionableNeed=0` → BB `CurrentNeed=0` → BT bezczynny (`active_node=None`). Reverse na poziomie gettera: `CurrentHydration=94 → GetActionableNeed=0`.

→ `CurrentNeed` podąża za C++ Hydration w obie strony (2 przy deficycie, 0 przy nasyceniu). Serwis pisze BB co ~1 s z gettera = sprzężenie ciągłe, nie jednorazowe.

---

## 🚩 FINDING / FLAGA (nowy, do roadmapy mostu) — homeostatyczny reset stanu C++
> ⚠️ **WYCOFANE 2026-06-20** (`raport_2026-06-20_trash_recon.md`): poniższy „reset" to był **H2 — artefakt TRASH_** skażonej sesji, NIE realny re-init. Na świeżym edytorze stan C++ jest trwały (set configu trzyma przez ticki; hydration drenuje legalnie). Most jest czysty. Treść niżej zostawiona dla historii.
Podczas prób puppetowania: wstrzyknięte do komponentu C++ wartości **wracają w ~2 s**:
- `CurrentHydration` ustawione na 0 → regeneruje do ~86 (na zdrowym NPC).
- **`CriticalHydrationThreshold` ustawione na 99 → wraca do 20** (read-only po ticku).

Próg konfiguracyjny NIE powinien być ruszany przez metabolizm → to **re-inicjalizacja configu/stanu komponentu C++ co tick**, najpewniej warstwą BP (`CharacterStats`/`ProcessMetabolism` re-pushujące dane). **Refinement TECH-08:** systemy NIE są w pełni rozłączone — istnieje ukryty per-tick wpływ na stan C++. Skutek praktyczny: **C++ pragnienie odpala się tylko przy GENUINE deficycie** (warstwa utrzymuje C++ hydration wysoko, gdy BP nie jest spragniony). Dlatego na świeżym NPC iniekcja jest natychmiast nadpisywana; dowód główny złapał realny, utrzymany deficyt. **Do rozplotu w kolejnych plastrach mostu** (kto karmi C++ Hydration).

---

## Dług / scaffolding (odwracalne, NIE w kodzie)
- Obejście pułapki omdlenia na czas testu: próby przez progi snu (`CollapseThreshold` itd.) **nie chwyciły** (pola nie-EditAnywhere → `set_editor_property` odrzucone) → użyto zerowania `HoursAwake` + krótkich okien. Pułapka omdlenia (pusta Handle Sleep + brak wyjścia) **NADAL ISTNIEJE** — trwały fix = plaster #3 mostu.
- FindWater na mapie Game bywa bezowocny (woda/nawigacja) — osobny temat, nie most.

## Infra
- Edytor uruchamiany przeze mnie; build edytor-zamknięty (Build.bat). Zapisy BP/BT ścieżką silnikową (nie Monolith). `CaldrethMap.umap` pokazał się jako zmieniony mimo braku mojej ingerencji (edytor/PIE dotknął startup-mapy) — **wykluczony z commita**.

## STOP — wg gate: po plastrze #1 czekam na kolejny gate przed plastrem #2 (głód).
