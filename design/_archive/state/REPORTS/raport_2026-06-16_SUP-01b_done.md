# Raport — SUP-01b DONE + cleanup (2026-06-16)

## Zakres
Domknięcie zakładki Body (SUP-01b), test wizualny na NPC, dedup komponentu,
aktualizacja routingu MCP. Wszystko przez **Monolith** (autorowanie węzłów);
remiphilippe użyty tylko do odczytu/weryfikacji.

## Co zrobiono

### 1. Gap A — WBP_Tab_Body / PopulatePartRow
- Dodano węzeł `GetPartDisplayName` (BodyConditionComponent, BlueprintPure, EBodyPart→FText).
- `Part` ← `CastByteToEnum_0.ReturnValue` (ta sama EBodyPart wiersza co health/injury;
  parametr funkcji `Part` jest nieużywany — żywą wartością jest zmienna składowa).
- `ReturnValue`(Text) → tytuł `SetText.InText`; usunięto stub getter `Get PartName`.
- Compile czysty, read-back potwierdzony, zapisane.

### 2. Gap B — WBP_Tab_Body / RefreshBodyPanel
- Wstawiono 7× `SetVisibility(Collapsed)` na PartRow_0..6 między `IsValid(valid)` a `Switch`.
- Łańcuch: `IsValid.Is Valid → CF_25 → … → CF_31 → SwitchOnE_BodyPart.execute`
  (stara krawędź IsValid→Switch zerwana). Każdy SetVis: self←Get PartRow_N (Border→Widget),
  InVisibility=Collapsed (default).
- Efekt: każdy refresh najpierw zwija wszystkie wiersze; Switch pokazuje tylko potrzebne →
  naprawia "wiszące" wiersze przy przełączaniu region 7-części → 1-część.
- Compile czysty, read-back: pełny łańcuch, 0 dyndających pinów, zapisane.

### 3. TEST — BP_NPC_Character / BeginPlay
- BeginPlay miał już 3 testowe ApplyDamage; zamiast dokładać 4. — przekierowano jeden:
  `CastByteToEnum_2.Byte` 15(RightArm)→**8(LeftArm)** → `ApplyDamage(LeftArm, 0.6, Fracture)`.
- Sąsiednie zostawione: AD1 LeftEye/Wound/0.7, AD3 RightLeg/Sprain/0.5.
- Dodano czerwony komentarz **"TEST ONLY - remove later"** obejmujący blok.
- Cel: zobaczyć kaskadę ramię→dłoń→palce na jednym regionie w HUD. HUD czyta
  `NPC.BodyConditionComp` (przez SetNPCRef) = ten sam komponent, który dostaje damage.
- Compile czysty, zapisane. Wartości: EBodyPart LeftArm=8, EInjuryType Fracture=2.

### 4. Dedup komponentu — BP_NPC_Character
- Były DWA BodyConditionComponent: `BodyCondition` + `BodyConditionComp`.
- Pełny skan 13 grafów / 264 węzłów + search_nodes → `BodyCondition` nieużywany NIGDZIE.
- Usunięto `BodyCondition` (został `BodyConditionComp`); compile czysty; 5→4 komponenty; zapisane.

### 5. CLAUDE.md — routing MCP
- Monolith = autorowanie węzłów (potwierdzone działa). remiphilippe = build/test/C++/odczyt
  (NIE potrafi autorować — tylko pusty shell). Usunięto nieaktualne notatki.

## Wykryte luki Monolith / remiphilippe
- **remiphilippe `blueprint_modify.add_node`**: tworzy tylko pusty, niezbindowany K2Node
  (brak pola na funkcję/piny) — nie nadaje się do autorowania. → używać Monolith.
- **Monolith `blueprint.save_asset`/`save_dirty_assets`**: zawodne na WBP_Tab_Body
  (błąd mimo dirty + plik nie-RO). Działa `editor.save_packages`.
- **Monolith `connect_pins` auto-konwersja**: przy enum-byte(TEnumAsByte)→scoped-EBodyPart
  wstawia węzeł Byte→Enum, spina go z celem, ale ZOSTAWIA wejście castu dyndające
  (cichy błąd, kompiluje się jako default). Obejście: źródłuj z pinu już typu EBodyPart.
- **Monolith `find_variable_references`**: zwraca 0 dla zmiennych-komponentów (nawet
  faktycznie używanych) — nie polegać; używać odczytu grafów / search_nodes.

## Pliki
- Content/DocelowaGra/UI/Inventory/WBP_Tab_Body.uasset (PopulatePartRow, RefreshBodyPanel)
- Content/DocelowaGra/AI_NPC/BP_NPC_Character.uasset (EventGraph, komponenty)
- CLAUDE.md, ROADMAP.md, CHANGELOG.md

## Build
NIE wykonano — wszystkie zmiany są BP-only (wywołania istniejących funkcji C++),
każdy BP skompilowany czysto w edytorze (0 err / 0 warn). Build C++ niepotrzebny.

## Status
SUP-01b → ✅ DONE.

## Co dalej
- Uruchomić PIE i otworzyć inspector na NPC → zweryfikować wizualnie: tytuły części
  (Gap A), zwijanie wierszy przy zmianie regionu (Gap B), kaskada LeftArm/Fracture (TEST).
- Po weryfikacji: USUNĄĆ blok TEST ApplyDamage z BeginPlay (oznaczony komentarzem).
- SUP-01a (złożenie zakładek w WidgetSwitcher), SUP-01c (klikalne regiony sylwetki).
- Dług: pusta funkcja-stub `GetPartName` w WBP_Tab_Body (osierocona po Gap A) — do usunięcia.
