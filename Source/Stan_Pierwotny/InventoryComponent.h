#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "InventoryComponent.generated.h"

// Dedicated log category for inventory / ownership / theft / equipment events.
DECLARE_LOG_CATEGORY_EXTERN(LogInventory, Log, All);

// Sentinel owner ID for public containers (village storehouse / "anthill").
#define PUBLIC_OWNER_ID -1

/** High-level category of an item. Drives Maslow-level behaviour selection. */
UENUM(BlueprintType)
enum class EItemType : uint8
{
	None		UMETA(DisplayName = "None"),
	Food		UMETA(DisplayName = "Food"),
	Resource	UMETA(DisplayName = "Resource"),
	Tool		UMETA(DisplayName = "Tool"),
	Clothing	UMETA(DisplayName = "Clothing"),
	Luxury		UMETA(DisplayName = "Luxury")
};

/** Equipment slots. Hands always exist (naked). Body slots hold clothing.
 *  Items worn in Back/Rig/Torso/Legs may each open their OWN storage compartment. */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None		UMETA(DisplayName = "None / Body Pockets"),
	LeftHand	UMETA(DisplayName = "Left Hand"),
	RightHand	UMETA(DisplayName = "Right Hand"),
	Head		UMETA(DisplayName = "Head"),
	Torso		UMETA(DisplayName = "Torso"),
	Legs		UMETA(DisplayName = "Legs"),
	Feet		UMETA(DisplayName = "Feet"),
	Back		UMETA(DisplayName = "Back (Backpack)"),
	Rig			UMETA(DisplayName = "Rig (Vest)")
};

/**
 * Static, shared item data. ONE row per item type in a UDataTable.
 * Never duplicated per-NPC.
 */
USTRUCT(BlueprintType)
struct FItemDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Core")
	EItemType Type = EItemType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Core", meta = (ClampMin = "0.0"))
	float UnitWeight = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Economy", meta = (ClampMin = "0"))
	int32 BaseValue = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Food")
	float NutritionValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Food")
	bool bIsPerishable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Core", meta = (ClampMin = "1"))
	int32 MaxStackSize = 99;

	// --- Storage / equipment ---

	/** How many slots this item occupies inside a compartment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Storage", meta = (ClampMin = "1"))
	int32 SlotSize = 1;

	/** If this item is a wearable container: number of slots its OWN compartment
	 *  has when equipped. 0 = provides no storage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Storage", meta = (ClampMin = "0"))
	int32 ContainerCapacity = 0;

	/** Which equipment slot this item can be equipped to. None = not equippable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Storage")
	EEquipmentSlot ValidEquipSlot = EEquipmentSlot::None;
};

/** Runtime payload: an item ID + count. */
USTRUCT(BlueprintType)
struct FItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemStack")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemStack")
	int32 Quantity = 0;

	FItemStack() {}
	FItemStack(FName InID, int32 InQty) : ItemID(InID), Quantity(InQty) {}
};

/**
 * A discrete storage space. Provided either by the body itself (innate pockets,
 * SourceSlot == None) or by an equipped container item (backpack on Back, etc.).
 * Items live INSIDE a specific compartment — removing the source gear removes
 * the compartment (and, by design, only when it is empty).
 */
USTRUCT(BlueprintType)
struct FStorageCompartment
{
	GENERATED_BODY()

	/** Equipment slot whose worn item provides this storage.
	 *  None = innate body pockets (always present). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Compartment")
	EEquipmentSlot SourceSlot = EEquipmentSlot::None;

	/** HUD label ("Pockets", "Backpack", "Trousers", "Tunic"). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Compartment")
	FName Label = NAME_None;

	/** Total slot capacity of THIS compartment. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Compartment")
	int32 Capacity = 0;

	/** Items stored in this compartment. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Compartment")
	TArray<FItemStack> Contents;
};

/**
 * Inventory with ownership + per-container compartment storage (Tarkov-style).
 * Pure logic, no Tick. Visual reactions via OnInventoryChanged (Blueprint).
 */
UCLASS(ClassGroup = (NPC), meta = (BlueprintSpawnableComponent))
class STAN_PIERWOTNY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

public:
	// --- Ownership --------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Ownership")
	int32 OwnerID = PUBLIC_OWNER_ID;

	UFUNCTION(BlueprintPure, Category = "Inventory|Ownership")
	bool IsPublic() const { return OwnerID == PUBLIC_OWNER_ID; }

	// --- Data -------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Data")
	TObjectPtr<UDataTable> ItemDefinitions = nullptr;

	const FItemDefinition* FindDefinition(FName ItemID) const;

	// --- Compartment configuration ----------------------------------------

	/** Innate body pocket capacity, present even when naked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Storage", meta = (ClampMin = "0"))
	int32 BasePocketCapacity = 2;

	/** All active compartments (index 0 is always the innate pockets). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Storage")
	TArray<FStorageCompartment> Compartments;

	/** Worn gear keyed by slot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Equipment")
	TMap<EEquipmentSlot, FItemStack> EquippedItems;

	// --- Global queries ---------------------------------------------------

	/** Total quantity of an item across ALL compartments. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	int32 GetQuantity(FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	bool HasItem(FName ItemID, int32 MinQuantity = 1) const;

	/** Weight of everything carried (all compartments + worn gear). */
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	int32 GetTotalCapacity() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	int32 GetTotalUsedSlots() const;

	/** Copy of all compartments for HUD rendering (separate grids per container). */
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	TArray<FStorageCompartment> GetCompartments() const { return Compartments; }

	// --- Auto-distributing mutations (NPC logic) --------------------------

	/** Add items, auto-distributed across compartments. Returns amount added. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Mutate")
	int32 AddItem(FName ItemID, int32 Amount);

	/** Remove items, pulled from any compartment. Returns amount removed. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Mutate")
	int32 RemoveItem(FName ItemID, int32 Amount);

	// --- Targeted mutations (player drag & drop) --------------------------

	/** Add into a SPECIFIC compartment (by its SourceSlot). Returns amount added. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Mutate")
	int32 AddItemToCompartment(EEquipmentSlot CompartmentSlot, FName ItemID, int32 Amount);

	/** Remove from a SPECIFIC compartment. Returns amount removed. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Mutate")
	int32 RemoveItemFromCompartment(EEquipmentSlot CompartmentSlot, FName ItemID, int32 Amount);

	// --- Equipment --------------------------------------------------------

	/** Equip an item from storage into a slot. If it is a container, opens its
	 *  own compartment. Returns success. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	bool EquipItem(FName ItemID, EEquipmentSlot Slot);

	/** Unequip a slot back into storage. A container slot must be EMPTY first
	 *  (v1 rule). Returns success. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	bool UnequipSlot(EEquipmentSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	FName GetEquippedItem(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	bool IsSlotOccupied(EEquipmentSlot Slot) const;

	// --- Ownership-aware access (feeds the L5 detective system) -----------

	UFUNCTION(BlueprintCallable, Category = "Inventory|Ownership")
	int32 TryWithdraw(int32 RequesterID, FName ItemID, int32 Amount, bool& bWasUnauthorized);

protected:
	// --- Internal helpers -------------------------------------------------

	/** Ensure the innate pockets compartment exists at index 0. */
	void EnsurePocketsCompartment();

	/** Index into Compartments for a given SourceSlot, or INDEX_NONE. */
	int32 FindCompartmentIndex(EEquipmentSlot SourceSlot) const;

	/** Slots used inside one compartment (sum of SlotSize over contents). */
	int32 GetCompartmentUsedSlots(const FStorageCompartment& Comp) const;

	/** Add into a single compartment. Returns amount added. */
	int32 AddToCompartment(FStorageCompartment& Comp, FName ItemID, const FItemDefinition* Def, int32 Amount);

	// --- Visual / BP layer hook -------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Events")
	void OnInventoryChanged();
};
