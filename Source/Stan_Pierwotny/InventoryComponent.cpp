#include "InventoryComponent.h"

DEFINE_LOG_CATEGORY(LogInventory);

UInventoryComponent::UInventoryComponent()
{
	// HARD RULE: no Event Tick. Inventory mutates on discrete actions only.
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsurePocketsCompartment();

	if (!IsValid(ItemDefinitions))
	{
		UE_LOG(LogInventory, Warning,
			TEXT("[%s] OwnerID=%d has NO ItemDefinitions table assigned."),
			*GetNameSafe(GetOwner()), OwnerID);
	}
}

// --- Definitions ----------------------------------------------------------

const FItemDefinition* UInventoryComponent::FindDefinition(FName ItemID) const
{
	if (!IsValid(ItemDefinitions) || ItemID.IsNone())
	{
		return nullptr;
	}
	static const FString Context(TEXT("UInventoryComponent::FindDefinition"));
	return ItemDefinitions->FindRow<FItemDefinition>(ItemID, Context, false);
}

// --- Compartment helpers --------------------------------------------------

void UInventoryComponent::EnsurePocketsCompartment()
{
	if (FindCompartmentIndex(EEquipmentSlot::None) == INDEX_NONE)
	{
		FStorageCompartment Pockets;
		Pockets.SourceSlot = EEquipmentSlot::None;
		Pockets.Label      = TEXT("Pockets");
		Pockets.Capacity   = BasePocketCapacity;
		Compartments.Insert(Pockets, 0); // pockets always index 0
	}
}

int32 UInventoryComponent::FindCompartmentIndex(EEquipmentSlot SourceSlot) const
{
	for (int32 i = 0; i < Compartments.Num(); ++i)
	{
		if (Compartments[i].SourceSlot == SourceSlot)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UInventoryComponent::GetCompartmentUsedSlots(const FStorageCompartment& Comp) const
{
	int32 Used = 0;
	for (const FItemStack& Stack : Comp.Contents)
	{
		const FItemDefinition* Def = FindDefinition(Stack.ItemID);
		Used += (Def) ? Def->SlotSize : 1;
	}
	return Used;
}

int32 UInventoryComponent::AddToCompartment(FStorageCompartment& Comp, FName ItemID,
	const FItemDefinition* Def, int32 Amount)
{
	if (!Def || Amount <= 0)
	{
		return 0;
	}

	int32 Remaining = Amount;

	// 1. Top up an existing stack of the same item, if present.
	for (FItemStack& Stack : Comp.Contents)
	{
		if (Stack.ItemID == ItemID)
		{
			const int32 Room = FMath::Max(0, Def->MaxStackSize - Stack.Quantity);
			const int32 Add  = FMath::Min(Remaining, Room);
			Stack.Quantity += Add;
			Remaining      -= Add;
			break; // one stack per item per compartment in this model
		}
	}

	// 2. Place a NEW stack if there's room AND a free slot for its footprint.
	if (Remaining > 0)
	{
		const int32 FreeSlots = Comp.Capacity - GetCompartmentUsedSlots(Comp);
		if (FreeSlots >= Def->SlotSize)
		{
			const int32 NewStackQty = FMath::Min(Remaining, Def->MaxStackSize);
			Comp.Contents.Emplace(ItemID, NewStackQty);
			Remaining -= NewStackQty;
		}
	}

	return Amount - Remaining;
}

// --- Global queries -------------------------------------------------------

int32 UInventoryComponent::GetQuantity(FName ItemID) const
{
	int32 Total = 0;
	for (const FStorageCompartment& Comp : Compartments)
	{
		for (const FItemStack& Stack : Comp.Contents)
		{
			if (Stack.ItemID == ItemID)
			{
				Total += Stack.Quantity;
			}
		}
	}
	return Total;
}

bool UInventoryComponent::HasItem(FName ItemID, int32 MinQuantity) const
{
	return GetQuantity(ItemID) >= FMath::Max(1, MinQuantity);
}

float UInventoryComponent::GetTotalWeight() const
{
	float Total = 0.0f;

	for (const FStorageCompartment& Comp : Compartments)
	{
		for (const FItemStack& Stack : Comp.Contents)
		{
			if (const FItemDefinition* Def = FindDefinition(Stack.ItemID))
			{
				Total += Def->UnitWeight * Stack.Quantity;
			}
		}
	}

	for (const TPair<EEquipmentSlot, FItemStack>& Pair : EquippedItems)
	{
		if (const FItemDefinition* Def = FindDefinition(Pair.Value.ItemID))
		{
			Total += Def->UnitWeight * Pair.Value.Quantity;
		}
	}

	return Total;
}

int32 UInventoryComponent::GetTotalCapacity() const
{
	int32 Total = 0;
	for (const FStorageCompartment& Comp : Compartments)
	{
		Total += Comp.Capacity;
	}
	return Total;
}

int32 UInventoryComponent::GetTotalUsedSlots() const
{
	int32 Total = 0;
	for (const FStorageCompartment& Comp : Compartments)
	{
		Total += GetCompartmentUsedSlots(Comp);
	}
	return Total;
}

// --- Auto-distributing mutations ------------------------------------------

int32 UInventoryComponent::AddItem(FName ItemID, int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const FItemDefinition* Def = FindDefinition(ItemID);
	if (!Def)
	{
		UE_LOG(LogInventory, Warning, TEXT("[%s] AddItem rejected: '%s' has no definition."),
			*GetNameSafe(GetOwner()), *ItemID.ToString());
		return 0;
	}

	int32 Remaining = Amount;
	for (FStorageCompartment& Comp : Compartments)
	{
		if (Remaining <= 0) break;
		Remaining -= AddToCompartment(Comp, ItemID, Def, Remaining);
	}

	const int32 Added = Amount - Remaining;
	if (Added > 0)
	{
		UE_LOG(LogInventory, Log, TEXT("[%s] +%d '%s' (auto-distributed)."),
			*GetNameSafe(GetOwner()), Added, *ItemID.ToString());
		OnInventoryChanged();
	}
	else
	{
		UE_LOG(LogInventory, Verbose, TEXT("[%s] AddItem '%s': no room (%d/%d slots)."),
			*GetNameSafe(GetOwner()), *ItemID.ToString(), GetTotalUsedSlots(), GetTotalCapacity());
	}
	return Added;
}

int32 UInventoryComponent::RemoveItem(FName ItemID, int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	int32 Remaining = Amount;
	for (FStorageCompartment& Comp : Compartments)
	{
		if (Remaining <= 0) break;

		for (int32 i = Comp.Contents.Num() - 1; i >= 0 && Remaining > 0; --i)
		{
			if (Comp.Contents[i].ItemID != ItemID) continue;

			const int32 Take = FMath::Min(Remaining, Comp.Contents[i].Quantity);
			Comp.Contents[i].Quantity -= Take;
			Remaining                 -= Take;

			if (Comp.Contents[i].Quantity <= 0)
			{
				Comp.Contents.RemoveAtSwap(i);
			}
		}
	}

	const int32 Removed = Amount - Remaining;
	if (Removed > 0)
	{
		UE_LOG(LogInventory, Log, TEXT("[%s] -%d '%s'."),
			*GetNameSafe(GetOwner()), Removed, *ItemID.ToString());
		OnInventoryChanged();
	}
	return Removed;
}

// --- Targeted mutations ---------------------------------------------------

int32 UInventoryComponent::AddItemToCompartment(EEquipmentSlot CompartmentSlot, FName ItemID, int32 Amount)
{
	const int32 Index = FindCompartmentIndex(CompartmentSlot);
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogInventory, Warning, TEXT("[%s] AddItemToCompartment: no compartment for slot %d."),
			*GetNameSafe(GetOwner()), static_cast<int32>(CompartmentSlot));
		return 0;
	}

	const FItemDefinition* Def = FindDefinition(ItemID);
	if (!Def)
	{
		return 0;
	}

	const int32 Added = AddToCompartment(Compartments[Index], ItemID, Def, Amount);
	if (Added > 0)
	{
		OnInventoryChanged();
	}
	return Added;
}

int32 UInventoryComponent::RemoveItemFromCompartment(EEquipmentSlot CompartmentSlot, FName ItemID, int32 Amount)
{
	const int32 Index = FindCompartmentIndex(CompartmentSlot);
	if (Index == INDEX_NONE || Amount <= 0)
	{
		return 0;
	}

	FStorageCompartment& Comp = Compartments[Index];
	int32 Remaining = Amount;

	for (int32 i = Comp.Contents.Num() - 1; i >= 0 && Remaining > 0; --i)
	{
		if (Comp.Contents[i].ItemID != ItemID) continue;

		const int32 Take = FMath::Min(Remaining, Comp.Contents[i].Quantity);
		Comp.Contents[i].Quantity -= Take;
		Remaining                 -= Take;

		if (Comp.Contents[i].Quantity <= 0)
		{
			Comp.Contents.RemoveAtSwap(i);
		}
	}

	const int32 Removed = Amount - Remaining;
	if (Removed > 0)
	{
		OnInventoryChanged();
	}
	return Removed;
}

// --- Equipment ------------------------------------------------------------

bool UInventoryComponent::IsSlotOccupied(EEquipmentSlot Slot) const
{
	return EquippedItems.Contains(Slot);
}

FName UInventoryComponent::GetEquippedItem(EEquipmentSlot Slot) const
{
	const FItemStack* Found = EquippedItems.Find(Slot);
	return (Found) ? Found->ItemID : NAME_None;
}

float UInventoryComponent::GetTotalEquippedInsulation() const
{
	// L1-09a: suma Insulation po WSZYSTKICH założonych itemach (bez wag per-slot — decyzja dyrektora).
	float Total = 0.0f;
	for (const TPair<EEquipmentSlot, FItemStack>& Pair : EquippedItems)
	{
		if (const FItemDefinition* Def = FindDefinition(Pair.Value.ItemID))
		{
			Total += Def->Insulation;
		}
	}
	return Total;
}

bool UInventoryComponent::EquipItem(FName ItemID, EEquipmentSlot Slot)
{
	const FItemDefinition* Def = FindDefinition(ItemID);
	if (!Def)
	{
		return false;
	}

	if (Def->ValidEquipSlot != Slot)
	{
		UE_LOG(LogInventory, Warning, TEXT("[%s] EquipItem '%s': wrong slot."),
			*GetNameSafe(GetOwner()), *ItemID.ToString());
		return false;
	}

	if (!HasItem(ItemID, 1))
	{
		return false;
	}

	// Free the target slot first (must succeed, e.g. empty container rule).
	if (IsSlotOccupied(Slot) && !UnequipSlot(Slot))
	{
		return false;
	}

	// Move 1 unit from storage into the equipment slot.
	if (RemoveItem(ItemID, 1) < 1)
	{
		return false;
	}
	EquippedItems.Add(Slot, FItemStack(ItemID, 1));

	// If this gear is a container, open its own compartment.
	if (Def->ContainerCapacity > 0 && FindCompartmentIndex(Slot) == INDEX_NONE)
	{
		FStorageCompartment NewComp;
		NewComp.SourceSlot = Slot;
		NewComp.Label      = ItemID;
		NewComp.Capacity   = Def->ContainerCapacity;
		Compartments.Add(NewComp);

		UE_LOG(LogInventory, Log, TEXT("[%s] equipped container '%s' (+%d slots)."),
			*GetNameSafe(GetOwner()), *ItemID.ToString(), Def->ContainerCapacity);
	}
	else
	{
		UE_LOG(LogInventory, Log, TEXT("[%s] equipped '%s' to slot %d."),
			*GetNameSafe(GetOwner()), *ItemID.ToString(), static_cast<int32>(Slot));
	}

	OnInventoryChanged();
	return true;
}

bool UInventoryComponent::UnequipSlot(EEquipmentSlot Slot)
{
	const FItemStack* Equipped = EquippedItems.Find(Slot);
	if (!Equipped)
	{
		return false;
	}

	const FItemStack       Item = *Equipped;
	const FItemDefinition* Def  = FindDefinition(Item.ItemID);

	// VALIDATE FIRST — no state is mutated until every check passes.
	const int32 CompIndex     = (Def && Def->ContainerCapacity > 0)
		? FindCompartmentIndex(Slot) : INDEX_NONE;
	const int32 ItemSlotSize  = (Def) ? Def->SlotSize : 1;
	const int32 LostCapacity  = (CompIndex != INDEX_NONE) ? Compartments[CompIndex].Capacity : 0;

	// v1 rule: a container must be EMPTY before it can be removed.
	if (CompIndex != INDEX_NONE && Compartments[CompIndex].Contents.Num() > 0)
	{
		UE_LOG(LogInventory, Warning,
			TEXT("[%s] UnequipSlot %d refused: container not empty."),
			*GetNameSafe(GetOwner()), static_cast<int32>(Slot));
		return false;
	}

	// The worn item must have room to return to storage AFTER its own
	// (empty) compartment is removed.
	const int32 FreeAfterRemoval = (GetTotalCapacity() - LostCapacity) - GetTotalUsedSlots();
	if (FreeAfterRemoval < ItemSlotSize)
	{
		UE_LOG(LogInventory, Warning,
			TEXT("[%s] UnequipSlot %d refused: no room to stow '%s'."),
			*GetNameSafe(GetOwner()), static_cast<int32>(Slot), *Item.ItemID.ToString());
		return false;
	}

	// All checks passed — now mutate.
	if (CompIndex != INDEX_NONE)
	{
		Compartments.RemoveAt(CompIndex);
	}
	EquippedItems.Remove(Slot);
	AddItem(Item.ItemID, Item.Quantity);

	UE_LOG(LogInventory, Log, TEXT("[%s] unequipped slot %d ('%s')."),
		*GetNameSafe(GetOwner()), static_cast<int32>(Slot), *Item.ItemID.ToString());

	OnInventoryChanged();
	return true;
}

// --- Ownership-aware access -----------------------------------------------

int32 UInventoryComponent::TryWithdraw(int32 RequesterID, FName ItemID, int32 Amount, bool& bWasUnauthorized)
{
	bWasUnauthorized = (!IsPublic() && OwnerID != RequesterID);

	if (bWasUnauthorized)
	{
		UE_LOG(LogInventory, Warning,
			TEXT("THEFT: requester %d took from private container OwnerID=%d ('%s' x%d)."),
			RequesterID, OwnerID, *ItemID.ToString(), Amount);
	}

	return RemoveItem(ItemID, Amount);
}
