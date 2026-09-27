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
    material_path = "/Game/LD/Materials/M_P0Flat"
    if not unreal.EditorAssetLibrary.does_asset_exist(material_path):
        material = asset_tools.create_asset(
            "M_P0Flat", "/Game/LD/Materials", unreal.Material, unreal.MaterialFactoryNew()
        )
        if not material:
            raise RuntimeError("M_P0Flat creation failed")
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        color = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionVectorParameter, -300, 0
        )
        color.set_editor_property("parameter_name", "Color")
        color.set_editor_property("default_value", unreal.LinearColor(0.55, 0.48, 0.32, 1.0))
        unreal.MaterialEditingLibrary.connect_material_property(
            color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
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
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", mode_class)
    if not levels.save_current_level():
        raise RuntimeError("L_P0 save failed")
    entry_path = "/Game/LD/Maps/L_P0Entry"
    entry_class = unreal.load_class(None, "/Script/Mobile_defense_clone.LDEntryGameMode")
    if not entry_class:
        raise RuntimeError("Compile LDEntryGameMode before creating the entry map")
    if not unreal.EditorAssetLibrary.does_asset_exist(entry_path):
        if not levels.new_level(entry_path):
            raise RuntimeError("L_P0Entry creation failed")
        entry_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        entry_world.get_world_settings().set_editor_property("default_game_mode", entry_class)
        if not levels.save_current_level():
            raise RuntimeError("L_P0Entry save failed")
    elif not levels.load_level(entry_path):
        raise RuntimeError("L_P0Entry loading failed")
    entry_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if entry_world.get_world_settings().get_editor_property("default_game_mode") != entry_class:
        raise RuntimeError("Existing L_P0Entry has another GameMode; preserved for review")
    output = os.path.join(root, "Saved", "P0Runs", "assets.json")
    os.makedirs(os.path.dirname(output), exist_ok=True)
    with open(output, "w", encoding="utf-8") as stream:
        json.dump({"result": "Pass", "map": level_path, "game_mode": game_mode_path,
                   "material": material_path, "entry_map": entry_path,
                   "scope": "Asset creation and save only; runtime not verified"}, stream, indent=2)
    unreal.log("P0_ASSETS_SAVED " + output)


try:
    main()
finally:
    unreal.SystemLibrary.quit_editor()
