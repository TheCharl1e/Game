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
| **:8000** | native MCP (kanon, 58) | **LIVE** (po StartServer — patrz niżej) |
| :8090 | MCPUnreal (legacy bridge) | LISTENING, PID **5700 = UnrealEditor.exe** |
| :9316 | Monolith (legacy bridge) | brak listenera |

**Diagnoza (SKORYGOWANA 2026-06-30):** PID 5700 to **kanoniczny edytor 58**, nie 5.7. Command-line procesu:
`"E:/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "E:\Game_58\Game_58.uproject"`. Binarka UE_5.8 + uproject 58.
Żaden edytor 5.7 NIE działa. Pole `project_root: E:\Game` w `status` pochodzi z osobnego, statycznie skonfigurowanego serwera narzędziowego mcp-unreal — NIE z żywego procesu (błąd pierwotnej diagnozy).
- Listener **:8090 = plugin MCPUnreal wewnątrz edytora 58** (narzędzie zachowane decyzją reżysera).
- Native **:8000 down**, bo w otwartym edytorze 58 NIE wystartowano serwera ModelContextProtocol (brak autostartu).
→ **AKCJA RĘCZNA (Ty):** w już otwartym edytorze 58 uruchom serwer MCP (`StartServer`/autostart). NIE trzeba nic zamykać. Patrz checklist w `README_CANON.md`.
→ **NIE ubijać PID 5700** — to edytor 58. Instrukcja „ubij edytor 5.7" oparta na nieaktualnym założeniu: takiego procesu nie ma.

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
- ~~OPEN-A. Native :8000 nie działa~~ → **RESOLVED 2026-06-30.** Po `ModelContextProtocol.StartServer` w edytorze 58: listener `127.0.0.1:8000` LISTENING (PID 5700 = edytor 58); endpoint `POST /mcp` initialize → HTTP 200, JSON-RPC `protocolVersion: 2024-11-05`, `capabilities.tools.listChanged: true`, `Mcp-Session-Id` zwrócony. `.mcp.json` (8000//mcp) zgodny z domyślnymi pluginu (`DefaultServerPort=8000`).
- **Auto Start Server — WŁĄCZONE i ZWERYFIKOWANE 2026-06-30.** `bAutoStartServer=True` w `Config/DefaultEditorPerProjectUserSettings.ini`. Dowód po restarcie edytora (świeży PID 12968, start 19:35): `:8000` LISTENING bez ręcznego StartServer; log `[ 0]LogModelContextProtocol: Starting MCP server on port 8000` (indeks klatki 0 = faza startu modułu, nie konsola); `POST /mcp` initialize → HTTP 200 JSON-RPC + Mcp-Session-Id. Edytor wstaje z MCP samodzielnie.
- ~~OPEN-B. MCPUnreal (:8090)~~ → **RESOLVED:** :8090 to plugin MCPUnreal wewnątrz edytora 58 (nie 5.7), zachowany decyzją reżysera; działa równolegle z native :8000, nie koliduje.
