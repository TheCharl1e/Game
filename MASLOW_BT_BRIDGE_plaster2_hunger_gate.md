# DESIGN GATE — Most Maslow→BT, PLASTER #2: WARSTWA 1 GŁODU (głód odczuwany → jedzenie)

> **Status: TALK-FIRST. Czeka na „ok" dyrektora. ZERO kodu przed zatwierdzeniem.**
> Mechanika definiująca. Autor: Executor. Grounded w żywym kodzie + `REPORTS/raport_2026-06-20_metabolizm_recon.md`
> + `raport_2026-06-20_maslow_bt_bridge_plaster1.md` + recon TRASH_ (`raport_2026-06-20_trash_recon.md`).
> Wizja: `HUNGER_design.md` (WARSTWA 1, sekcja „plaster #2"). Wzorzec: plaster #1 (pragnienie).

---

## 0. Po co (jedno zdanie)
Bliźniak pragnienia, bogatszy o osobowość: **C++ `Glucose` (nie BP `HungerLevel`)** wyzwala `CurrentNeed=Hunger`,
re-attach odłączonej gałęzi **Handle Hunger** w BT → NPC je; **Neurotyczność modeluje PILNOŚĆ** (nerwowy je
wcześniej) — ten sam lewar co rzut paniki L3-02.

## 1. KONTRAKT (zalockowane: dyrektor + HUNGER_design.md warstwa 1)
1. Wyzwalacz warstwy 1 = **C++ `Glucose ≤ próg`** (proxy true-state; grelina-oscylator = osobny hook PÓŹNIEJ).
2. **`GetActionableNeed()` JUŻ mapuje `Level_1_Nutrition → 1 (Hunger)`** (plaster #1) — bez zmian.
3. **Re-attach** odłączonej „Handle Hunger" pod selektor „Potrzeby", dekorator `CurrentNeed == Hunger(1)`.
4. **OCEAN-pilność = hook lekki:** Neurotyczność wzmacnia pilność (próg głodu wyżej = je wcześniej), wzorzec
   `EvaluatePanicRoll` (`Lerp` z Neurotyzmem). Conscientiousness/gromadzenie = PÓŹNIEJ (wymaga L2 spiżarni).
5. **Kaskada `EHungerPhase` DALEJ liczy fazy** (hook pod warstwę 2) — NIE spłaszczać, NIE ruszać.
6. **Warstwa 2 (rzut pęknięcia, restraint/człowieczeństwo, erozja, kradzież/kanibalizm) = POZA ZAKRESEM.**

## 2. Grounding — co JUŻ jest w żywym kodzie (zweryfikowane)
- `EvaluateCurrentNeed()` (MaslowBiologicalComponent.cpp): w kolejności piramidy, nutrition = `if (Glucose <= CriticalKcalThreshold) return Level_1_Nutrition;` (po panice/temp/hydration/rest). `CriticalKcalThreshold=500` (live).
- `GetActionableNeed()` mapuje `Level_1_Nutrition → 1` (Hunger). **Gotowe.**
- Serwis pisze JEDEN klucz `SetValueAsEnum(CurrentNeed, GetActionableNeed())` (plaster #1). **Hunger pojedzie tym samym torem co Thirst — zero nowej hydrauliki C++→BB.**
- BP `EvaluateNeeds` zapis `CurrentNeed` ZAMROŻONY (4× odpięte w plastrze #1, w tym gałąź Hunger). **Brak nowego freeze'u.**
- Wzorzec OCEAN: `EvaluatePanicRoll` → `CachedIdentity` (TWeakObjectPtr, lazy-init, bez FindComponent co tick) + `EffThr=Lerp(StableThr, NervousThr, Neuroticism)`. **Szablon do skopiowania.**
- BT orphany (recon): „Handle Hunger" (Sequence, **pusta**, `parent_id:null`), `BTTask_FindFood_Experyment`, Eat-subsequence (`MoveTo`+`BTTask_Eat`) — wszystko odłączone od korzenia. BB ma klucze: `NearestFoodLocation`/`NearestFoodObject`/`Finded`.
- Stan C++ TRWAŁY per-instancja (recon TRASH_, H2) → iniekcja Glucose będzie trzymać na świeżym edytorze.

## 3. DECYZJA A — wyzwalacz + OCEAN-pilność

> **✅ ROZSTRZYGNIĘTA 2026-06-21 (Architekt): OBOK, nie w sędzim.** Modulacja Neurotyzmu liczy się na
> kadencji metabolizmu (tuż po `EvaluatePanicRoll()`, reużycie `CachedIdentity`) i zapisuje pole transient
> `float EffectiveKcalThreshold = Lerp(StableKcalThr, NervousKcalThr, Neuroticism)`. `EvaluateCurrentNeed()`
> rung nutrition CZYTA gołego floata: `if (Glucose <= EffectiveKcalThreshold) ...` — zero Lerpa/identity
> w sędzim, dokładny bliźniak latcha `bIsInPanic`. **Override rekomendacji §3 niżej (Lerp w sędzim).**
> Powód: kod już kanonizuje „NO RNG/personality in the judge" (komentarze MaslowBiologicalComponent l. 532/556/727).
> Init `EffectiveKcalThreshold = StableKcalThr` → baseline gotowy przed 1. kadencją (kasuje potrzebę lazy-init guardu).
> Defaulty: `StableKcalThr=500 (=CriticalKcalThreshold)`, `NervousKcalThr=650`. Killer-demo §9 bez zmian + read-back pola.



**Próg efektywny modulowany Neurotyzmem (ten sam lewar co panika):**
```
// SZKIC — do zatwierdzenia
EffKcalThreshold = Lerp(StableKcalThr, NervousKcalThr, Neuroticism)   // NervousKcalThr > StableKcalThr
// nutrition w EvaluateCurrentNeed: Glucose <= EffKcalThreshold  (zamiast stałego CriticalKcalThreshold)
```
- UPROPERTY tunable (nazewnictwo jak panika): `StableKcalThr` (spokojny, ~500 = obecny próg), `NervousKcalThr` (nerwowy, np. ~650 = je wcześniej, przy wyższym Glucose). Neurotyzm czytany z `CachedIdentity` (reużycie cache'u z paniki; lazy-init guard, bo `EvaluateCurrentNeed` może odpalić przed pierwszym `EvaluatePanicRoll`).
- `CriticalKcalThreshold` (500) **zostaje** jako twarda podłoga/baseline (np. `StableKcalThr=CriticalKcalThreshold`).
- **Kaskada NIETKNIĘTA** — `EHungerPhase` liczy się dalej niezależnie.

**Decyzja do podjęcia (Twoja):**
- **GDZIE modulować:** (A) wprost w `EvaluateCurrentNeed` (jedyny sędzia → spójny tor do BT; „poziom" też odzwierciedli pilność — pożądane, nerwowy NPC realnie priorytetyzuje jedzenie wcześniej) ← **rekomendacja**; vs (B) osobny pre-check tylko dla Huntera (więcej kodu, rozjazd z sędzią).
  🚩 Flaga (A): OCEAN wchodzi do współdzielonego sędziego. Konsument jest jeden (serwis→BT; HUD nie woła) → ryzyko niskie.
- **Helper:** `GetEffectiveKcalThreshold()` (private, lazy-cache identity) wołany przez `EvaluateCurrentNeed` — enkapsuluje modulację, jedno miejsce.

## 4. DECYZJA B — re-attach „Handle Hunger" (struktura, lustro Handle Thirst)
Cel (mirror działającej Handle Thirst), pod selektorem „Potrzeby":
```
Selector "Potrzeby"
 ├─ Handle Thirst   [dekorator: CurrentNeed == Thirst(2)]   (JEST)
 ├─ Handle Hunger   [dekorator: CurrentNeed == Hunger(1)]   (RE-ATTACH)
 │    ├─ BTTask_FindFood_Experyment            (ustawia NearestFoodLocation)
 │    ├─ Sequence [dekorator: NearestFoodLocation Is Set]
 │    │     ├─ Move To (BlackboardKey = NearestFoodLocation)
 │    │     └─ BTTask_Eat
 │    └─ BTTask_Check
 └─ Handle Sleep    [dekorator: CurrentNeed == Sleep(3)]    (pusta — plaster #3)
```
- Wykorzystać istniejące orphany (`Handle Hunger` seq, `FindFood_Experyment`, `MoveTo`+`Eat`) — wpiąć/poskładać, nie tworzyć od zera, gdzie się da. Dekoratory enumowe (jak Thirst) — zero zmian w serwisie/kluczu.
- Autoring w edytorze przez Monolith `ai_query` (add_bt_decorator / move_bt_node / reorder), zapis ścieżką silnikową (NIE Monolith save dla BP/BT? — BT_NPC zapisywany OK w plastrze #1 przez `EditorAssetLibrary.save_asset`; trzymam się tego).
- Kolejność dzieci selektora: Thirst → Hunger → Sleep (pragnienie zabija szybciej; spójne z piramidą i z `EvaluateCurrentNeed`).

## 5. DECYZJA C — GetActionableNeed: bez zmian
Mapuje `Nutrition→1`. Tylko **weryfikacja** read-back. Zero kodu.

## 6. DECYZJA D — BP freeze: bez nowego
Zapis `CurrentNeed` z `EvaluateNeeds` już zamrożony (plaster #1, wszystkie 4 gałęzie). **Weryfikacja** find-references, że nic nowego nie pisze Huntera. Zero kodu.

## 7. ZAKRES PLASTRA #2 (warstwa 1)
**W zakresie:** EffKcalThr (Neurotyzm-Lerp) w C++; re-attach Handle Hunger; dowód „C++ Glucose je" + killer-demo OCEAN-pilności.
**POZA (warstwa 2 / później):** rzut pęknięcia, restraint=człowieczeństwo×OCEAN, erozja głodówki, kradzież/kanibalizm, grelina-oscylator (prawdziwy felt-signal), maski/arbitraż, Conscientiousness-gromadzenie (wymaga L2), pusta Handle Sleep (plaster #3), pułapka omdlenia.

## 8. KROKI IMPLEMENTACJI (po „ok")
1. C++: `GetEffectiveKcalThreshold()` + podmiana w `EvaluateCurrentNeed` (nutrition) + UPROPERTY `StableKcalThr`/`NervousKcalThr`. Build edytor-zamknięty (Build.bat UHT).
2. BT: re-attach Handle Hunger (dekorator + wpięcie food-tasków), zapis silnikowy. Read-back struktury.
3. Weryfikacja find-references (D).

## 9. DOWÓD (twarde liczby, ŚWIEŻY edytor + krótka sesja — higiena anty-TRASH_)
- **C++ Glucose steruje (lustro plastra #1):** zepsuj C++ `Glucose < EffKcalThr`, **BP `HungerLevel` NIETKNIĘTY** → `EvaluateCurrentNeed=Nutrition` → `GetActionableNeed=1` → BB `CurrentNeed=1` → BT wchodzi w **Handle Hunger** (FindFood→MoveTo→Eat). Kontrast: BP `HungerLevel` bez zmian.
- **Killer-demo OCEAN-pilności (deterministyczny, in-script, odporny na homeostazę):** dwa NPC, **to samo Glucose** (np. 600), C1 Neurotyzm=0.9 → `EffKcalThr≈650` → 600≤650 → `GetActionableNeed=1` (Hunger); C2 Neurotyzm=0.1 → `EffKcalThr≈515` → 600>515 → `=0` (None). Ta sama glukoza, inna potrzeba — jak demo paniki.
- **Reverse:** Glucose z powrotem > EffKcalThr → `GetActionableNeed` opuszcza Hunger.
- Wszystko czytane z live + log; BP HungerLevel logowany jako niezmieniony.

## 10. FLAGI / pytania odłożone (z HUNGER_design.md — NIE w tym plastrze)
- Grelina-oscylator (felt prawdziwy) — warstwa 1 używa proxy Glucose; oscylator = osobny hook.
- Conscientiousness-gromadzenie — wymaga L2 spiżarni.
- E_NeedState wpis 4 (Flee) — niezweryfikowany (dotyczy plastra #4, nie tu).
- Warstwa 2 cała — osobny gate, gdy L2 własność + L0 panika dojrzeją.

## 11. CZEGO ŻĄDA „ok"
Autoryzacja: decyzji A (modulacja w EvaluateCurrentNeed via helper, A vs B + wartości StableKcalThr/NervousKcalThr),
B (struktura re-attach Handle Hunger), potwierdzenie C/D (bez zmian) i zakresu warstwy 1. Po „ok" → kod, build,
re-attach, dowód PIE (świeży edytor), korekta docs, commit. **Po plastrze #2 STOP — kolejny gate.**

**NIE koduję nic przed „ok".**
