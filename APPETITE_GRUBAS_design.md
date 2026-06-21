# APPETITE / GRUBAS — DESIGN GATE (rozpychanie żołądka + odkładanie tłuszczu)
> Zatwierdzony projekt do implementacji (architekt + Szymon, 2026-06-21).
> ROZSZERZA UMaslowBiologicalComponent (NIE nowy komponent, NIE nowy timer, NIE Event Tick).
> Buduje na: Hunger warstwa 1 (EffectiveKcalThreshold/Neuroticism), kaskadzie EHungerPhase,
> MaxBodyFat=5000, AmbientTemp (InsulationFactor hook).
> C++ liczy WSZYSTKO (objętość, EMA, depozyt, izolacja). Blueprint TYLKO rysuje.
>
> 🔴 ZAKRES TEGO ETAPU = SLICE 1 (rdzeń grubasa, celowo BEZ hamulca):
>   dwa progi (trigger vs satiety-stop) + GastricCapacity (dual-driver) + surplus→BodyFat
>   + fat→InsulationFactor (jeśli hook żyje). Leptyna (hamulec) = SLICE 2, leptynooporność
>   (trap) = SLICE 3 — HOOKI, nie teraz. Slice 1 produkuje runaway-grubasa; to ZAMIERZONY,
>   weryfikowalny stan pośredni. Hamulec dodajemy osobno.

═══════════════════════════════════════════════════════════
# DECYZJE PROJEKTOWE (zatwierdzone — NIE zmieniać bez architekta)
═══════════════════════════════════════════════════════════
| # | Decyzja | Wartość |
|---|---|---|
| D-VOLUME | Sygnał sytości | **OBJĘTOŚCIOWY (rozciągnięcie), NIE kaloryczny** — mechanoreceptory mierzą napięcie ściany, nie kcal. To jest nośnik rozpychania. |
| D-TWO-THRESH | Próg głodu vs próg sytości | **DWA OSOBNE progi** — trigger (kiedy zacząć jeść) niezależny od satiety-stop (kiedy przestać). |
| D-CAP-DUAL | GastricCapacity | **DWA DRIVERY** — stretch event-driven (na posiłku, EMA ku rozmiarowi posiłku), shrink time-driven (na ticku, gdy długo nie jadł). **Asymetryczne tempa**: stretch szybki, shrink wolny. |
| D-MACRO | Skąd tłuszcz | **Jedzenie ma makra (Carb/Fat/Protein), routowane różnymi ścieżkami o różnej efektywności depozytu** (Atwater 4/9/4; fat→fat ~1:1, carb→fat droga/nieefektywna, protein→mięśnie/MaxHP). Patrz część C. 🔴 ZALEŻNOŚĆ KRYTYCZNA — recon #1. |
| D-VOLUME-KCAL | Sytość vs energia | **OBJĘTOŚĆ ≠ kalorie.** StomachFill napędzany Volume jedzenia (sytość), makra (w tej objętości) liczą energię/depozyt OSOBNO. Gęste tłuste tuczy przy małej objętości; bujne nisko-kal wypełnia bez tycia. |
| D-EAT | Akcja jedzenia | **Trzymany, przerywalny proces.** BP gra anim, AnimNotify (ugryzienie) → C++ ConsumeBite. Przerwanie → StopEating: głód zaspokojony proporcjonalnie, jedzenie NADGRYZIONE (remaining trwa). Rozmiar sesji → stretch-EMA. |
| D-INSULATION | Most fat→temp | `BodyFat` obniża `InsulationFactor` (grubas wolniej stygnie / przegrzewa się). Wpinamy w slice 1 JEŚLI hook AmbientTemp żyje. |
| D-FLAG2 | Modulacja Neuroticism | **Asymetryczny Lerp(StableKcalThr, NervousKcalThr, N) = TraitFloor** (podłoga cechy). Stretch dokłada od góry, leptyna (slice 2) odejmie. FLAG 2 zalockowane jako baseline. |
| D-STAGE | Realizacja | **Slice 1 = runaway (bez hamulca)**. Leptyna=slice 2, oporność=slice 3. |

═══════════════════════════════════════════════════════════
# 🔴 RECON-STOP — POTWIERDŹ 3 DZIURY PRZED KODEM (wektor #1 projektu: martwe/brakujące systemy)
═══════════════════════════════════════════════════════════
ZANIM kodujesz cokolwiek — przeczytaj żywy kod i potwierdź. Jeśli któraś dziura istnieje,
mechanika nie ma na czym stanąć. To dokładnie wzorzec cold-burn / Maslow→BT — nie zgaduj, FLAGUJ.

1. **DEPOZYT TŁUSZCZU + MAKRA — co istnieje?**
   Kaskada EHungerPhase w docu to strona SPALANIA (Glucose→Glycogen→FatBurn→Autophagy).
   Pytania:
   - Czy `FoodItemRow` / item jedzenia ma już makra (Carb/Fat/Protein) czy tylko jeden „kcal/food value"?
     Jeśli nie ma makr → dodajemy pola Volume/CarbG/FatG/ProteinG (część C).
   - Gdy NPC je i Glucose dobija do MaxGlucose (1000) — nadwyżka przelewa w Glycogen→BodyFat,
     czy jest CAPowana i WYRZUCANA? Jeśli wyrzucana → depozyt NIE istnieje, dobudować (część C, routing makr).
   - Czy istnieje gdziekolwiek przyrost BodyFat (lustro FatBurn), czy tylko spalanie?
   ODCZYTAJ: akcja jedzenia (co inkrementuje), ProcessMetabolism (cap/overflow), definicja item jedzenia.

2. **AKCJA JEDZENIA — jak działa dziś?** (decyzja designu: ma być TRZYMANYM procesem, część A)
   - Czy jedzenie to dziś instant (jeden call doładowuje Glucose) czy proces?
   - Czy istnieje hook BP/anim do „ugryzienia" (AnimNotify), do którego doczepimy ConsumeBite?
   - Czy item jedzenia ma „remaining portion" (do mechaniki nadgryzienia)? Jeśli nie → dodać.
   Cel: zamienić jedzenie w StartEating/ConsumeBite(per AnimNotify)/StopEating (część A).

3. **InsulationFactor — żywy w AmbientTemp czy martwy hook?**
   AMBIENT_TEMP_design.md deklaruje `InsulationFactor` (1.0=brak izolacji) czytany w sprzężeniu
   otoczenie→ciało (EffCoolingRate). POTWIERDŹ że jest realnie czytany w kodzie (nie kolejny
   martwy system). Jeśli żyje → most fat→izolacja to jeden mnożnik. Jeśli martwy → wpięcie do
   slice 1, ale FLAGUJ że ożywiamy hook (jak przy cold-burn).

═══════════════════════════════════════════════════════════
# CZĘŚĆ A — JEDZENIE JAKO TRZYMANY PROCES + DWA PROGI
═══════════════════════════════════════════════════════════
Dziś jest JEDEN próg (trigger). Dokładamy: jedzenie = trzymany przerywalny proces, plus drugi
próg (satiety-stop). C++=mózg liczy skutek, BP=ciało rządzi rytmem gryzień (AnimNotify).

## Pętla posiłku (sesja):
```
1. EffectiveKcalThreshold (trigger) odpala → BT/BP wchodzi w jedzenie  [istnieje]
2. BP: StartEating(Food) → gra anim. Każde ugryzienie (AnimNotify) → C++ ConsumeBite()
3. ConsumeBite: kęs (Volume+makra proporcjonalnie z Food) →
     StomachFill += biteVolume; routuj makra (część C); dekrementuj Food.RemainingPortion
4. STOP automatyczny gdy: StomachFill >= SatietySetpoint (syty)  LUB  Food wyczerpane (zjedzone)
5. STOP wymuszony (puść guzik / panika L0 / wróg / collapse) → StopEating(Reason)
6. zamknięcie posiłku → rozmiar sesji (suma kęsów) → stretch-EMA (część B)
   Food.RemainingPortion > 0 → jedzenie NADGRYZIONE (trwa, nie znika)
7. StomachFill drenuje przez trawienie (tick) → kcal/makra → Glucose/Glycogen/BodyFat (część C)
```
🔑 Głód zaspokojony PROPORCJONALNIE automatycznie — każdy kęs już dolał Glucose. Przerwanie =
po prostu mniej dolane, zero osobnej logiki „połowiczna sytość".

## Nowe pola:
```cpp
/** Objętość pokarmu w żołądku TERAZ (transient, drenuje przez trawienie). NAPĘDZANA VOLUME, nie kcal. */
UPROPERTY(BlueprintReadOnly, Category="Biology|Appetite")
float StomachFill = 0.0f;

/** Czy NPC aktualnie je (sesja otwarta). */
UPROPERTY(BlueprintReadOnly, Category="Biology|Appetite")
bool bIsEating = false;

/** Jedzenie tej sesji — weak ptr, bo inny NPC może je zabrać/zniszczyć (P2P). */
TWeakObjectPtr<AActor> EatTargetFood;   // typ do potwierdzenia (item jedzenia)

/** Rozmiar bieżącej sesji (suma kęsów) — domyka stretch-EMA przy StopEating. */
float CurrentMealSize = 0.0f;

/** Próg sytości = ile objętości potrzeba, by przestać jeść. */
// SKORYGOWANE 2026-06-21 (Szymon zatwierdził): = GastricCapacity × SatietyOverfillFactor (1.15).
// Bez overfill (>1) MealSize nigdy nie przekracza GastricCapacity → stretch-EMA (część B) matematycznie
// ZAMROŻONY. Overfill = NPC je lekko PONAD pojemność → rozpycha żołądek. SatietyOverfillFactor = tunable.
UFUNCTION(BlueprintPure, Category="Biology|Appetite")
float GetSatietySetpoint() const { return GastricCapacity * SatietyOverfillFactor; }
```

## API (BP woła, C++ liczy):
```cpp
UFUNCTION(BlueprintCallable, Category="Biology|Appetite")
void StartEating(AActor* Food);     // otwiera sesję, cache'uje Food (IsValid guard)

UFUNCTION(BlueprintCallable, Category="Biology|Appetite")
void ConsumeBite();                 // 1 ugryzienie — wołane z AnimNotify. Early-return gdy
                                    // !bIsEating || bIncapacitated || !EatTargetFood.IsValid()

UFUNCTION(BlueprintCallable, Category="Biology|Appetite")
void StopEating(EEatStopReason Reason);   // Full/Finished/Interrupted/SourceGone/Incapacitated
                                          // → CurrentMealSize → stretch-EMA; Food nadgryzione zostaje
```

🔴 C++ NIGDY nie dotyka animacji. AnimNotify = naturalny zegar gryzień → zero nowego timera.

## 🛡️ EDGE-CASES (fail-safe, 500 NPC):
- **EC-EAT-1 śmierć w trakcie:** EndPlay → bIsEating=false, EatTargetFood=null. Nadgryzione jedzenie
  NIE niszczone (zostaje w świecie/inwentarzu).
- **EC-EAT-2 jedzenie zabrane/zniszczone (P2P):** EatTargetFood stale → ConsumeBite IsValid-guard →
  StopEating(SourceGone). Zero crasha.
- **EC-EAT-3 omdlenie/mikrosen w trakcie:** ConsumeBite early-return przy bIncapacitated;
  EnterCollapse() (z ETAP 2) woła StopEating(Incapacitated).
- **EC-EAT-4 panika L0 w trakcie:** L0 i tak przerywa kolejkę → StopEating(Interrupted);
  CurrentMealSize (małe) i tak karmi stretch-EMA (przerwany posiłek = mały stretch, OK).

## Koszt:
- Pamięć: bool + weak ptr + 2 float ≈ 24 B/NPC × 500 ≈ 12 KB. ConsumeBite event-driven (AnimNotify),
  NIE per-tick → zero kosztu, gdy NPC nie je; kilku je naraz. Routing makr = kilka dodawań/kęs.

🔑 Direction (fizjologia): rozpchany żołądek = wyższy SatietySetpoint = więcej objętości na
posiłek = przy okazji więcej kcal (zależnie od makr) → tłuszcz. To JEST nośnik przejadania.

═══════════════════════════════════════════════════════════
# CZĘŚĆ B — GASTRIC CAPACITY: DWA DRIVERY (stretch / shrink)
═══════════════════════════════════════════════════════════
Jeden float, ale popychany z DWÓCH stron, w dwóch różnych momentach:

```cpp
/** Pojemność żołądka (stan, dryfuje). Stretchowana posiłkami, kurczona głodówką. */
UPROPERTY(BlueprintReadOnly, Category="Biology|Appetite")
float GastricCapacity = 0.0f;   // init = BaseGastricCapacity w BeginPlay
```

## Driver 1 — STRETCH (event-driven, na zamknięciu posiłku):
```cpp
// Wołane RAZ przy końcu posiłku (krok 4 części A), NIE co tick.
// EMA: pojemność podpełza ku rozmiarowi tego posiłku, ale tylko W GÓRĘ (rozciąganie).
const float MealSize = StomachFillAtMealEnd;
if (MealSize > GastricCapacity)
{
    GastricCapacity = FMath::Lerp(GastricCapacity, MealSize, StretchRate);   // szybki
}
GastricCapacity = FMath::Min(GastricCapacity, MaxGastricCapacity);
```

## Driver 2 — SHRINK (time-driven, na ticku metabolizmu, gdy NPC NIE je / głoduje):
```cpp
// Na istniejącym ticku. Gdy NPC w stanie głodu (np. EHungerPhase >= FatBurn LUB HoursSinceLastMeal
// > próg) → pojemność powoli wraca ku podłodze. Asymetria: ShrinkRate << StretchRate.
if (bIsFasting)   // warunek do potwierdzenia z istniejącym stanem głodu
{
    GastricCapacity = FMath::Lerp(GastricCapacity, BaseGastricCapacity, ShrinkRate);   // wolny
}
```

🔑 Bez Driver 2 EMA zamarza przy pustym żołądku (posiłków brak → stretch nie odpala → pojemność
stoi). Driver 2 daje realne „głodówka kurczy żołądek" + refeeding-realizm (po głodówce NPC nie
wciśnie dużego posiłku — SatietySetpoint spadł).

## Skala czasu (jeden zegar — jak sen):
StretchRate/ShrinkRate wyrażone per GAME-DAY, konwertowane przez TimeSpeed (BP_DayNightCycle),
tak jak AwakeRatePerTick. Spowolnienie zegara NIE rusza tej mechaniki.
- stretch: widoczny po ~kilku game-dniach przejadania.
- shrink: ~tydzień game-głodówki do BaseGastricCapacity.

═══════════════════════════════════════════════════════════
# CZĘŚĆ C — MAKRA → MAGAZYN (depozyt tłuszczu przez routing makr)
═══════════════════════════════════════════════════════════
Jedzenie ma makra. Każde makro routowane INNĄ ścieżką o INNEJ efektywności depozytu.
Grounding: Atwater (carb 4 / protein 4 / fat 9 kcal/g); fat→adipose ~1:1, carb/protein→fat
metabolicznie drogie (DNL / wiele kroków + koszt termiczny białka).

## FoodItem — nowa struktura makr (DataTable row, data-driven):
```cpp
USTRUCT(BlueprintType)
struct FFoodMacros
{
    GENERATED_BODY()
    /** Objętość/masa — napędza StomachFill (sytość). NIEZALEŻNE od kcal. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Volume = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float CarbG = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FatG = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ProteinG = 0.0f;
    // kcal NIE storowane — liczone: Carb*4 + Protein*4 + Fat*9 (Atwater).
};
```
🔑 OBJĘTOŚĆ ≠ KALORIE (D-VOLUME-KCAL): StomachFill rośnie o Volume kęsa (sytość), ale energia/
depozyt liczone z makr. Gęste tłuste (mało Volume, dużo FatG) tuczy przy małym żołądku; bujne
nisko-kal (dużo Volume, mało makr) wypełnia bez tycia. Dieta = wybór strategiczny.

## Routing (na trawieniu kęsa / na ticku — lustro kaskady spalania):
| Makro | Ścieżka | Efektywność → BodyFat | Efekt uboczny |
|---|---|---|---|
| **Carb** | → Glucose (cap 1000) → Glycogen (cap 1000) → BodyFat | NISKA (`CarbToFatEfficiency` ~0.75) | szybka energia; najpierw pali węgle zamiast tłuszczu |
| **Fat** | → bezpośrednio BodyFat (cap 5000) | **WYSOKA** (`FatToFatEfficiency` ~0.95) | makro grubasa, magazyn ~1:1 |
| **Protein** | → odbudowa mięśni/**MaxHP** (rewers autofagii), gluconeogeneza; surplus → BodyFat | NAJNIŻSZA (`ProteinToFatEfficiency` ~0.5) | buduje ciało; domyka hook „wychudzony" |

```cpp
// Szkic routingu (na trawieniu — wartości/ścieżki do potwierdzenia recon #1):
Glucose  += Bite.CarbG * CarbToGlucose;              // cap MaxGlucose → overflow Glycogen
Glycogen += overflowGlucose;                          // cap MaxGlycogen → overflow:
BodyFat  += overflowGlycogen * CarbToFatEfficiency;   // węgle tuczą SŁABO
BodyFat  += Bite.FatG * FatToFatEfficiency;           // tłuszcz tuczy MOCNO (~1:1)
RepairMuscle(Bite.ProteinG);                          // białko → MaxHP/mięśnie (rewers autofagii)
BodyFat  += proteinSurplus * ProteinToFatEfficiency;  // reszta białka tuczy NAJSŁABIEJ
BodyFat = FMath::Min(BodyFat, MaxBodyFat);
```

🔴 RECON #1 decyduje, ile z tego dobudować: jeśli item jedzenia nie ma makr → dodać FFoodMacros;
jeśli przyrost BodyFat nie istnieje → dobudować (to rdzeń grubasa). RepairMuscle: sprawdź, czy
autofagia ma odwracalny licznik MaxHP do odbudowy, czy trzeba dodać.

## Emergencja z routingu (za darmo):
- mięso (Fat+Protein) → tuczy + buduje mięśnie → top survival, ale robi grubasa.
- jagody/korzenie (Carb) → szybko gaszą głód, słabo tuczą → dieta roślinna = chudszy NPC.
- białko → jedyne źródło odbudowy MaxHP po autofagii (dotąd nie było czym).

═══════════════════════════════════════════════════════════
# CZĘŚĆ D — MOST: FAT → INSULATION (AmbientTemp) — najtańsza emergencja
═══════════════════════════════════════════════════════════
Fizjologia: tkanka tłuszczowa izoluje termicznie. Po recon #3 (hook żyje):
```cpp
// BodyFat 0..MaxBodyFat → InsulationFactor 1.0..MinInsulationFromFat (np. 0.6)
const float FatRatio = FMath::Clamp(BodyFat / MaxBodyFat, 0.0f, 1.0f);
InsulationFactor = FMath::Lerp(1.0f, MinInsulationFromFat, FatRatio);
```
Efekt emergentny z jednego floata:
- grubas wolniej stygnie w Ocean/Mountain (przeżywa zimno),
- przegrzewa się w AshSlope/Lava (sens dla odłożonej hipertermii),
- większy bufor BodyFat na głód/zimno,
- (mobilność↓ / gorsza ucieczka L0 — HOOK, gdy ruszymy speed/panikę).
⚠️ InsulationFactor ma JEDNEGO writera. Jeśli ekwipunek też go pisze (przyszłe ubranie) →
ustalić kompozycję (mnożenie czynników), nie nadpisywanie. Na teraz: tylko fat.

═══════════════════════════════════════════════════════════
# CZĘŚĆ E — SPIĘCIE Z FLAG 2 (kompozycja triggera)
═══════════════════════════════════════════════════════════
EffectiveKcalThreshold przestaje być stałą cechy, staje się kompozycją:
```
EffectiveKcalThreshold = TraitFloor(Neuroticism)          // FLAG 2, istnieje (Lerp Stable/Nervous)
                       + StretchBonus(GastricCapacity)    // NOWE: rozpchany = czuje pusto wcześniej
                       - LeptinBrake(BodyFat)             // SLICE 2 HOOK (=0 w slice 1)
```
- StretchBonus: rozpchany żołądek → wyższy trigger → je wcześniej (dodatnie sprzężenie).
- LeptinBrake = 0 w slice 1 (runaway). Slice 2 włącza ujemne sprzężenie.
- Liczone OBOK sędziego (na kadencji, jak EffectiveKcalThreshold dziś) — sędzia czyta gołego floata.

═══════════════════════════════════════════════════════════
# PODZIAŁ C++ / BLUEPRINT
═══════════════════════════════════════════════════════════
## C++ (UMaslowBiologicalComponent — ROZSZERZAMY):
- FFoodMacros (Volume/Carb/Fat/Protein) na item jedzenia + kcal liczone (Atwater 4/4/9).
- StartEating/ConsumeBite/StopEating (proces jedzenia, część A) + StomachFill + drenaż + satiety-stop.
- Routing makr → Glucose/Glycogen/BodyFat (różne efektywności) + RepairMuscle(Protein) (część C).
- GastricCapacity (dual-driver: stretch event / shrink tick).
- fat→InsulationFactor (część D).
- StretchBonus w kompozycji triggera (część E).
- Gettery: GetSatietySetpoint(), GetGastricCapacity(), GetBodyFatRatio().
- UE_LOG na: każdy kęs (opcj.), zamknięcie posiłku (rozmiar, nadgryzione, nowa pojemność),
  przelew w tłuszcz per makro, shrink-tick przy głodówce.

## Blueprint (BlueprintImplementableEvent / BlueprintCallable):
- BP woła StartEating/StopEating; AnimNotify ugryzienia → ConsumeBite (BP=rytm, C++=skutek).
- OnMealEnd(float MealSize, EEatStopReason) — beknięcie / animacja sytości / odłożenie nadgryzionego.
- OnBodyFatTierChanged(int Tier) — zmiana sylwetki (mesh morph) na progach BodyFat.
(Mechanika sama wystarcza do verify. Eventy wizualne = nice-to-have.)

═══════════════════════════════════════════════════════════
# PERF (500 NPC — religia)
═══════════════════════════════════════════════════════════
- Pamięć: GastricCapacity (persistent) + StomachFill + CurrentMealSize (transient) + bIsEating +
  EatTargetFood (weak ptr) ≈ 24 B/NPC × 500 ≈ 12 KB. Setpoint/Leptin/Insulation w getterach (zero storage).
- CPU: ConsumeBite EVENT-DRIVEN (AnimNotify), nie per-tick → zero kosztu gdy NPC nie je; routing makr
  = kilka dodawań/kęs. Drenaż StomachFill = 1 odjęcie + transfer na istniejącym ticku.
  Shrink-tick = 1 Lerp warunkowo (bIsFasting). Stretch = 1 Lerp RAZ na posiłek. Reużycie CachedIdentity.
- Brak nowego timera, brak Event Tick, brak per-tick zone/component lookupu.

═══════════════════════════════════════════════════════════
# WARTOŚCI DO MECHANICS (po implementacji — [TBD→tune])
═══════════════════════════════════════════════════════════
| Zmienna | Propozycja | Uzasadnienie |
|---|---|---|
| Atwater kcal/g (Carb/Protein/Fat) | 4 / 4 / 9 | standard, [LOCKED] |
| FatToFatEfficiency | ~0.95 | tłuszcz w diecie → adipocyt ~1:1 |
| CarbToFatEfficiency | ~0.75 | DNL droga (wiele kroków) |
| ProteinToFatEfficiency | ~0.5 | + koszt termiczny, ostatnia w kolejce |
| BaseGastricCapacity | = obecny rozmiar posiłku (z recon #2) | punkt zero |
| MaxGastricCapacity | ~2.5× Base | skrajni przejadacze realnie 2–4× |
| StretchRate (per game-day) | szybki | łatwo rozciągnąć |
| ShrinkRate (per game-day) | << StretchRate | wolno zwinąć (tygodnie) |
| SurplusToFatEfficiency | ~0.8 | magazyn tłuszczu tani |
| MinInsulationFromFat | ~0.6 | maks otyłość = mocna izolacja |
| StretchBonus skala | dostroić | by demo separowało |
| LeptinBrake | 0 (slice 1) | [LOCKED, hook slice 2] |

═══════════════════════════════════════════════════════════
# STAGING
═══════════════════════════════════════════════════════════
- SLICE 1 (TEN ETAP): proces jedzenia (Start/Bite/Stop) + FFoodMacros + routing makr→BodyFat +
  dwa progi + GastricCapacity dual-driver + fat→insulation. Runaway grubas (brak hamulca) — ZAMIERZONY.
- SLICE 2 (HOOK): LeptinBrake(BodyFat) → ujemne sprzężenie → stabilizacja.
- SLICE 3 (HOOK): leptynooporność (przewlekły fat gasi hamulec) → trap / dołek potencjału.

═══════════════════════════════════════════════════════════
# WERYFIKACJA (Claude Code — twarde liczby, zero placeholderów)
═══════════════════════════════════════════════════════════
Po buildzie (edytor zamknięty → UHT), PIE + log z dysku + Monolith live read:

BLOK 0 — proces jedzenia (przerywalny):
  StartEating → kilka ConsumeBite → StomachFill rośnie kęsami, Food.RemainingPortion maleje.
  StopEating(Interrupted) w połowie → bIsEating=false, Food.RemainingPortion>0 (nadgryzione, istnieje),
  Glucose dolane proporcjonalnie do zjedzonych kęsów. EC-EAT-2: zniszcz Food w trakcie → StopEating
  (SourceGone), zero crasha/warninga.

BLOK 1 — dwa progi działają:
  StartEating bez przerwania → ConsumeBite aż StomachFill==SatietySetpoint → auto StopEating(Full).
  Log: CurrentMealSize == SatietySetpoint (exact). NPC nie je w nieskończoność.

BLOK 2 — stretch (dodatnie sprzężenie):
  Karm NPC w kółko N pełnych posiłków → GastricCapacity rośnie monotonicznie ku MaxGastricCapacity,
  SatietySetpoint rośnie, kolejne posiłki większe. Pokaż trend (liczby per posiłek).

BLOK 3 — shrink (głodówka kurczy):
  Zagłodź NPC (brak posiłków, bIsFasting) przez M ticków → GastricCapacity spada ku
  BaseGastricCapacity. Asymetria: M ticków shrink >> N posiłków stretch dla tej samej delty.

BLOK 4 — makra → depozyt (różne efektywności):
  Nakarm jedzeniem czysto TŁUSZCZOWYM vs czysto WĘGLOWYM o tych samych kcal → BodyFat rośnie
  WYRAŹNIE szybciej od tłuszczu (×0.95 vs ×0.75). Białko → MaxHP/mięśnie rośnie, BodyFat ledwo.
  Live: trend BodyFat per typ makra. Potwierdź że overflow NIE jest wyrzucany (recon #1 zalepione).
  Objętość≠kcal: gęste tłuste (mały Volume) syci mało, ale tuczy; bujne nisko-kal syci, nie tuczy.

BLOK 5 — most insulation:
  NPC z wysokim BodyFat w Ocean (8°C) vs chudy → InsulationFactor niższy u grubasa →
  CurrentTemp stygnie WOLNIEJ (porównaj timeline temp obu). Potwierdza, że hook AmbientTemp żyje.

WSZYSTKO twarde liczby z logu/live. Format logu jak w kodzie.

═══════════════════════════════════════════════════════════
# CO TEN ETAP ROBI / CZEGO NIE
═══════════════════════════════════════════════════════════
✅ ROBI: satiety-stop (StomachFill+SatietySetpoint), GastricCapacity dual-driver (stretch/shrink),
   surplus→BodyFat, fat→InsulationFactor, StretchBonus w triggerze, gettery, UE_LOG, verify.
❌ NIE ROBI (HOOKI):
- LeptinBrake (ujemne sprzężenie) → SLICE 2 (=0 teraz, runaway zamierzony).
- Leptynooporność (trap) → SLICE 3.
- mobilność↓ / ucieczka L0 od tłuszczu → HOOK, gdy ruszymy speed/panikę.
- mesh morph sylwetki → BP, OnBodyFatTierChanged tylko wystawia.
- hipertermia w Lava/Caldera → symetryczny mechanizm, osobno (AmbientTemp odłożył).

═══════════════════════════════════════════════════════════
# 🔑 INSTRUKCJA DLA CLAUDE CODE — POTWIERDŹ PRZED KODOWANIEM
═══════════════════════════════════════════════════════════
Ten gate pisany BEZ dostępu do żywego kodu. ZANIM kodujesz:
1. Wykonaj RECON-STOP (3 dziury wyżej) — surplus→fat deposition, sygnał końca posiłku,
   InsulationFactor żywy. Odczytaj realne: akcja jedzenia, ProcessMetabolism (cap/overflow),
   EHungerPhase, sprzężenie temp w AmbientTemp.
2. Jeśli depozyt tłuszczu (#1) NIE istnieje — to PIERWSZY priorytet, zbuduj go, bo bez niego
   reszta wisi. FLAGUJ i zaproponuj ścieżkę przelewu, zatwierdzę.
3. Jeśli cokolwiek się nie zgadza z tym docem — FLAGUJ przed kodem (jak cold-burn), nie zgaduj.
4. Patch, nie regeneracja. Reużyj CachedIdentity, istniejący tick, istniejące cap'y.
5. STOP po slice 1 (runaway zweryfikowany) — leptyna (slice 2) na osobne „jedź".

═══════════════════════════════════════════════════════════
# REGUŁA PATCHOWANIA
═══════════════════════════════════════════════════════════
Rozszerzenie ISTNIEJĄCEGO MaslowBiologicalComponent.{h,cpp}. NIE regeneruj plików — dodaj pola/
funkcje, wepnij wywołania w istniejący ProcessMetabolism (drenaż, shrink-tick, kompozycja triggera)
i w akcję jedzenia (StomachFill, satiety-stop, zamknięcie posiłku → stretch EMA).
