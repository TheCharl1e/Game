# NPCRegistry (L3-01) — DESIGN GATE
> Projekt do akceptacji (architekt, 2026-06-19). KOD DOPIERO PO „zatwierdzam".
> Keystone: zamienia int32 ID → żywego NPC. Nic społecznego (P2P L3, reputacja L4, detektyw L5)
> nie ruszy bez tego. Cel: 500–1000+ NPC.
>
> Decyzje wejściowe (już podjęte z Szymonem):
> • ID PROSTE, narastające (NextNPCId++), **nigdy nie recyklowane** (martwy NPC zabiera numer do grobu).
> • BRAK save/load świata → ID runtime-only, bez persistence (decyzja: „gra ma być odpowiedzialna bez saveów").
> • Rejestracja przez `BeginPlay` NPC (fakt: brak spawnera, NPC to statyczne instancje w Game.umap).
>
> Fakty z kodu (EXECUTOR, read-only):
> • Brak bazowej klasy C++ `ANPC`. NPC = Blueprint `BP_NPC_Character` (parent `ACharacter`) z komponentami.
> • NPC nie ma żadnego pola ID. Komentarz `ItemBase.h:53` już zakłada: „NPC identity comes from the NPC Registry (int32 ID)".
> • Brak spawnera — NPC stoją ręcznie w mapie (`_C_1`, `_C_2`).

═══════════════════════════════════════════════════════════
# DECYZJE PROJEKTOWE (zatwierdzane — NIE zmieniać bez architekta)
═══════════════════════════════════════════════════════════
| # | Decyzja | Wartość |
|---|---|---|
| D-SUB | Gdzie żyje rejestr | **`UNPCRegistrySubsystem : UWorldSubsystem`** (jeden na świat, lifecycle = świat) |
| D-ID | Typ i nadawanie ID | **int32 narastające, start=1, 0=INVALID, nigdy nie recyklowane** |
| D-HOME | Kto woła rejestrację | **Nowy `UNPCIdentityComponent`** na NPC — register w `BeginPlay`, unregister w `EndPlay` |
| D-RET | Co zwraca lookup | **`ACharacter*`** (kanoniczny uchwyt „NPC"); komponenty przez `FindComponentByClass` |
| D-API | Powierzchnia API | **`RegisterNPC` / `UnregisterNPC` / `GetNPCById`** (+ `GetRegisteredCount` debug). BEZ `GetAllNPCs` |

🔑 **Dlaczego osobny `UNPCIdentityComponent`, a nie rejestracja z `UMaslowBiologicalComponent`:**
tożsamość ≠ biologia. Cała warstwa społeczna (reputacja, OCEAN, alibi) będzie wisieć na tożsamości,
nie na metabolizmie — pytanie „kim jesteś?" nie powinno iść przez komponent głodu. Pasuje do wzorca
projektu (wszystko jest komponentem: Maslow, Body, Inventory). Koszt: **jedno dodanie komponentu do
`BP_NPC_Character`** (edytor) — instancje `_C_1/_C_2` dziedziczą.
🔻 **Fallback (gdyby dodanie komponentu do BP było problematyczne):** rejestracja z istniejącego
`UMaslowBiologicalComponent::BeginPlay/EndPlay` (zero edycji BP). EXECUTOR: oceń i zgłoś, jeśli wolisz fallback.

═══════════════════════════════════════════════════════════
# 🧮 KOSZT PRZY 500–1000 NPC (soczewka RTS — dowód, że nie zabije RAM)
═══════════════════════════════════════════════════════════
**Pamięć rejestru:** `TMap<int32, TWeakObjectPtr<ACharacter>>`.
- Wpis: klucz int32 (4B) + `TWeakObjectPtr` (8B: ObjectIndex + SerialNumber) = 12B payload.
- Narzut TMap (sparse array + hash/alignment): ~16–20B/wpis.
- **≈ 30B/wpis → ~15 KB @ 500 NPC, ~30 KB @ 1000 NPC.** Plus `NextNPCId` (4B) i obiekt subsystemu (znikome).

**Pamięć per NPC:** `UNPCIdentityComponent` trzyma 1× int32 (4B) + narzut UObject komponentu (~kilkadziesiąt B).
Komponent i tak by istniał — to nie jest dodatkowy koszt rejestru, tylko nośnik ID.

**CPU (kluczowe — i tu jest cała oszczędność):**
- `RegisterNPC`: 1× `TMap.Add` + inkrement = **O(1)**, raz na NPC w `BeginPlay`. 500 wywołań na starcie. Nic.
- `UnregisterNPC`: 1× `TMap.Remove` = **O(1)**, raz na śmierć. Nic.
- `GetNPCById`: 1× `TMap.Find` + `IsValid` = **O(1)**, wołane przy ROZWIĄZYWANIU RELACJI (P2P/alibi/reputacja),
  **NIE co-tick**. Nawet setki/s = zero problemu.
- **Zero ticka. Zero iteracji. Zero logów per-lookup.** Logi tylko na register/unregister/self-heal.

🔑 Ryzyko keystone'a NIE leży w pamięci (15–30 KB to nic). To najtańszy fundament w projekcie.

═══════════════════════════════════════════════════════════
# KOD — SUBSYSTEM (NOWY plik)
═══════════════════════════════════════════════════════════
> ⚠️ EXECUTOR: potwierdź dokładny makro-API modułu kopiując z istniejącego nagłówka
> (np. `MaslowBiologicalComponent.h`) — `STAN_PIERWOTNY_API` vs `STANPIERWOTNY_API`.

## NPCRegistrySubsystem.h
```cpp
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NPCRegistrySubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNPCRegistry, Log, All);

class ACharacter;

UCLASS()
class STAN_PIERWOTNY_API UNPCRegistrySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Registers an NPC, assigns a fresh unique int32 ID (never recycled). Returns new ID, or 0 on failure. */
    int32 RegisterNPC(ACharacter* NPC);

    /** Removes an NPC by ID. The ID is retired and NEVER reused. Idempotent. */
    void UnregisterNPC(int32 NPCId);

    /** Resolves an int32 ID to a live NPC. nullptr if unknown/dead. Self-heals stale entries. */
    UFUNCTION(BlueprintCallable, Category="NPC|Registry")
    ACharacter* GetNPCById(int32 NPCId);

    /** Live registered NPC count (debug/telemetry). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="NPC|Registry")
    int32 GetRegisteredCount() const { return RegisteredNPCs.Num(); }

private:
    /** int32 ID -> live NPC. TWeakObjectPtr auto-nulls on destruction (safety net). */
    UPROPERTY()
    TMap<int32, TWeakObjectPtr<ACharacter>> RegisteredNPCs;

    /** Next ID to hand out. ONLY increments — IDs never recycled. 0 reserved as INVALID. */
    int32 NextNPCId = 1;
};
```

## NPCRegistrySubsystem.cpp
```cpp
#include "NPCRegistrySubsystem.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY(LogNPCRegistry);

int32 UNPCRegistrySubsystem::RegisterNPC(ACharacter* NPC)
{
    if (!IsValid(NPC))
    {
        UE_LOG(LogNPCRegistry, Warning, TEXT("[Registry] RegisterNPC: invalid NPC — skipped."));
        return 0;   // INVALID
    }

    const int32 NewId = NextNPCId++;          // never rolls back — IDs retired forever
    RegisteredNPCs.Add(NewId, NPC);

    UE_LOG(LogNPCRegistry, Log, TEXT("[Registry] Registered %s as ID %d (live=%d)."),
           *GetNameSafe(NPC), NewId, RegisteredNPCs.Num());
    return NewId;
}

void UNPCRegistrySubsystem::UnregisterNPC(int32 NPCId)
{
    if (NPCId <= 0) { return; }               // 0/negative = never registered

    if (RegisteredNPCs.Remove(NPCId) > 0)
    {
        UE_LOG(LogNPCRegistry, Log, TEXT("[Registry] Unregistered ID %d (live=%d). ID retired."),
               NPCId, RegisteredNPCs.Num());
    }
    // not present → silent (idempotent)
}

ACharacter* UNPCRegistrySubsystem::GetNPCById(int32 NPCId)
{
    if (NPCId <= 0) { return nullptr; }

    TWeakObjectPtr<ACharacter>* Found = RegisteredNPCs.Find(NPCId);
    if (!Found) { return nullptr; }           // unknown ID

    ACharacter* NPC = Found->Get();
    if (IsValid(NPC)) { return NPC; }

    // Stale: died without unregister (should not happen — diagnostic + self-heal).
    UE_LOG(LogNPCRegistry, Warning,
           TEXT("[Registry] ID %d stale (destroyed without unregister) — self-healed."), NPCId);
    RegisteredNPCs.Remove(NPCId);
    return nullptr;
}
```

═══════════════════════════════════════════════════════════
# KOD — IDENTITY COMPONENT (NOWY plik, dodawany do BP_NPC_Character)
═══════════════════════════════════════════════════════════
## NPCIdentityComponent.h
```cpp
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCIdentityComponent.generated.h"

UCLASS(ClassGroup=(NPC), meta=(BlueprintSpawnableComponent))
class STAN_PIERWOTNY_API UNPCIdentityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNPCIdentityComponent();

    /** Registry ID. 0 = not registered. Assigned on BeginPlay, read by all social systems. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="NPC|Identity")
    int32 GetNPCId() const { return NPCId; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    /** Assigned by registry on BeginPlay. Never changes once set. 0 = unregistered. */
    UPROPERTY(VisibleInstanceOnly, Category="NPC|Identity")
    int32 NPCId = 0;
};
```

## NPCIdentityComponent.cpp
```cpp
#include "NPCIdentityComponent.h"
#include "NPCRegistrySubsystem.h"
#include "GameFramework/Character.h"

UNPCIdentityComponent::UNPCIdentityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;   // project rule: NO Event Tick
}

void UNPCIdentityComponent::BeginPlay()
{
    Super::BeginPlay();

    if (NPCId != 0) { return; }                  // EC: idempotent (double BeginPlay)

    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!IsValid(OwnerChar))
    {
        UE_LOG(LogNPCRegistry, Warning, TEXT("[Identity] %s: owner not a Character — not registered."),
               *GetNameSafe(GetOwner()));
        return;
    }

    UWorld* World = GetWorld();
    if (!IsValid(World)) { return; }             // EC: teardown

    UNPCRegistrySubsystem* Registry = World->GetSubsystem<UNPCRegistrySubsystem>();
    if (!IsValid(Registry))
    {
        UE_LOG(LogNPCRegistry, Warning, TEXT("[Identity] %s: registry unavailable — not registered."),
               *GetNameSafe(OwnerChar));
        return;
    }

    NPCId = Registry->RegisterNPC(OwnerChar);
}

void UNPCIdentityComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (NPCId != 0)
    {
        if (UWorld* World = GetWorld())
        {
            if (UNPCRegistrySubsystem* Registry = World->GetSubsystem<UNPCRegistrySubsystem>())
            {
                Registry->UnregisterNPC(NPCId);
            }
        }
        NPCId = 0;
    }
    Super::EndPlay(Reason);
}
```

═══════════════════════════════════════════════════════════
# 🛡️ FAIL-SAFE'Y / EDGE CASES (krytyczne dla setek NPC)
═══════════════════════════════════════════════════════════
| # | Sytuacja | Zabezpieczenie |
|---|---|---|
| EC-1 | NPC ginie (kontrakt P2P, walka) | `EndPlay` wyrejestrowuje — `EndPlay` ZAWSZE pada przy destroy aktora |
| EC-2 | `EndPlay` jakimś cudem pominięte | `TWeakObjectPtr` auto-null + `GetNPCById` self-heal (usuwa martwy wpis, loguje Warning) |
| EC-3 | Podwójne `BeginPlay` | guard `if (NPCId != 0) return;` — idempotentne |
| EC-4 | `GetWorld()`/subsystem null w teardown | każdy dostęp przez `IsValid`/`if` — brak crasha, NPC po prostu nieзаrejestrowany + Warning |
| EC-5 | `GetNPCById(0)` / nieznane ID | `nullptr` czysto (guard `<=0`, `Find`→nullptr) |
| EC-6 | Owner nie jest `ACharacter` | Cast guard → Warning, skip (nie zakłada, że każdy aktor to NPC) |

Reguła: martwy NPC NIE zostawia wiszącego ID ani fałszywego wskaźnika. Numer umiera z nim i nie wraca.

═══════════════════════════════════════════════════════════
# WERYFIKACJA (Claude Code — twarde liczby, zero placeholderów)
═══════════════════════════════════════════════════════════
Build z ZAMKNIĘTYM edytorem → UHT. Potem PIE + log z `Saved/Logs/` + Monolith live read.
W Game.umap stoją 2 instancje NPC (`_C_1`, `_C_2`) — to nasz materiał testowy.

1. **Rejestracja:** PIE → log `[Registry] Registered ... as ID 1` i `... ID 2`. `GetRegisteredCount()==2` (live read).
2. **ID unikalne + nie-recyklowane:** zniszcz NPC ID 1 → log `Unregistered ID 1`, count==1.
   Postaw/spawnij nowego NPC → dostaje **ID 3, NIE 1** (dowód: `NextNPCId` nie cofnął się). To jest sedno decyzji „nigdy nie recyklujemy".
3. **Lookup:** `GetNPCById(2)` → zwraca żywego NPC (live). `GetNPCById(1)` po śmierci → `nullptr`. `GetNPCById(999)` → `nullptr`.
4. **EndPlay sprząta:** zabij NPC w trakcie PIE → log unregister pada, brak Warning o stale przy kolejnym lookup.
5. **EC-5:** `GetNPCById(0)` → `nullptr`, brak crasha.
6. **Brak ticka/spamu:** w logu zero wpisów per-lookup; logi tylko na register/unregister.

Skala 500/1000: nie ma jeszcze spawnera, więc liczba pamięci to matematyka (sekcja RTS), a mechanizm
dowodzimy na 2 instancjach. Gdy powstanie spawner — masowy test rejestracji.

═══════════════════════════════════════════════════════════
# CO TEN ETAP ROBI / CZEGO NIE
═══════════════════════════════════════════════════════════
✅ ROBI: `UNPCRegistrySubsystem` (Register/Unregister/GetById/Count), `UNPCIdentityComponent`
   (register w BeginPlay, unregister w EndPlay), unikalne nie-recyklowane int32 ID, self-heal, fail-safe'y.
❌ NIE ROBI (HOOKI / następne etapy):
- `GetAllNPCs()` / iteracja po wszystkich → footgun przy 500 (kopia tablicy). Dobudujemy, GDY realny konsument tego zażąda.
- Integracja ze spawnerem → spawnera nie ma; statyczne instancje rejestrują się przez BeginPlay. Spawner później zarejestruje tak samo.
- Persistence ID przez save/load → BRAK saveów (decyzja). ID runtime-only.
- Dane społeczne (reputacja, OCEAN, log alibi) → wiszą na `UNPCIdentityComponent` w PRZYSZŁYCH etapach (L3/L4/L5).
- Cache strefy (`CurrentZone` z AmbientTemp) → osobny temat; docelowo może też siąść na Identity, ale NIE teraz.

═══════════════════════════════════════════════════════════
# 🔑 EXECUTOR — POTWIERDŹ PRZED KODOWANIEM (nie zgaduj)
═══════════════════════════════════════════════════════════
1. Dokładny makro-API modułu (`STAN_PIERWOTNY_API`?) — skopiuj z istniejącego nagłówka.
2. Czy dodanie `UNPCIdentityComponent` do **klasy** `BP_NPC_Character` jest czyste (instancje dziedziczą)?
   Jeśli nie — zgłoś, wchodzi fallback (rejestracja z `UMaslowBiologicalComponent`).
3. Czy `UWorldSubsystem` jest dostępny w `BeginPlay` NPC (powinien — subsystemy startują przed aktorami). Guard i tak jest.
4. Kategoria logu: nowa `LogNPCRegistry` czy reuse istniejącej konwencji? (Proponuję nową — czysta separacja.)
5. Cokolwiek się nie zgadza z tym docem → FLAGUJ (jak przy cold-burn), nie zgaduj.

KOLEJNOŚĆ: (a) napisz 2 klasy C++ headless → (b) edytor: dodaj komponent do BP_NPC_Character → (c) build (edytor zamknięty) → (d) PIE verify wg bloków → (e) CHANGELOG + CODE_REGISTRY + MILESTONES (z hashem).
