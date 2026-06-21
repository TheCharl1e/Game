# DODATEK DO CLAUDE.md — Pamięć i Biblioteka Skryptów
# Wklej tę treść do E:\Game\CLAUDE.md (sekcja na końcu pliku).
# Cel: Claude Code zna styl pracy (mniej pytań) i reużywa skryptów (mniej tokenów).

## 📖 SESSION START — czytaj zawsze, w tej kolejności
1. CLAUDE.md (te zasady)
2. Gra_Stan_Pierwotny/PROJECT_STATE.md   — co istnieje, statusy komponentów
3. Gra_Stan_Pierwotny/ROADMAP.md         — zadania, kolejność, zależności
4. Gra_Stan_Pierwotny/MECHANICS.md       — wartości zmiennych (ground-truth do testów)
5. Gra_Stan_Pierwotny/DECISIONS.md       — zatwierdzone decyzje mechaniczne
6. Gra_Stan_Pierwotny/CHANGELOG.md (ostatnie 20 wpisów) — co zmienione niedawno

Nie pytaj o rzeczy, które są w tych plikach. Jeśli odpowiedź tam jest — użyj jej.
Jeśli pliku brakuje — utwórz go pusty z nagłówkiem i kontynuuj.

## 🧠 STYL PRACY (żeby mniej pytać)
Zasady, których trzymam się bez pytania za każdym razem:
- Kod/zmienne/komentarze po angielsku, rozmowa po polsku.
- C++ = mózg (matematyka/pamięć/logika). Blueprint = ciało (wizualizacja).
- Zero Event Tick. Timery + BT + EQS. Cel: 500+ NPC.
- Czysta separacja .h/.cpp, poprawne UPROPERTY/UFUNCTION/UENUM z kategoriami BP.
- Data-driven: UDataAsset/UDataTable zamiast hardkodu.
- Fail-safes: IsValid() na wskaźnikach, edge cases.
- Hojny UE_LOG z kategoriami przy decyzjach algorytmu.
- Patche, nie regeneracja całych plików (oszczędność tokenów).
- git commit przed każdą serią zmian.

## ❓ KIEDY PYTAĆ, A KIEDY NIE
NIE pytaj (rób autonomicznie):
- flip statusów w ROADMAP, aktualizacja CHANGELOG/MECHANICS
- dług techniczny (redirectory, literówki BB, cleanup)
- kompilacje, weryfikacje, dokończenia już zaprojektowanych rzeczy
- reużycie/modyfikacja skryptu z biblioteki

ZATRZYMAJ SIĘ i pokaż plan (decyzja mechaniczna):
- nowy enum / USTRUCT wpływający na rozgrywkę
- L3-01 NPCRegistry, OCEAN→BT, kontrakty P2P, reputacja, detektyw
- zmiana wartości [LOCKED] w MECHANICS.md
- cokolwiek, co inny instancja Claude'a mogłaby uznać za zmianę charakteru projektu

## 📦 BIBLIOTEKA SKRYPTÓW — polityka (oszczędność tokenów)
Lokalizacja: Gra_Stan_Pierwotny/Scripts/

PRZED napisaniem nowego skryptu Python/bash:
1. Przejrzyj Scripts/ — czy jest coś, co robi to lub podobne?
2. Decyzja:
   - Jest i pasuje → użyj bez zmian (NIE przepisuj).
   - Jest, ale parametry inne → zmodyfikuj i zapisz wariant.
   - Brak → napisz nowy, zapisz do Scripts/ PO udanym użyciu.

Nazewnictwo:
   ue_[akcja]_[cel].py        np. ue_create_widget_tab.py
   ue_edit_[co].py            np. ue_edit_blueprint_var.py
   patch_[komponent]_[co].py  np. patch_maslow_getters.py
   build_[co].py / test_[co].py

Każdy skrypt: komentarz na górze = co robi + jak użyć + data.
Po udanym użyciu nowego skryptu → zapisz do Scripts/ + wpis do CHANGELOG.

## 📝 CHANGELOG — loguj każdą zmianę (tanie, więc hojnie)
Plik: Gra_Stan_Pierwotny/CHANGELOG.md
Format wpisu:
   [YYYY-MM-DD] [komponent] krótki opis
     zmienne: NazwaZmiennej = wartość (jeśli dotyczy)
     plik: ścieżka
     commit: hash (jeśli był)
     skrypt: nazwa z Scripts/ (jeśli użyto/utworzono)

Aktualizuj MECHANICS.md gdy zmienia się wartość zmiennej gry.
Aktualizuj PROJECT_STATE.md gdy zmienia się status/API komponentu.

## 🔌 MCP ROUTING
- remiphilippe → build/test/C++/czytanie grafów/tworzenie assetów
- monolith (po instalacji) → autorowanie węzłów Blueprint (add_node, wire)
Reguła: spawn/wiring węzłów grafu → monolith. Reszta → remiphilippe.
