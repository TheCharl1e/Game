# Oś L — NPC / Piramida Maslowa

Oś **L** to NPC jako **piramida potrzeb Maslowa**. Każdy poziom L(n) to jedna warstwa potrzeb;
NPC wybiera akcję wg priorytetu (niższy poziom przebija wyższy, gdy niezaspokojony).

**Strzałka-w-dół:** L(n) zna **tylko** L(<n) — wyższa potrzeba może czytać niższe (bo one ją gatują),
ale niższa nigdy nie zależy od wyższej. Świat (oś C) jest czytany przez interfejsy, nigdy odwrotnie.

**Single-bridge (jedyny most C++→BB):** **tylko** `BTService_MaslowBlackboardSync` zapisuje Blackboard.
Żaden inny system nie pisze kluczy potrzeb do BB — jeden pisarz = brak dwóch rozjechanych modeli
(historyczny dług „drugiego mózgu" BP zamknięty).

## Spis warstw
| ID | Warstwa | Status |
|---|---|---|
| L0 | `L0_fight_or_flight` — stres, panika, flee | 🔴 (model gotowy, kod nie) |
| L1 | `L1_hunger` · `L1_thirst` · `L1_temperature` · `L1_sleep` · `L1_body_senses` | ✅ |
| L2 | `L2_safety` — bezpieczeństwo/stabilność, ekwipunek, człowieczeństwo | 🔨 |
| L3 | `L3_belonging` — przynależność/socjalizacja (P2P, OCEAN) | ⬜ |
| L4 | `L4_esteem` — szacunek/uznanie/dominacja | ⬜ |
| L5 | `L5_actualization` — samorealizacja/eureki/geopolityka | ⬜ |

> Treść warstw migrowana w ETAP 2 (per-warstwa, osobna sesja) wg `_gates/MIGRATION_MAP.md`.
