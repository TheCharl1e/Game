# ue_open_asset.py - Open an asset's editor window (edit ASSET below).
# Useful to surface a Blueprint/graph for manual in-editor work.
import unreal
ASSET = "/Game/DocelowaGra/UI/Inventory/WBP_Tab_Body"
asset = unreal.load_asset(ASSET)
unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).open_editor_for_assets([asset])
print("OPENED:" + ("ok" if asset else "load-failed"))
