# G0 — Nic (punkt zero)

Oś: G (grywalny stan) · Status: ✅ (osiągnięty — punkt odniesienia)

## Czym G0 jest
Pusty **Game_58**: silnik UE 5.8 + moduły `Stan_Pierwotny`/`Stan_PierwotnyEditor` kompilują się,
edytor startuje, MCP (MCPUnreal 8090 / natywny) steruje edytorem. **Zero gameplayu** — brak żywej
pętli NPC, brak zaludnionej mapy, brak grywalnej symulacji.

## Po co
G0 to **punkt zero pomiaru** — baseline, względem którego mierzymy każdy następny stan G:
koszt pustego ticku, czas startu, budżet klatki bez agentów. Wszystko, co G1 dodaje, liczy się
jako delta ponad G0.

## DoD
- Editor target buduje się (editor-closed UHT, `Result: Succeeded`).
- Edytor wstaje, PIE startuje na pustej/testowej mapie bez błędów Fatal.
- MCP odpowiada (script_execution żywe).

> Źródło treści: stan faktyczny Game_58 (README_CANON.md, PROJECT_STATE.md). Szkielet — bez migracji.
