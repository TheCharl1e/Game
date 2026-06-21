# ue_save_all_dirty.py - Save ALL dirty packages (maps + content) before closing
# the editor for a C++ rebuild, so no in-editor work is lost.
import unreal
saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("SAVED_DIRTY:" + str(saved))
