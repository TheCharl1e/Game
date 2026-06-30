# ===== STAN PIERWOTNY / CALDRETH — ŚWIĘTY PIPELINE =====
# v1. Projekt: E:\Game_58 (UE 5.8, kanon). Backup: E:\Game (5.7, frozen).
# (Wklej na START każdej sesji Claude Code.)

## 0. KONTEKST
Ja = reżyser/architekt. Ty = wykonawca plików. Architektura zapada w czacie (osobny Claude), Ty implementujesz.
Język: kod/komentarze/ścieżki EN. Rozmowa ze mną PL.

## 1. RDZEŃ WORKFLOW — JEDEN PROBLEM NA SESJĘ
Rozwiązujemy DOKŁADNIE jeden nazwany problem. Zero scope-creep.
Sekwencja: (a) potwierdź problem 1 zdaniem, (b) zrób, (c) zapisz wynik do _gates\<GATE>.md, (d) ja czyszczę czat.
Token-higiena: nie regeneruj całych plików. Patch = tylko zmieniony fragment + gdzie wkleić.

## 2. TWARDE ZASADY ARCHITEKTURY (nienaruszalne)
- O(N) to religia. Każda ścieżka hot przy 500+ NPC: udowodnij złożoność ZANIM wdrożysz. Loop-inversion (event broadcast), nie skanowanie.
- Single-bridge: TYLKO BTService_MaslowBlackboardSync pisze do blackboarda. Inny zapis = naruszenie, zgłoś.
- Data-driven: zero hardkodów statystyk. OCEAN/fizjologia przez UDataAsset/UDataTable.
- C++ = mózg (matematyka, pamięć, logika). Blueprint = ciało. Wizualia/animacje/particle TYLKO przez BlueprintImplementableEvent/NativeEvent.
- Fail-safe: każdy wskaźnik IsValid()/if(Ptr). Edge-case śmierci NPC w trakcie akcji/kontraktu P2P obsłużony.
- UE_LOG hojnie w punktach decyzyjnych (własne kategorie).

## 3. ZASADY NARZĘDZI / MCP
- MCP do AUTORINGU assetów (native, port 8000).
- PIE-verification NIGDY przez live MCP introspection (konkuruje z game thread, fałszuje pomiary). Dowód = UE_LOG parsowany PO PIE, twarde liczby.
- set_pin_value na pinach object/enum koruptuje BP -> te robię RĘCZNIE ja w edytorze, Ty mnie instruujesz, nie ruszasz.

## 4. BRAMKI NIEODWRACALNE — STOP i pytaj
Zatrzymaj się i czekaj na moje wyraźne "tak" przed: edycją/usuwaniem assetów BP, PIE, git push, hard-delete, nadpisaniem Content, zmianą permissions, czymkolwiek nieodwracalnym.
Autoryzacja zadania X != autoryzacja niezgłoszonego Y. Nie samo-autoryzuj poza bramkę.

## 5. TRYB PRACY
- Zadania odwracalne: assume-log-continue (załóż sensowny default, zaloguj założenie, jedź dalej — nie przerywaj pytaniami).
- Batch-report na końcu, nie po każdym kroku.
- Audyt PRZED specem: sprawdź istniejące taski/assety zanim dodasz nowe (unikaj duplikatów).

## 6. OUTPUT KAŻDEJ SESJI — plik _gates\<GATE>.md:
- RESOLVED: zablokowane decyzje / co zrobione (z dowodem: log/liczby).
- DEFAULTS: gapy pre-odpowiedziane założeniem.
- OPEN: co wymaga mojej decyzji.
Bramka zamknięta dopiero gdy Definition of Done spełniona i FALSYFIKOWALNA (runtime value change + log).
# ===== KONIEC PIPELINE =====
