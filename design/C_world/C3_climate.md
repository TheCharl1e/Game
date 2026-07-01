# C3 — Klimat (temperatura)

Oś: C (świat) · Warstwa: C3 · Status: ✅ (rdzeń + doba)

**Źródło treści:** `AMBIENT_TEMP_design.md` (strona świata) + `REPORTS/raport_2026-06-19_ambienttemp_recon.md` + `raport_2026-06-19_warstwa_doby.md`

**Zakres (1 zdanie):** Temperatura otoczenia jako `AmbientTemp = BaseTemp(strefa) + offset doby` — rdzeń strefowy + warstwa dobowa (bez pełnej pogody i słońca sezonowego — Track B).

**Import:** C3 importuje C0-C1 (czas dla offsetu doby, strefa dla BaseTemp). Odczuwanie temperatury przez ciało = L1_temperature (oś L, przez interfejs).

> SZKIELET (ETAP 1). Treść do zmigrowania w ETAP 2 wg `_gates/MIGRATION_MAP.md`.
