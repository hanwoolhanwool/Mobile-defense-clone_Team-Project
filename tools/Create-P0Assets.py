"""Run once after Editor compilation; creates only the P0-owned map and Blueprint.

UnrealEditor-Cmd project.uproject -unattended -nullrhi -ExecutePythonScript=.../Create-P0Assets.py
Existing assets are preserved. Runtime code never imports this script.
"""
import json
import os
import unreal


def main():
    root = unreal.SystemLibrary.get_project_directory()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    game_mode_path = "/Game/LD/Core/BP_LDGameMode"
    if not unreal.EditorAssetLibrary.does_asset_exist(game_mode_path):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property(
            "parent_class", unreal.load_class(None, "/Script/Mobile_defense_clone.LDGameMode")
        )
        blueprint = asset_tools.create_asset("BP_LDGameMode", "/Game/LD/Core", unreal.Blueprint, factory)
        if not blueprint:
            raise RuntimeError("BP_LDGameMode creation failed")
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    mode_class = unreal.EditorAssetLibrary.load_blueprint_class(game_mode_path)
    if not mode_class:
        raise RuntimeError("BP_LDGameMode class missing")
    level_path = "/Game/LD/Maps/L_P0"
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(level_path):
        if not levels.new_level(level_path):
            raise RuntimeError("L_P0 creation failed")
    elif not levels.load_level(level_path):
        raise RuntimeError("L_P0 loading failed")
    world = unreal.EditorLevelLibrary.get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", mode_class)
    if not levels.save_current_level():
        raise RuntimeError("L_P0 save failed")
    output = os.path.join(root, "Saved", "P0Runs", "assets.json")
    os.makedirs(os.path.dirname(output), exist_ok=True)
    with open(output, "w", encoding="utf-8") as stream:
        json.dump({"result": "Pass", "map": level_path, "game_mode": game_mode_path,
                   "scope": "Asset creation and save only; runtime not verified"}, stream, indent=2)
    unreal.log("P0_ASSETS_SAVED " + output)


try:
    main()
finally:
    unreal.SystemLibrary.quit_editor()
