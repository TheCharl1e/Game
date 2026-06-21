#include "RTSCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

ARTSCameraPawn::ARTSCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true; // intentional — single camera instance

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	SetRootComponent(CameraRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraRoot);
	Camera->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
}

void ARTSCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	// Initialise smooth-zoom target to current spawn height.
	TargetZ = GetActorLocation().Z;

	// Register our Input Mapping Context with the Enhanced Input subsystem.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (IsValid(RTSMappingContext))
			{
				Sub->AddMappingContext(RTSMappingContext, 0);
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RTSCamera] RTSMappingContext not assigned in class defaults!"));
			}
		}
	}
}

void ARTSCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- 1. WASD — translate in world XY, Z strictly locked ---------------
	if (!MoveInput.IsNearlyZero())
	{
		// Project movement along current yaw so WASD is always camera-relative.
		const FRotator YawOnly(0.0f, GetActorRotation().Yaw, 0.0f);
		const FVector  Forward = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
		const FVector  Right   = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

		FVector Delta = (Forward * MoveInput.Y + Right * MoveInput.X)
			* MoveSpeed * DeltaTime;
		Delta.Z = 0.0f; // absolute XY lock

		AddActorWorldOffset(Delta, /*bSweep*/ true);
	}

	// --- 2. Q/E — yaw rotation (keys mapped in IMC, code sees +1/-1) ------
	if (!FMath::IsNearlyZero(RotateInput))
	{
		AddActorWorldRotation(
			FRotator(0.0f, RotateInput * RotationSpeed * DeltaTime, 0.0f));
	}

	// --- 3. Smooth zoom — interpolate current Z toward TargetZ ------------
	FVector Pos = GetActorLocation();
	Pos.Z = FMath::FInterpTo(Pos.Z, TargetZ, DeltaTime, ZoomInterpSpeed);
	SetActorLocation(Pos);

	// --- 4. Optional world-bounds clamp -----------------------------------
	if (bClampToWorldBounds)
	{
		ClampPositionToBounds();
	}
}

void ARTSCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!IsValid(EIC))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[RTSCamera] EnhancedInputComponent missing. "
				 "Check Project Settings → Input → Default Input Component Class."));
		return;
	}

	// Bind with IsValid guards — missing assets log a warning but don't crash.
	auto Bind = [&](UInputAction* Action, ETriggerEvent Event, auto Func)
	{
		if (IsValid(Action)) EIC->BindAction(Action, Event, this, Func);
		else UE_LOG(LogTemp, Warning, TEXT("[RTSCamera] InputAction not assigned: %s"),
			*GetNameSafe(Action));
	};

	Bind(IA_RTSMove,      ETriggerEvent::Triggered,  &ARTSCameraPawn::OnMove);
	Bind(IA_RTSMove,      ETriggerEvent::Completed,   &ARTSCameraPawn::OnMoveCompleted);
	Bind(IA_RTSRotate,    ETriggerEvent::Triggered,  &ARTSCameraPawn::OnRotate);
	Bind(IA_RTSRotate,    ETriggerEvent::Completed,   &ARTSCameraPawn::OnRotateCompleted);
	Bind(IA_RTSZoom,      ETriggerEvent::Triggered,  &ARTSCameraPawn::OnZoom);
	Bind(IA_RTSPanToggle, ETriggerEvent::Started,    &ARTSCameraPawn::OnPanToggle);
	Bind(IA_RTSPanToggle, ETriggerEvent::Completed,   &ARTSCameraPawn::OnPanToggle);
	Bind(IA_RTSPan,       ETriggerEvent::Triggered,  &ARTSCameraPawn::OnPan);
}

// --- Input callbacks ------------------------------------------------------

void ARTSCameraPawn::OnMove(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
}

void ARTSCameraPawn::OnMoveCompleted(const FInputActionValue& Value)
{
	MoveInput = FVector2D::ZeroVector;
}

void ARTSCameraPawn::OnRotate(const FInputActionValue& Value)
{
	RotateInput = Value.Get<float>();
}

void ARTSCameraPawn::OnRotateCompleted(const FInputActionValue& Value)
{
	RotateInput = 0.0f;
}

void ARTSCameraPawn::OnZoom(const FInputActionValue& Value)
{
	const float ScrollDelta = Value.Get<float>();

	// Ground trace fires here (on input event), not in Tick. One trace per
	// scroll click — cheap. For 500+ NPC scenes the terrain is mostly static.
	const float TerrainMinZ = GetTerrainMinZ();

	TargetZ = FMath::Clamp(
		TargetZ - ScrollDelta * ZoomStep,
		TerrainMinZ,
		MaxCameraZ);

	UE_LOG(LogTemp, Verbose,
		TEXT("[RTSCamera] Zoom → TargetZ=%.0f (terrain min=%.0f)"),
		TargetZ, TerrainMinZ);
}

void ARTSCameraPawn::OnPanToggle(const FInputActionValue& Value)
{
	bIsPanning = Value.Get<bool>();

	// Show / hide the cursor so the player knows they're in pan mode.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetShowMouseCursor(!bIsPanning);
	}
}

void ARTSCameraPawn::OnPan(const FInputActionValue& Value)
{
	if (!bIsPanning) return;

	const FVector2D MouseDelta = Value.Get<FVector2D>();
	if (MouseDelta.IsNearlyZero()) return;

	// Scale pan speed by current height so dragging 1 cm of mouse always moves
	// the same perceived distance on the map regardless of zoom level.
	const float HeightRatio = GetActorLocation().Z / FMath::Max(PanReferenceZ, 1.0f);

	const FRotator YawOnly(0.0f, GetActorRotation().Yaw, 0.0f);
	const FVector  Right   = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);
	const FVector  Forward = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);

	// Invert delta: dragging right grabs the map and pulls it right (camera goes left).
	FVector Delta = (-Right * MouseDelta.X + -Forward * MouseDelta.Y)
		* PanSensitivity * HeightRatio;
	Delta.Z = 0.0f;

	AddActorWorldOffset(Delta, /*bSweep*/ true);
}

// --- Helpers --------------------------------------------------------------

float ARTSCameraPawn::GetTerrainMinZ() const
{
	// Trace straight down from well above the camera's XY position.
	const FVector XY    = GetActorLocation();
	const FVector Start = FVector(XY.X, XY.Y, XY.Z + 5000.0f);
	const FVector End   = Start - FVector(0.0f, 0.0f, GroundTraceLength);

	FHitResult            Hit;
	FCollisionQueryParams Params(TEXT("RTSGroundTrace"), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(this);

	if (GetWorld() &&
		GetWorld()->LineTraceSingleByChannel(Hit, Start, End,
			ECC_WorldStatic, Params))
	{
		return Hit.ImpactPoint.Z + MinHeightAboveGround;
	}

	// Fallback: no terrain hit (edge of map, flying over void).
	return MinCameraZ;
}

void ARTSCameraPawn::ClampPositionToBounds()
{
	FVector Pos = GetActorLocation();
	Pos.X = FMath::Clamp(Pos.X, WorldBoundsMin.X, WorldBoundsMax.X);
	Pos.Y = FMath::Clamp(Pos.Y, WorldBoundsMin.Y, WorldBoundsMax.Y);
	// Z untouched — handled by zoom clamping.
	SetActorLocation(Pos);
}
