#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "BodyConditionComponent.generated.h"

// Dedicated log category for the body/sense damage model.
DECLARE_LOG_CATEGORY_EXTERN(LogBody, Log, All);

/**
 * 26 body parts forming a containment hierarchy. Damage to a parent caps the
 * effective function of every child below it (see GetPartEffectiveHealth).
 */
UENUM(BlueprintType)
enum class EBodyPart : uint8
{
	Body		UMETA(DisplayName = "Body (root)"),
	Head		UMETA(DisplayName = "Head"),
	LeftEye		UMETA(DisplayName = "Left Eye"),
	RightEye	UMETA(DisplayName = "Right Eye"),
	LeftEar		UMETA(DisplayName = "Left Ear"),
	RightEar	UMETA(DisplayName = "Right Ear"),
	Tongue		UMETA(DisplayName = "Tongue"),
	Torso		UMETA(DisplayName = "Torso"),
	LeftArm		UMETA(DisplayName = "Left Arm"),
	LeftHand	UMETA(DisplayName = "Left Hand"),
	LeftThumb	UMETA(DisplayName = "Left Thumb"),
	LeftIndex	UMETA(DisplayName = "Left Index"),
	LeftMiddle	UMETA(DisplayName = "Left Middle"),
	LeftRing	UMETA(DisplayName = "Left Ring"),
	LeftPinky	UMETA(DisplayName = "Left Pinky"),
	RightArm	UMETA(DisplayName = "Right Arm"),
	RightHand	UMETA(DisplayName = "Right Hand"),
	RightThumb	UMETA(DisplayName = "Right Thumb"),
	RightIndex	UMETA(DisplayName = "Right Index"),
	RightMiddle	UMETA(DisplayName = "Right Middle"),
	RightRing	UMETA(DisplayName = "Right Ring"),
	RightPinky	UMETA(DisplayName = "Right Pinky"),
	LeftLeg		UMETA(DisplayName = "Left Leg"),
	LeftFoot	UMETA(DisplayName = "Left Foot"),
	RightLeg	UMETA(DisplayName = "Right Leg"),
	RightFoot	UMETA(DisplayName = "Right Foot")
};

/** Derived gameplay sense each part feeds into. */
UENUM(BlueprintType)
enum class ESenseType : uint8
{
	None				UMETA(DisplayName = "None (structural)"),
	Vision				UMETA(DisplayName = "Vision"),
	Hearing				UMETA(DisplayName = "Hearing"),
	Speech				UMETA(DisplayName = "Speech"),
	HandPrecisionLeft	UMETA(DisplayName = "Hand Precision (Left)"),
	HandPrecisionRight	UMETA(DisplayName = "Hand Precision (Right)"),
	Mobility			UMETA(DisplayName = "Mobility")
};

/** Nature of an injury, for HUD display and treatment routing. */
UENUM(BlueprintType)
enum class EInjuryType : uint8
{
	None		UMETA(DisplayName = "Healthy"),
	Wound		UMETA(DisplayName = "Wound"),
	Fracture	UMETA(DisplayName = "Fracture"),
	Sprain		UMETA(DisplayName = "Sprain"),
	Burn		UMETA(DisplayName = "Burn"),
	Sever		UMETA(DisplayName = "Severed")
};

/** Static definition of one body part: where it sits and what it powers. */
USTRUCT(BlueprintType)
struct FBodyPartDefinition
{
	GENERATED_BODY()

	/** Containing part. Damage here caps this part's effective health. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	EBodyPart Parent = EBodyPart::Body;

	/** Which derived sense this part contributes to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	ESenseType FeedsSense = ESenseType::None;

	/** Relative contribution to that sense (e.g. thumb >> pinky for grip). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0.0"))
	float SenseWeight = 0.0f;
};

/**
 * Shared, data-driven body hierarchy. ONE asset referenced by all NPCs.
 * Optional — if unset, the component falls back to a built-in default
 * (see UBodyConditionComponent::GetDefaultHierarchy).
 */
UCLASS(BlueprintType)
class STAN_PIERWOTNY_API UBodyHierarchyAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	TMap<EBodyPart, FBodyPartDefinition> Parts;
};

/**
 * Per-NPC body / sense state. SPARSE: only damaged parts are stored
 * (absence = 100% healthy). Derived senses are CACHED and recomputed only
 * when a part changes — never per frame. No Tick.
 */
UCLASS(ClassGroup = (NPC), meta = (BlueprintSpawnableComponent))
class STAN_PIERWOTNY_API UBodyConditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBodyConditionComponent();

protected:
	virtual void BeginPlay() override;

public:
	/** Optional shared hierarchy. If null, built-in default is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body|Data")
	TObjectPtr<UBodyHierarchyAsset> Hierarchy = nullptr;

	/** Sparse health map. Absent key = 1.0 (healthy). Range 0..1. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Body")
	TMap<EBodyPart, float> PartHealth;

	/** Injury type for damaged parts (HUD display). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Body")
	TMap<EBodyPart, EInjuryType> PartInjury;

	// --- Per-part queries -------------------------------------------------

	/** Own health of a part (ignores parent), 0..1. */
	UFUNCTION(BlueprintPure, Category = "Body")
	float GetPartHealth(EBodyPart Part) const;

	/** Effective health = own capped by the whole parent chain, 0..1.
	 *  A 40% arm caps its hand and fingers at 40% no matter their own health. */
	UFUNCTION(BlueprintPure, Category = "Body")
	float GetPartEffectiveHealth(EBodyPart Part) const;

	UFUNCTION(BlueprintPure, Category = "Body")
	EInjuryType GetPartInjury(EBodyPart Part) const;

	/** Human-readable display name for a part (e.g. "Left Eye").
	 *  Runtime-safe: names are baked in C++, NOT read from editor-only UMETA
	 *  DisplayName metadata (which is stripped in cooked builds). One place. */
	UFUNCTION(BlueprintPure, Category = "Body")
	static FText GetPartDisplayName(EBodyPart Part);

	// --- Derived senses (cached, read by BT/EQS) --------------------------

	UFUNCTION(BlueprintPure, Category = "Body|Senses")
	float GetVisionAcuity() const { return VisionAcuity; }

	UFUNCTION(BlueprintPure, Category = "Body|Senses")
	float GetHearingAcuity() const { return HearingAcuity; }

	UFUNCTION(BlueprintPure, Category = "Body|Senses")
	float GetSpeechClarity() const { return SpeechClarity; }

	UFUNCTION(BlueprintPure, Category = "Body|Senses")
	float GetHandPrecision(bool bRightHand) const
	{
		return bRightHand ? HandPrecisionRight : HandPrecisionLeft;
	}

	UFUNCTION(BlueprintPure, Category = "Body|Senses")
	float GetMobility() const { return Mobility; }

	// --- Mutations --------------------------------------------------------

	/** Damage a part (Amount 0..1 subtracted from its health), set injury type,
	 *  then recompute senses once. */
	UFUNCTION(BlueprintCallable, Category = "Body")
	void ApplyDamage(EBodyPart Part, float Amount, EInjuryType Injury);

	/** Heal a part. At full health it is removed from the sparse map. */
	UFUNCTION(BlueprintCallable, Category = "Body")
	void HealPart(EBodyPart Part, float Amount);

protected:
	// Cached derived senses (0..1).
	float VisionAcuity			= 1.0f;
	float HearingAcuity			= 1.0f;
	float SpeechClarity			= 1.0f;
	float HandPrecisionLeft		= 1.0f;
	float HandPrecisionRight	= 1.0f;
	float Mobility				= 1.0f;

	/** Recompute all cached senses. Called only on change. */
	void RecalculateDerivedStats();

	/** Weighted average of effective health over all parts feeding a sense. */
	float ComputeSense(ESenseType Sense) const;

	/** Resolve part definition from asset, else from built-in default. */
	const FBodyPartDefinition* FindPartDef(EBodyPart Part) const;

	/** Lazily-built static default hierarchy (used when no asset assigned). */
	static const TMap<EBodyPart, FBodyPartDefinition>& GetDefaultHierarchy();

	/** Visual/SFX hook — implement in Blueprint (e.g. play hurt anim, blood). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Body|Events")
	void OnBodyChanged();
};
