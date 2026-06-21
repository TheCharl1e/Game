#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSCameraPawn.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * Classic RTS floating camera pawn.
 *
 * Controls:
 *   WASD              — translate in world XY. Z is STRICTLY locked during movement.
 *   Q / E             — yaw rotation. Remap freely in IMC_RTS — code only sees +1 / -1.
 *   Scroll wheel      — zoom along world Z axis. Hard-clamped to MinHeightAboveGround
 *                       (~3m) above the actual terrain via a downward line trace.
 *   MMB hold + drag   — drag-pan in world XY. Pan speed scales with camera height
 *                       so it feels consistent at any zoom level.
 *
 * NOTE ON TICK: this pawn intentionally uses Tick. It is a SINGLE instance (player
 * camera). The "no Event Tick" rule in CLAUDE.md targets the 500+ NPC simulation,
 * not one-off player-controlled actors that require per-frame smooth movement.
 *
 * SETUP in editor:
 *   1. Create a Blueprint child of this class (e.g. BP_RTSCamera).
 *   2. Assign all IA_ and RTSMappingContext assets in Class Defaults.
 *   3. Create IMC_RTS: bind WASD → IA_RTSMove (Vector2D), Q→-1 / E→+1 → IA_RTSRotate
 *      (float), Scroll → IA_RTSZoom (float), MMB → IA_RTSPanToggle (bool),
 *      Mouse XY → IA_RTSPan (Vector2D).
 *   4. Set GM_RTSGameMode DefaultPawnClass to your BP.
 */
UCLASS()
class STAN_PIERWOTNY_API ARTSCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ARTSCameraPawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:
	// --- Components -------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Components")
	TObjectPtr<USceneComponent> CameraRoot;

	/** Camera attached directly to root, pitched down at CameraPitch degrees. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Components")
	TObjectPtr<UCameraComponent> Camera;

	// --- Enhanced Input assets (assign in BP class defaults) --------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RTS|Input")
	TObjectPtr<UInputMappingContext> RTSMappingContext;

	/** WASD → Vector2D (X = strafe, Y = forward). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RTS|Input")
	TObjectPtr<UInputAction> IA_RTSMove;

	/** Rotation keys → float (+1 / -1). Remap Q/E freely in IMC. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RTS|Input")
	TObjectPtr<UInputAction> IA_RTSRotate;

	/** Scroll wheel → float. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RTS|Input")
	TObjectPtr<UInputAction> IA_RTSZoom;

	/** MMB press/release → bool. Activates drag-pan mode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RTS|Input")
	TObjectPtr<UInputAction> IA_RTSPanToggle;

	/** Mouse XY → Vector2D. Read only when bIsPanning is true. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RTS|Input")
	TObjectPtr<UInputAction> IA_RTSPan;

	// --- Tunable settings (all exposed to editor / BP) --------------------

	/** WASD translation speed in UU/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float MoveSpeed = 1500.0f;

	/** Yaw rotation speed in degrees/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float RotationSpeed = 80.0f;

	/** Height change in UU per scroll click. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float ZoomStep = 350.0f;

	/** Lerp speed for smooth zoom. Higher = snappier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float ZoomInterpSpeed = 8.0f;

	/** Absolute minimum Z height (fallback if ground trace misses). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float MinCameraZ = 300.0f;

	/** 3 m above terrain (in UU, 1 UU ≈ 1 cm → 300 UU = 3 m). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float MinHeightAboveGround = 300.0f;

	/** Maximum camera altitude. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float MaxCameraZ = 10000.0f;

	/** Fixed look-down pitch angle (negative = looking down). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float CameraPitch = -50.0f;

	/** Pan sensitivity. Scales automatically with camera height. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float PanSensitivity = 1.0f;

	/** Reference height for pan speed normalisation (matches MoveSpeed feel). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float PanReferenceZ = 2000.0f;

	/** Max distance for the downward ground-detection trace. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings")
	float GroundTraceLength = 50000.0f;

	/** Optional: clamp pawn position to a rectangular play area. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings|Bounds")
	bool bClampToWorldBounds = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings|Bounds",
		meta = (EditCondition = "bClampToWorldBounds"))
	FVector2D WorldBoundsMin = FVector2D(-50000.0f, -50000.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTS|Settings|Bounds",
		meta = (EditCondition = "bClampToWorldBounds"))
	FVector2D WorldBoundsMax = FVector2D(50000.0f, 50000.0f);

private:
	// Runtime state — no UPROPERTY needed (no serialisation, no GC interaction).
	FVector2D MoveInput    = FVector2D::ZeroVector;
	float     RotateInput  = 0.0f;
	float     TargetZ      = 2000.0f;    // smooth zoom target
	bool      bIsPanning   = false;

	// --- Input callbacks --------------------------------------------------
	void OnMove(const FInputActionValue& Value);
	void OnMoveCompleted(const FInputActionValue& Value);
	void OnRotate(const FInputActionValue& Value);
	void OnRotateCompleted(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnPanToggle(const FInputActionValue& Value);
	void OnPan(const FInputActionValue& Value);

	// --- Helpers ----------------------------------------------------------

	/** Returns the minimum allowed Z for the camera at its current XY position.
	 *  Traces downward to find terrain, adds MinHeightAboveGround.
	 *  Only called on scroll events — never per-frame. */
	float GetTerrainMinZ() const;

	/** Clamp actor XY to WorldBounds if enabled. Z is left untouched. */
	void ClampPositionToBounds();
};
