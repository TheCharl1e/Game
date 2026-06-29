# ue_save_widgets.py - Compile-check and save the debug UI widget assets to disk.
# Run via the mcp-unreal execute_script tool (UE Python Editor Script Plugin).
# Editing a Blueprint via MCP compiles it but does NOT persist; this saves it.
import unreal

paths = [
    "/Game/DocelowaGra/UI/Inventory/WBP_Tab_Body",
    "/Game/DocelowaGra/UI/Debug/WBP_NPC_Inspector",
]
results = {}
for p in paths:
    results[p] = unreal.EditorAssetLibrary.save_asset(p, only_if_is_dirty=False)
print("SAVE_RESULTS:" + str(results))
