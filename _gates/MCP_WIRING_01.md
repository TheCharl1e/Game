# GATE: PORZADEK-02 — MCP wiring + szkielet governance

Data: 2026-06-30 · Wykonawca: Claude Code · Target: E:\Game_58 (UE5.8)
Akcje nieodwracalne wykonane: ŻADNE. Pluginów NIE włączano, NIC nie pobierano z sieci, `.uproject` NIE edytowany.

---

## RESOLVED (zrobione, z dowodem)

### 1. Config klienta `.mcp.json` — JUŻ POPRAWNY (nie nadpisano)
`E:\Game_58\.mcp.json` istnieje i wskazuje native endpoint:
```json
{ "mcpServers": { "ue58": { "type": "http", "url": "http://127.0.0.1:8000/mcp" } } }
```
- `type: http` = transport streamable-http w kliencie Claude Code (poprawne).
- Plik jest `.gitignore`-owany (lokalny config / ścieżki usera) — celowe, nie commitujemy.
- Założenie: jeśli wygenerujesz config przez `ModelContextProtocol.GenerateClientConfig`, ten wygenerowany ma priorytet — NIE nadpisywałem ręcznie.

### 2. Weryfikacja portu (netstat)
| Port | Rola | Wynik |
|---|---|---|
| **:8000** | native MCP (kanon, 58) | **BRAK LISTENERA** |
| :8090 | MCPUnreal (legacy bridge) | LISTENING, PID **5700 = UnrealEditor.exe** |
| :9316 | Monolith (legacy bridge) | brak listenera |

**Diagnoza:** native :8000 NIE wstał, bo działający edytor (PID 5700) to **projekt 5.7 `E:\Game`** (status mcp-unreal: `project_root: E:\Game`, `uproject: E:\Game\Stan_Pierwotny.uproject`), a nie Game_58. Native MCP 58 wstaje tylko gdy działa edytor Game_58 z uruchomionym serwerem.
→ **AKCJA RĘCZNA (Ty):** zamknij edytor 5.7, otwórz `Game_58.uproject`, wystartuj serwer MCP (StartServer/autostart). Patrz checklist w `README_CANON.md`.

### 3. Szkielet governance — UTWORZONY
- `E:\Game_58\_gates\` ✔ (zawiera PORZADEK_01.md + ten plik)
- `E:\Game_58\_pipeline\PIPELINE.md` ✔ (treść ŚWIĘTEGO PIPELINE wklejona)
- `E:\Game_58\_redteam\` ✔ (README.md — przeznaczenie: adversarial/falsyfikacja)
- Zacommitowane lokalnie (NO push) — hash w sekcji DEFAULTS.

### 4. Checklist ręczny MCP — DODANY do README_CANON.md
Kroki: enable „Unreal MCP" (ModelContextProtocol) + „AllToolsets" + restart; (opcjonalnie) VibeUE z Fab; StartServer; GenerateClientConfig; weryfikacja `netstat :8000`. Pełna treść w `README_CANON.md`.

---

## DEFAULTS (domknięte założeniem)
- Plugin status w `.uproject`: `ModelContextProtocol` Enabled=true, `AllToolsets` Enabled=true, `MCPUnreal` Enabled=true — warunek briefu (włączasz ręcznie) już spełniony w pliku; weryfikacja w UI po Twojej stronie.
- Commit governance objął: `README_CANON.md`, `_pipeline/PIPELINE.md`, `_redteam/README.md`, `_gates/*.md`. Stray pliki źródłowe RTS pominięte (patrz PORZADEK_01 OPEN-4).

---

## OPEN (wymaga Twojej decyzji)
- **OPEN-A. Native :8000 nie działa** — wymaga Twojej akcji w edytorze 58 (zamknięcie 5.7 + StartServer). Dopóki działa 5.7 na :8090, kanoniczny tor authoringu (native) jest offline.
- **OPEN-B. MCPUnreal (:8090, 5.7) wciąż żyje** — czy ubić edytor 5.7, by nie konkurował/nie mylił z kanonem 58? (Powiązane z PORZADEK_01 OPEN-1.)
