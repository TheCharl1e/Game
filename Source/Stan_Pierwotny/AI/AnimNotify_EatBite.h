#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EatBite.generated.h"

/**
 * APPETITE / GRUBAS slice 1 — "zegar gryzień".
 * Marker AnimNotify wkładany na anim/montage jedzenia NPC. Każde wystąpienie (ugryzienie) woła
 * UMaslowBiologicalComponent::ConsumeBite() na ownerze mesha. C++ NIE dotyka animacji — to animacja
 * (data-driven notify) napędza rytm gryzień, zero nowego timera. BP = ciało, C++ = skutek.
 */
UCLASS(meta = (DisplayName = "Eat Bite (Maslow)"))
class STAN_PIERWOTNY_API UAnimNotify_EatBite : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                        const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
    virtual FString GetNotifyName_Implementation() const override;
#endif
};
