# L1 — Sen (Sleep / Rest)

Oś: L (NPC) · Warstwa: L1 · Status: ✅

**Źródło treści:** `SLEEP_ENGINE_design.md` (ETAP 1) + `SLEEP_ENGINE_ETAP2_design.md` (skutki + reset)

**Zakres (1 zdanie):** Sen jako narastanie `HoursAwake` na zegarze świata (C0) → mgła/mikrosen/omdlenie, sen dobrowolny + auto-wake z pułapki omdlenia, reset + buff Rested.

**Strzałka-w-dół:** L1 zna tylko L0. Tempo czuwania dostrojone do „jednego zegara" (C0); potrzeba do BB tylko przez `BTService_MaslowBlackboardSync`.

> SZKIELET (ETAP 1). Treść do zmigrowania w ETAP 2 wg `_gates/MIGRATION_MAP.md`.
