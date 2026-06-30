// NPCAIControllerBase.cpp
#include "NPCAIControllerBase.h"
#include "BehaviorTree/BehaviorTree.h"

void ANPCAIControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Run the designer-assigned tree (BT_Exploration on the NPC BP). Guarded: a controller without a tree
	// assigned simply runs nothing here — the Blueprint may still start its own logic.
	if (DefaultBehaviorTree)
	{
		RunBehaviorTree(DefaultBehaviorTree);
	}
}
