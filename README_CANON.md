# README — CANON / BACKUP (Stan Pierwotny / Caldreth)

> Status doc. Ustala, który projekt jest źródłem prawdy i jak podpiąć narzędzia.

## Kanon vs backup
- **E:\Game_58 (UE 5.8) = KANON dev.** Tu zapada cała praca: kod, assety, weryfikacja, git.
- **E:\Game (UE 5.7) = FROZEN BACKUP.** Tylko źródło do migracji / odczyt historyczny. NIE rozwijać.
- Ścieżka **„MCP-on-5.7" jest PORZUCONA.** Natywny MCP (ModelContextProtocol + AllToolsets, endpoint `http://127.0.0.1:8000/mcp`) istnieje TYLKO na 5.8. Legacy bridge (MCPUnreal :8090 / Monolith :9316) nie jest kanonicznym torem authoringu na 58.

## Stan migracji (audyt PORZADEK-01, 2026-06-30)
- Assety Content: 5.7 = 3013 (.uasset+.umap), 5.8 = 3017. **Braków w 58: 0.** 58 ma +4 nowe assety AI (BT_Exploration, EQS_FindResource/SafeZone/Water). Migracja kompletna.

---

## CHECKLIST RĘCZNY — uruchomienie natywnego MCP (robi reżyser, w edytorze 5.8)
Ten projekt steruje MCP przez natywny plugin silnika. Kroki manualne, których Claude Code NIE wykonuje:

1. **Otwórz edytor projektu Game_58** (`E:\Game_58\Game_58.uproject`) — nie 5.7! (native 8000 wstaje tylko gdy działa edytor 58).
2. **Plugins → włącz „Unreal MCP" (ModelContextProtocol)** — Enabled. *(W .uproject już Enabled — zweryfikuj w UI.)*
3. **Plugins → włącz „AllToolsets"** — Enabled. *(W .uproject już Enabled.)*
4. **Restart edytora** po zmianie pluginów.
5. *(Opcjonalnie)* **VibeUE z Fab** — jeśli chcesz dodatkowe toolsety, zainstaluj z Fab (Claude Code nic nie pobiera z sieci).
6. **Wystartuj serwer MCP**: ModelContextProtocol nie autostartuje — uruchom `StartServer` (konsola edytora / przycisk pluginu) lub włącz autostart w ustawieniach pluginu. Cel: listener na `127.0.0.1:8000`.
7. **Wygeneruj config klienta**: jeśli plugin udostępnia `ModelContextProtocol.GenerateClientConfig`, użyj go — wygenerowany `.mcp.json` jest źródłem prawdy, nie nadpisuj ręcznie.
8. **Weryfikacja**: `netstat -ano | findstr :8000` → listener na 127.0.0.1. Dopiero wtedy `.mcp.json` (server `ue58`) działa.

> UWAGA: dopóki działa edytor 5.7 (E:\Game) z MCPUnreal na :8090, native :8000 dla 58 nie wstanie obok — to dwa różne projekty. Zamknij edytor 5.7 przed odpaleniem 58 do authoringu.
