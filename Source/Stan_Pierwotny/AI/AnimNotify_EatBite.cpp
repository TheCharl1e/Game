#include "AnimNotify_EatBite.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "MaslowBiologicalComponent.h"

void UAnimNotify_EatBite::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                 const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!IsValid(MeshComp))
    {
        return;
    }

    // Owner mesha = NPC. Znajdź jego komponent biologiczny i zgłoś jedno ugryzienie.
    if (AActor* OwnerActor = MeshComp->GetOwner())
    {
        if (UMaslowBiologicalComponent* Maslow = OwnerActor->FindComponentByClass<UMaslowBiologicalComponent>())
        {
            Maslow->ConsumeBite();   // C++ liczy skutek kęsa; jeśli sesja zamknięta — early-return wewnątrz.
        }
    }
}

#if WITH_EDITOR
FString UAnimNotify_EatBite::GetNotifyName_Implementation() const
{
    return TEXT("Eat Bite");
}
#endif
