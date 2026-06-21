# DESIGN GATE — Most Maslow → BT (C++ steruje, BP = cienkie ciało)

> **Status: TALK-FIRST. Czeka na „ok" dyrektora. ZERO kodu przed zatwierdzeniem.**
> Mechanika definiująca. Autor: Executor (Claude Code). Grounded w recon
> `Gra_Stan_Pierwotny/REPORTS/raport_2026-06-20_metabolizm_recon.md`.
> Kierunek zatwierdzony przez dyrektora 2026-06-20: **C++ `UMaslowBiologicalComponent` steruje wyborem
> akcji; BP zostaje wyłącznie przy WYKONANIU** (MoveTo/Drink/Eat/anim).

---

## 0. Dlaczego (z reconu, twardo)
- Dekoratory pod selektorem „Potrzeby" bramkują wybór gałęzi **wyłącznie kluczem `CurrentNeed`** (Enum `E_NeedState`): Handle Thirst `==Thirst`, Handle Sleep `==Sleep`.
- `CurrentNeed` pisze poprawnie **tylko BP** (`EvaluateNeeds` → `SetValueAsEnum`). C++ serwis ma 6 selektorów wpiętych w `CurrentNeed` + mismatch typów → **(a)** dedykowane klucze Maslow nigdy nie zapisane (live=0/false przy realnym HP=100/Glucose=740), **(b)** serwis **zeruje `CurrentNeed` co ~1 s** (test różnicowy: BT stoi→klucz trzyma 4 s+; BT działa→wraca do 0 w ≤3 s).
- Kaskada kataboliczna + panika L3-02 są **policzone, ale nieczytane** przez BT.

Cel mostu: **C++ jest jedynym, poprawnie typowanym pisarzem konkretnej potrzeby do BT.**

---

## 1. KONTRAKT ARCHITEKTA (zalockowane przez dyrektora — wpisane do gate)
1. **Granularność:** C++ mówi BT KONKRETNĄ potrzebę (Thirst/Hunger/Sleep/Flee), nie poziom Maslowa.
2. **Jeden klucz, jeden pisarz:** C++ pisze JEDEN autorytatywny, poprawnie typowany klucz. Serwis przestaje pchać 6 kluczy w `CurrentNeed` i przestaje go zerować.
3. **BP decyzja wyłączona:** `EvaluateNeeds` przestaje PISAĆ klucz potrzeby — **NAJPIERW ZAMROŻENIE (odpięcie), nie kasowanie.** BP zostaje przy wykonaniu.
4. **Thin-slice:** plaster #1 = PRAGNIENIE end-to-end (Handle Thirst jako jedyna wpięta gałąź). Reszta = roadmapa.

---

## 2. POTWIERDZENIE TYPÓW I MAPOWANIA (wobec żywego kodu — wymóg dyrektora)

**`EMaslowPriority`** (C++ `uint8`, z nagłówka, linie 102–111):
`0` Level_0_FightOrFlight · `1` Level_1_Temperature · `2` Level_1_Hydration · `3` Level_1_Rest · `4` Level_1_Nutrition · `5` Level_2_Safety · `6` Satisfied.

**`E_NeedState`** (BP UserDefinedEnum `uint8`): `0` None · `1` Hunger · `2` Thirst · `3` Sleep · `4–7` istnieją (Switch ma 8 wyjść), **nazwy NIEZWERYFIKOWANE** (Python `num_enums` niedostępny; potwierdzone z dekoratorów IntValue 2/3 + Print-stringów EvaluateNeeds).

🚩 **PUŁAPKA MAPOWANIA — kluczowa dla decyzji #2:** enumy **NIE są wyrównane**.

| Pojęcie | EMaslowPriority | E_NeedState | Zgodność |
|---|---|---|---|
| Panika | 0 (FightOrFlight) | 0 = **None** | ❌ kolizja (panika→„nic") |
| Pragnienie | 2 (Hydration) | 2 = Thirst | ✅ (przypadkiem) |
| Sen | 3 (Rest) | 3 = Sleep | ✅ (przypadkiem) |
| Głód | 4 (Nutrition) | 1 = Hunger | ❌ |

**Wniosek:** surowy `static_cast<E_NeedState>(EMaslowPriority)` jest BŁĘDNY. Most MUSI **tłumaczyć**, nie castować. Klucze typów dekoratorów to **Enum** (porównania „Is Equal To Thirst") — nie Int.

---

## 3. DECYZJA #1 — kształt gettera (propozycja)

**Propozycja:** nowy `UFUNCTION(BlueprintPure)` zwracający **konkretną, akcjonowalną potrzebę**, liczony NA ŻĄDANIE z istniejących pól (bez nowego przechowywanego stanu = bez drugiego źródła prawdy).

```
// SZKIC (nie kod do wklejenia — do zatwierdzenia)
UENUM(BlueprintType)
enum class ENPCNeed : uint8 {           // wartości w LOCKSTEP z BP E_NeedState
    None   = 0,
    Hunger = 1,
    Thirst = 2,
    Sleep  = 3,
    Flee   = 4   // (wymaga potwierdzenia/utworzenia wpisu 4 w E_NeedState)
};

UFUNCTION(BlueprintPure, Category="Maslow")
ENPCNeed GetActionableNeed() const;     // drabina priorytetów, zwraca KONKRET
```

Logika (ta sama drabina co `EvaluateCurrentNeed`, ale mapująca poziom→konkret; liczone z `bIsInPanic`/`CurrentHydration`/`HoursAwake`/`Glucose` + progi `Critical*Threshold`):
1. `bIsInPanic || CurrentHP <= CriticalHPThreshold` → **Flee**
2. `CurrentHydration <= CriticalHydrationThreshold` → **Thirst**
3. `HoursAwake >= CollapseThreshold` (lub Microsleep — do ustalenia) → **Sleep**
4. `Glucose <= CriticalKcalThreshold` → **Hunger**
5. inaczej → **None** (idle/social)
*(Temperaturę pomijam w #1 — brak wpiętej gałęzi; wejdzie z osią temp.)*

**Decyzja do podjęcia (Twoja):**
- (A) **Nowy `GetActionableNeed()`**, `EvaluateCurrentNeed()` zostaje nietknięty dla HUD/innych (zwraca POZIOM). ← rekomendacja.
- (B) Przerobić `EvaluateCurrentNeed` na zwrot konkretu — odradzam (łamie kontrakt „zwraca poziom" + ryzyko dla istniejących konsumentów).
- Reprezentacja: osobny C++ `ENPCNeed` (typobezpiecznie, ale **drift-ryzyko**: bajty muszą trzymać lockstep z BP `E_NeedState`) **vs** zwrot surowego `uint8` z komentarzem-kontraktem. ← do wyboru.

---

## 4. DECYZJA #2 — jeden klucz, jeden pisarz

**Rekomendacja: zostaje `CurrentNeed` (Enum `E_NeedState`); C++ pisze go przez `SetValueAsEnum`.**

Uzasadnienie wobec realnych typów dekoratorów:
- Dekoratory są już autorskie na **Enum** (`CurrentNeed Is Equal To Thirst/Sleep`). Zostają **bez przepinania** — Handle Thirst działa od ręki, a re-attach kolejnych gałęzi (głód/sen) korzysta z czytelnych porównań enumowych.
- Przejście na `MaslowPriority` (Int) wymagałoby: przepiąć KAŻDY dekorator na Int-compare, wyeksponować surowy poziom Maslowa (za grubo — kontrakt #1 chce konkretu), i ręcznie pilnować wartości EMaslowPriority. Gorzej i więcej pracy.

Zmiany w serwisie `BTService_MaslowBlackboardSync` (do zatwierdzenia, kod po „ok"):
- **Usuń** 6 mis-bindowanych zapisów (`SetValueAsInt/Float/Bool` → `CurrentNeed`) — to one zerują klucz.
- **Pisz JEDEN** klucz: `BB->SetValueAsEnum(CurrentNeedKey, (uint8)Maslow->GetActionableNeed())`.
- Klucz `CurrentNeedKey` wpięty w `CurrentNeed` (jeden selektor, poprawny typ).
- Dedykowane klucze Maslow (`HPPercent` itd.) w plastrze #1 **nie ruszane** (nikt nie czyta; HUD-bind to osobny plaster).

🚩 Drift do odnotowania: po tej zmianie istniejące 6 `FBlackboardKeySelector` w nagłówku serwisu staje się martwe — w plastrze #1 zostają (nie kasuję publicznego API bez potrzeby), sprzątanie = późniejszy plaster.

---

## 5. DECYZJA #3 — zamrożenie decyzji BP (odwracalne)

`EvaluateNeeds` (BP_NPC_Character) ma 4× `SetValueAsEnum(KeyName="CurrentNeed")`. **Zamrożenie = odpięcie exec-pinów do tych 4 węzłów** (węzły zostają w grafie, łatwo wpiąć z powrotem). NIE kasujemy `EvaluateNeeds`, NIE kasujemy BP-zmiennej `CurrentNeed`, NIE kasujemy systemu potrzeb BP.
- Po zamrożeniu jedynym pisarzem `CurrentNeed` (BB) jest serwis C++.
- BP zostaje przy wykonaniu: `BTTask_FindWater/MoveTo/Drink/Eat`, animacje, `ApplyActionCost`.
- 🚩 Zweryfikuję przed odpięciem, czy nic w BP nie czyta **BB-klucza** `CurrentNeed` poza dekoratorami (szybki find-references). Jeśli czyta → flaguję przed zamrożeniem.

Kasacja BP-systemu potrzeb (HungerLevel/ThirstLevel/MetabolismStats jako źródła decyzji) = **świadomie późniejszy plaster**, nie teraz.

---

## 6. DECYZJA #4 — PLASTER #1: PRAGNIENIE end-to-end

**W zakresie:** oś Hydration. Handle Thirst (jedyna wpięta gałąź) napędzana przez C++.
**Poza zakresem (kolejne plastry):** głód, sen, panika/flee, re-attach gałęzi Hunger/Zagrożenie/Eat, pusta Handle Sleep, pułapka omdlenia (trwały fix), HUD-bind, kasacja BP need-calc.

Kroki implementacji (PO „ok", w tej kolejności, z weryfikacją):
1. C++: `GetActionableNeed()` (decyzja #1) + ewentualny `ENPCNeed`. Build edytor-zamknięty (UHT).
2. C++ serwis: jeden zapis `SetValueAsEnum` (decyzja #2).
3. BP: zamrożenie 4 zapisów w `EvaluateNeeds` (decyzja #3) — ręcznie w edytorze, zapis manualny (patrz pamięć: brak save przez Monolith dla BP).
4. Wpięcie `CurrentNeedKey` serwisu w `CurrentNeed` w BT (jeśli trzeba).

### Dowód plastra #1 (twarde liczby, wymóg dyrektora)
**Zepsuj C++ `CurrentHydration` (NIE BP `ThirstLevel`).** Protokół PIE:
1. Odczytaj live: `CriticalHydrationThreshold` (próg), `CurrentHydration`, BP `ThirstLevel`, BB `CurrentNeed`.
2. Ustaw `CurrentHydration` < `CriticalHydrationThreshold` (live set_editor_property). **BP `ThirstLevel` zostaw nietknięty.**
3. Po ticku serwisu (~1 s) odczytaj: BB `CurrentNeed` == **Thirst(2)**; `runtime_get_bt_execution_path` = gałąź Handle Thirst (FindWater→MoveTo→Drink); NPC pije.
4. Pokaż kontrast: BP `ThirstLevel` przez cały test bez zmian → **steruje C++ Hydration, nie BP**.
5. Negatyw (opcjonalnie): podnieś `CurrentHydration` z powrotem → `CurrentNeed`→None, NPC przestaje.

### ⚠️ Obejście pułapki omdlenia (żeby test pragnienia nie był skażony)
NPC omdlewają w ~2.5 min (HoursAwake→25 → `StopLogic`, Handle Sleep pusta = brak wyjścia).
Na CZAS testu (scaffolding, odwracalne, NIE część fixa): zneutralizuj oś snu — np.
`MicrosleepThreshold`/`CollapseThreshold` ustaw bardzo wysoko **lub** cyklicznie `HoursAwake=0`,
żeby BT nie padło w trakcie. Po teście przywróć. (Trwały fix pułapki = osobny plaster snu.)

---

## 7. ROADMAPA MOSTU (kolejne plastry — po #1)
- **#2 Głód:** re-attach gałęzi Handle Hunger pod selektor; getter już zwraca Hunger; dowód: zepsuj `Glucose`.
- **#3 Sen + fix pułapki omdlenia:** wypełnij Handle Sleep (akcja snu → `StartSleep`/reset HoursAwake), ścieżka wyjścia z `bIncapacitated`; getter zwraca Sleep; usuń „śmierć w 2.5 min".
- **#4 Panika/Flee:** potwierdź/utwórz wpis `Flee` w `E_NeedState` (wpis 4); re-attach gałęzi „Zagrożenie"; **ożywia L3-02** (panika realnie wchodzi do BT). Dowód: `bIsInPanic`/krytyczne HP → Flee.
- **#5 Sprzątanie:** dedykowane klucze Maslow → poprawne klucze dla HUD **lub** usunięcie martwego API serwisu; kasacja BP need-calc (HungerLevel/ThirstLevel jako decyzja) gdy wszystkie osie na C++.
- **#6 (temat odrębny):** dwa źródła prawdy (BP `MetabolismStats`/`BodyComposition` vs C++) — most danych albo wybór jednego.

---

## 8. KOREKTY DOKUMENTACJI (wymóg dyrektora — wykonam przy „ok", razem z plastrem)
- ROADMAP L3-02 + CODE_REGISTRY „OCEAN ŻYWY W GRZE": **sprostować** → panika **NIE dociera do BT** (żaden wpięty dekorator nie czyta kluczy Maslow; gałąź „Zagrożenie" odłączona). OCEAN/`bIsInPanic` **policzone, bez wpływu na zachowanie DO mostu** (plaster #4 go ożywi). „Żywe w grze" dotyczyło wyłącznie linii logu z `EvaluatePanicRoll`.
- TECH (nowy lub rozszerzenie TECH-08): wpisać konflikt „serwis zeruje `CurrentNeed`" + mis-bind 6 selektorów.

---

## 9. FLAGI DRIFTU
1. `E_NeedState` wpisy 4–7: nazwy niezweryfikowane — potwierdzić przed plastrem #4 (Flee).
2. C++ `ENPCNeed` (jeśli wybrane) ↔ BP `E_NeedState`: bajty MUSZĄ trzymać lockstep (kontrakt danych).
3. Mapowanie nie-castowalne (panika 0↔None 0, głód 4↔1) — getter tłumaczy ręcznie.
4. Po zmianie serwisu 6 `FBlackboardKeySelector` staje się martwe (sprzątanie później).
5. Sprawdzić find-references BB `CurrentNeed` w BP przed zamrożeniem (#3).

## 10. CZEGO ŻĄDA „ok"
Autoryzacja: decyzji #1 (A/B + reprezentacja), #2 (CurrentNeed enum vs MaslowPriority), #3 (zamrożenie), #4 (zakres pragnienia) oraz zgoda na test-scaffolding obejścia omdlenia. Po „ok" → koduję plaster #1, weryfikuję twardo, raportuję, czekam na kolejny gate przed plastrem #2.

**NIE koduję nic przed „ok".**
