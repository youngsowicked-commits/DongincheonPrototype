"""Simple native Editor Tools menu; no external UI dependency."""
import unreal
from . import core

COMMANDS = [
    ("DIEnvScene", "01 Environment / Scan all named buildings", "environment.scan_buildings()"),
    ("DIEnvSelected", "02 Environment / Scan selected actors", "environment.scan_selected()"),
    ("DIEnvMeshAssets", "03 Assets / Validate selected StaticMeshes", "environment.scan_selected_mesh_assets()"),
    ("DIEnvSceneAssets", "04 Assets / Validate building StaticMeshes", "environment.scan_building_mesh_assets()"),
    ("DIEnvGround", "05 Environment / Probe ground under selected", "environment.scan_selected_ground()"),
    ("DIAnimSelected", "06 Animation / Validate selected AnimSequences", "cinematic.scan_selected_animations()"),
    ("DISeqCurrent", "07 Sequencer / Validate open Sequence", "cinematic.scan_current_sequence()"),
    ("DISeqSelected", "08 Sequencer / Validate selected Sequences", "cinematic.scan_selected_sequences()"),
    ("DISelectActors", "09 QA / Select problem actors from last scan", "core.select_last_problem_actors()"),
    ("DIFocusAssets", "10 QA / Focus problem assets from last scan", "core.focus_last_problem_assets()"),
    ("DIReport", "11 QA / Print last report path", "core.show_last_report()"),
]


def register_menus():
    menus = unreal.ToolMenus.get()
    parent = menus.find_menu("LevelEditor.MainMenu")
    if parent is None:
        core.log("LevelEditor.MainMenu not ready. Run import di_toolkit; di_toolkit.register_menus() later", "WARNING")
        return False
    submenu = parent.add_sub_menu("LevelEditor.MainMenu", "DIProductionTools", "LevelEditor.MainMenu.DIProductionToolsMenu",
                                  "DI Tools", "Dongincheon environment, animation, and Sequencer quality checks")
    submenu.add_section("DIValidation", "Quality Checks")
    for id_, label, statement in COMMANDS:
        # Remove stale commands when re-registering the module in one session.
        try:
            menus.remove_entry("LevelEditor.MainMenu.DIProductionToolsMenu", "DIValidation", id_)
        except Exception:
            pass
        entry = unreal.ToolMenuEntry(name=id_, type=unreal.MultiBlockType.MENU_ENTRY)
        entry.set_label(label)
        entry.set_tool_tip("Read-only validation; JSON and CSV exported into Saved/DI_Toolkit")
        command = "import di_toolkit; di_toolkit." + statement
        entry.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", command)
        submenu.add_menu_entry("DIValidation", entry)
    menus.refresh_all_widgets()
    core.log("DI Tools menu registered. Use main menu > DI Tools.")
    return True
