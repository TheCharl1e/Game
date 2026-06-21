# Raport recon — TRASH_ / trwałość stanu C++ (H1 vs H2)

**Data:** 2026-06-20
**Zlecenie:** dyrektor — read-only, ŚWIEŻY edytor + ŚWIEŻE PIE + MINIMUM operacji. Rozstrzygnąć:
H1 (realna re-inicjalizacja configu C++ przez warstwę BP co tick) vs H2 (artefakt pomiaru — set/get trafiają w różne instancje przez TRASH_ reinstancing). Zamknąć TECH-09.
**Werdykt:** ✅ **H2 — artefakt pomiaru.** Stan `UMaslowBiologicalComponent` jest TRWAŁY per-instancja. „Reset configu" z plastra #1 był duchem skażonej sesji (ten sam, co poranny „pin HP"). **TECH-09 ZAMKNIĘTE: churn TRASH_ = artefakt zmęczonej sesji, znika na świeżym edytorze.**

---

## Warunki
Świeży edytor (ubity + relaunch, porty 8090/9316 wolne przed startem), świeże PIE na `/Game/DocelowaGra/Game`, minimum operacji. Kontrast vs plaster #1: tamta sesja była DŁUGA (rebuild + reopen + wiele start/stop PIE + reanimacje + powtarzane collapse) → akumulacja TRASH_.

## Warstwa kodu (read-only, z dysku)
`MaslowBiologicalComponent.cpp`: `CriticalHydrationThreshold = 20` ustawiane **wyłącznie w konstruktorze** (l.58, koniec ctor l.61) — nic innego go nie pisze. `CurrentHydration`: init 100 (l.51), **drenaż** w `ProcessMetabolism` (l.127 `-= burn`), **wzrost tylko przy piciu** (l.601 `+= WaterMl`, `ConsumeFoodItem`). → **Brak legalnego regen UP poza piciem; brak legalnego zapisu progu poza ctor.** Każdy runtime-owy „revert do 20" / „skok hydration w górę" musi więc oznaczać świeżą/inną instancję, nie re-init żywej.

## Twarde liczby (świeże PIE, NPC `BP_NPC_Character_C_1`)

### Krok 1 — enumeracja
`get_components_by_class(UMaslowBiologicalComponent)` → **count = 1**, jedyna instancja:
`...BP_NPC_Character_C_1.MaslowBiological`. **Brak prefiksu TRASH_.** Ponowna enumeracja (RECON B) → wciąż count=1, ta sama nazwa. **Zero churnu, zero inkrementu suffixu.**

### Krok 2 — która instancja żyje
Jedna instancja = ona jest żywa (wartości się ruszają: HP 100→70, hydration drenuje). `get_component_by_class` zwraca DOKŁADNIE ją (`same_obj=True`, ta sama ścieżka). Na czystej sesji get_by_class jest wiarygodne.

### Krok 3 — test trwałości (atomowy, klucz H1/H2)
```
RECONA: before=20.0  → set 99.0 → after_sameref=99.0  | fresh get: thr=99.0  same_obj=True
RECONB (po 4 s ticków): sameobj_thr_after_ticks=99.0  | fresh get: thr=99.0   count=1
```
→ Set `CriticalHydrationThreshold=99` **TRZYMA** — na tej samej ref, na świeżym get, i **PO TICKACH**. **Żadnej re-inicjalizacji configu.**

### Krok 4 — odróżnienie dwóch objawów
- **`CriticalHydrationThreshold`** (config): set 99 → **trwa 99** przez wiele ticków. Brak „zapachu". (W plastrze #1 cofał się do 20 — to był odczyt INNEJ/martwej instancji.)
- **`CurrentHydration`** (sim): set 5.0 → po ~6 s ta sama instancja = **0.0** (drenaż W DÓŁ, zgodnie z `ProcessMetabolism`), HP 100→70 (legalna kara odwodnienia), eval=`LEVEL_1_HYDRATION`, need=2. **NIE „regeneruje w górę"** — plaster-#1-owy skok 0→86 był odczytem martwej/CDO instancji (default), nie regenem.

## Wniosek: H2
Na świeżej sesji: **jedna trwała instancja, set/get spójne, config nie drga, hydration drenuje legalnie.** Wszystkie objawy „homeostatycznego resetu" z plastra #1 (hydration 0→86, próg 99→20) to **artefakt TRASH_ reinstancingu** zmęczonej sesji — `get_component_by_class` zwracał czasem martwą/CDO instancję (defaulty: próg 20, hydration ~100), gdy `set` trafiał w żywą. Ten sam duch, co poranny „pin HP=100" (raport recon metabolizmu, TECH-08).

## Skutki
1. **TECH-09 ZAMKNIĘTE:** churn `TRASH_` w PIE = artefakt zmęczonej sesji edytora. Na świeżym edytorze count=1, brak TRASH_. (Higiena: przy weryfikacjach iniekcyjnych — świeży edytor.)
2. **WYCOFANA flaga z plastra #1** („ukryty per-tick BP→C++ re-init / kto karmi C++ Hydration"). **Nie ma takiego mechanizmu.** Most jest CZYSTSZY niż wyglądał: C++ trzyma swój stan, hydration drenuje legalnie. „C++ thirst tylko przy realnym deficycie" pozostaje prawdą, ale z trywialnego powodu (zdrowy NPC ma wysokie hydration), nie przez ukryty reset.
3. **Dowód główny plastra #1 stoi** (był self-consistent: eval=Hydration ∧ need=2 ∧ BB=2 ∧ FindWater) i został tu **niezależnie odtworzony** na czystej sesji (set Hyd=5→drain→eval=Hydration→need=2).

## Rekomendacja
Most jest zdrowy — **droga wolna do plastra #2 (głód)**. Higiena weryfikacji: świeży edytor + krótka sesja, by uniknąć duchów TRASH_.

## Nie zrobione (świadomie)
Globalny skan wszystkich UObject pod TRASH_ — zbędny: `get_components_by_class` na aktorze = 1, brak TRASH_ w nazwach (poranny churn pokazywał TRASH_ właśnie w ścieżce komponentu). NIE naprawiano niczego.
