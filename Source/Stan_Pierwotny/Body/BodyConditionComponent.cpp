#include "BodyConditionComponent.h"

DEFINE_LOG_CATEGORY(LogBody);

UBodyConditionComponent::UBodyConditionComponent()
{
	// No Tick — senses recompute on damage events only.
	PrimaryComponentTick.bCanEverTick = false;
}

void UBodyConditionComponent::BeginPlay()
{
	Super::BeginPlay();
	// Establish baseline cached senses (all 1.0 unless pre-damaged in editor).
	RecalculateDerivedStats();
}

// --- Hierarchy resolution -------------------------------------------------

const FBodyPartDefinition* UBodyConditionComponent::FindPartDef(EBodyPart Part) const
{
	if (IsValid(Hierarchy))
	{
		if (const FBodyPartDefinition* Def = Hierarchy->Parts.Find(Part))
		{
			return Def;
		}
	}
	return GetDefaultHierarchy().Find(Part);
}

const TMap<EBodyPart, FBodyPartDefinition>& UBodyConditionComponent::GetDefaultHierarchy()
{
	// Built once, shared by all components lacking an explicit asset.
	static TMap<EBodyPart, FBodyPartDefinition> Default;
	if (Default.Num() > 0)
	{
		return Default;
	}

	auto Add = [](EBodyPart Part, EBodyPart Parent, ESenseType Sense, float Weight)
	{
		FBodyPartDefinition Def;
		Def.Parent      = Parent;
		Def.FeedsSense  = Sense;
		Def.SenseWeight = Weight;
		Default.Add(Part, Def);
	};

	// Roots (parent = Body, the implicit 100% root).
	Add(EBodyPart::Head,  EBodyPart::Body, ESenseType::None, 0.0f);
	Add(EBodyPart::Torso, EBodyPart::Body, ESenseType::None, 0.0f);

	// Head senses.
	Add(EBodyPart::LeftEye,  EBodyPart::Head, ESenseType::Vision,  0.5f);
	Add(EBodyPart::RightEye, EBodyPart::Head, ESenseType::Vision,  0.5f);
	Add(EBodyPart::LeftEar,  EBodyPart::Head, ESenseType::Hearing, 0.5f);
	Add(EBodyPart::RightEar, EBodyPart::Head, ESenseType::Hearing, 0.5f);
	Add(EBodyPart::Tongue,   EBodyPart::Head, ESenseType::Speech,  1.0f);

	// Left arm chain — hand precision weights sum to 1.0, thumb dominant.
	Add(EBodyPart::LeftArm,    EBodyPart::Body,     ESenseType::None,              0.0f);
	Add(EBodyPart::LeftHand,   EBodyPart::LeftArm,  ESenseType::HandPrecisionLeft, 0.40f);
	Add(EBodyPart::LeftThumb,  EBodyPart::LeftHand, ESenseType::HandPrecisionLeft, 0.30f);
	Add(EBodyPart::LeftIndex,  EBodyPart::LeftHand, ESenseType::HandPrecisionLeft, 0.10f);
	Add(EBodyPart::LeftMiddle, EBodyPart::LeftHand, ESenseType::HandPrecisionLeft, 0.08f);
	Add(EBodyPart::LeftRing,   EBodyPart::LeftHand, ESenseType::HandPrecisionLeft, 0.07f);
	Add(EBodyPart::LeftPinky,  EBodyPart::LeftHand, ESenseType::HandPrecisionLeft, 0.05f);

	// Right arm chain.
	Add(EBodyPart::RightArm,    EBodyPart::Body,      ESenseType::None,               0.0f);
	Add(EBodyPart::RightHand,   EBodyPart::RightArm,  ESenseType::HandPrecisionRight, 0.40f);
	Add(EBodyPart::RightThumb,  EBodyPart::RightHand, ESenseType::HandPrecisionRight, 0.30f);
	Add(EBodyPart::RightIndex,  EBodyPart::RightHand, ESenseType::HandPrecisionRight, 0.10f);
	Add(EBodyPart::RightMiddle, EBodyPart::RightHand, ESenseType::HandPrecisionRight, 0.08f);
	Add(EBodyPart::RightRing,   EBodyPart::RightHand, ESenseType::HandPrecisionRight, 0.07f);
	Add(EBodyPart::RightPinky,  EBodyPart::RightHand, ESenseType::HandPrecisionRight, 0.05f);

	// Legs — mobility weights sum to 1.0.
	Add(EBodyPart::LeftLeg,   EBodyPart::Body,    ESenseType::Mobility, 0.35f);
	Add(EBodyPart::LeftFoot,  EBodyPart::LeftLeg, ESenseType::Mobility, 0.15f);
	Add(EBodyPart::RightLeg,  EBodyPart::Body,    ESenseType::Mobility, 0.35f);
	Add(EBodyPart::RightFoot, EBodyPart::RightLeg,ESenseType::Mobility, 0.15f);

	return Default;
}

// --- Queries --------------------------------------------------------------

float UBodyConditionComponent::GetPartHealth(EBodyPart Part) const
{
	const float* Found = PartHealth.Find(Part);
	return Found ? *Found : 1.0f; // absent = healthy
}

float UBodyConditionComponent::GetPartEffectiveHealth(EBodyPart Part) const
{
	const float Own = GetPartHealth(Part);

	// Body is the implicit root — never capped.
	if (Part == EBodyPart::Body)
	{
		return Own;
	}

	EBodyPart Parent = EBodyPart::Body;
	if (const FBodyPartDefinition* Def = FindPartDef(Part))
	{
		Parent = Def->Parent;
	}

	// Safety: malformed data (self-parent) must not infinitely recurse.
	if (Parent == Part)
	{
		return Own;
	}

	// Cascade: capped by the parent's effective health.
	return FMath::Min(Own, GetPartEffectiveHealth(Parent));
}

EInjuryType UBodyConditionComponent::GetPartInjury(EBodyPart Part) const
{
	const EInjuryType* Found = PartInjury.Find(Part);
	return Found ? *Found : EInjuryType::None;
}

FText UBodyConditionComponent::GetPartDisplayName(EBodyPart Part)
{
	// Names baked here (one place) so they survive cooking — UMETA DisplayName
	// is editor-only and would fall back to raw identifiers in a packaged build.
	switch (Part)
	{
	case EBodyPart::Body:        return NSLOCTEXT("Body", "Part_Body",        "Body");
	case EBodyPart::Head:        return NSLOCTEXT("Body", "Part_Head",        "Head");
	case EBodyPart::LeftEye:     return NSLOCTEXT("Body", "Part_LeftEye",     "Left Eye");
	case EBodyPart::RightEye:    return NSLOCTEXT("Body", "Part_RightEye",    "Right Eye");
	case EBodyPart::LeftEar:     return NSLOCTEXT("Body", "Part_LeftEar",     "Left Ear");
	case EBodyPart::RightEar:    return NSLOCTEXT("Body", "Part_RightEar",    "Right Ear");
	case EBodyPart::Tongue:      return NSLOCTEXT("Body", "Part_Tongue",      "Tongue");
	case EBodyPart::Torso:       return NSLOCTEXT("Body", "Part_Torso",       "Torso");
	case EBodyPart::LeftArm:     return NSLOCTEXT("Body", "Part_LeftArm",     "Left Arm");
	case EBodyPart::LeftHand:    return NSLOCTEXT("Body", "Part_LeftHand",    "Left Hand");
	case EBodyPart::LeftThumb:   return NSLOCTEXT("Body", "Part_LeftThumb",   "Left Thumb");
	case EBodyPart::LeftIndex:   return NSLOCTEXT("Body", "Part_LeftIndex",   "Left Index");
	case EBodyPart::LeftMiddle:  return NSLOCTEXT("Body", "Part_LeftMiddle",  "Left Middle");
	case EBodyPart::LeftRing:    return NSLOCTEXT("Body", "Part_LeftRing",    "Left Ring");
	case EBodyPart::LeftPinky:   return NSLOCTEXT("Body", "Part_LeftPinky",   "Left Pinky");
	case EBodyPart::RightArm:    return NSLOCTEXT("Body", "Part_RightArm",    "Right Arm");
	case EBodyPart::RightHand:   return NSLOCTEXT("Body", "Part_RightHand",   "Right Hand");
	case EBodyPart::RightThumb:  return NSLOCTEXT("Body", "Part_RightThumb",  "Right Thumb");
	case EBodyPart::RightIndex:  return NSLOCTEXT("Body", "Part_RightIndex",  "Right Index");
	case EBodyPart::RightMiddle: return NSLOCTEXT("Body", "Part_RightMiddle", "Right Middle");
	case EBodyPart::RightRing:   return NSLOCTEXT("Body", "Part_RightRing",   "Right Ring");
	case EBodyPart::RightPinky:  return NSLOCTEXT("Body", "Part_RightPinky",  "Right Pinky");
	case EBodyPart::LeftLeg:     return NSLOCTEXT("Body", "Part_LeftLeg",     "Left Leg");
	case EBodyPart::LeftFoot:    return NSLOCTEXT("Body", "Part_LeftFoot",    "Left Foot");
	case EBodyPart::RightLeg:    return NSLOCTEXT("Body", "Part_RightLeg",    "Right Leg");
	case EBodyPart::RightFoot:   return NSLOCTEXT("Body", "Part_RightFoot",   "Right Foot");
	default:                     return FText::GetEmpty();
	}
}

// --- Derived senses -------------------------------------------------------

float UBodyConditionComponent::ComputeSense(ESenseType Sense) const
{
	const TMap<EBodyPart, FBodyPartDefinition>& Source =
		IsValid(Hierarchy) ? Hierarchy->Parts : GetDefaultHierarchy();

	float Weighted = 0.0f;
	float TotalW   = 0.0f;

	for (const TPair<EBodyPart, FBodyPartDefinition>& Pair : Source)
	{
		if (Pair.Value.FeedsSense == Sense && Pair.Value.SenseWeight > 0.0f)
		{
			Weighted += Pair.Value.SenseWeight * GetPartEffectiveHealth(Pair.Key);
			TotalW   += Pair.Value.SenseWeight;
		}
	}

	return (TotalW > 0.0f) ? (Weighted / TotalW) : 1.0f;
}

void UBodyConditionComponent::RecalculateDerivedStats()
{
	VisionAcuity		= ComputeSense(ESenseType::Vision);
	HearingAcuity		= ComputeSense(ESenseType::Hearing);
	SpeechClarity		= ComputeSense(ESenseType::Speech);
	HandPrecisionLeft	= ComputeSense(ESenseType::HandPrecisionLeft);
	HandPrecisionRight	= ComputeSense(ESenseType::HandPrecisionRight);
	Mobility			= ComputeSense(ESenseType::Mobility);

	UE_LOG(LogBody, Verbose,
		TEXT("[%s] senses: Vis=%.2f Hear=%.2f Speech=%.2f HandL=%.2f HandR=%.2f Mob=%.2f"),
		*GetNameSafe(GetOwner()),
		VisionAcuity, HearingAcuity, SpeechClarity,
		HandPrecisionLeft, HandPrecisionRight, Mobility);
}

// --- Mutations ------------------------------------------------------------

void UBodyConditionComponent::ApplyDamage(EBodyPart Part, float Amount, EInjuryType Injury)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	const float NewHealth = FMath::Clamp(GetPartHealth(Part) - Amount, 0.0f, 1.0f);
	PartHealth.Add(Part, NewHealth);

	if (Injury != EInjuryType::None)
	{
		PartInjury.Add(Part, Injury);
	}

	UE_LOG(LogBody, Log, TEXT("[%s] %s took %.2f damage -> %.2f (injury %d)."),
		*GetNameSafe(GetOwner()), *UEnum::GetValueAsString(Part),
		Amount, NewHealth, static_cast<int32>(Injury));

	RecalculateDerivedStats();
	OnBodyChanged();
}

void UBodyConditionComponent::HealPart(EBodyPart Part, float Amount)
{
	if (Amount <= 0.0f || !PartHealth.Contains(Part))
	{
		return;
	}

	const float NewHealth = FMath::Clamp(GetPartHealth(Part) + Amount, 0.0f, 1.0f);

	if (NewHealth >= 1.0f)
	{
		// Fully healed — drop from the sparse maps (back to implicit healthy).
		PartHealth.Remove(Part);
		PartInjury.Remove(Part);
	}
	else
	{
		PartHealth.Add(Part, NewHealth);
	}

	UE_LOG(LogBody, Log, TEXT("[%s] %s healed to %.2f."),
		*GetNameSafe(GetOwner()), *UEnum::GetValueAsString(Part), NewHealth);

	RecalculateDerivedStats();
	OnBodyChanged();
}
