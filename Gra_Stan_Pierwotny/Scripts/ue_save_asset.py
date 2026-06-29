# ue_save_asset.py - Save a single asset to disk by path (edit ASSET below).
# MCP blueprint edits compile in-memory but don't persist; this writes the .uasset.
import unreal
ASSET = "/Game/DocelowaGra/RTS/PC_RTSGameMode"
ok = unreal.EditorAssetLibrary.save_asset(ASSET, only_if_is_dirty=False)
print("SAVE_RESULT:" + ASSET + "=" + str(ok))
