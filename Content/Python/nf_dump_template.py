"""Neuron Forge - inventaire des Blueprints du template First Person.

Écrit dans Saved/nf_dump_template.txt les composants, assets et réglages du
personnage, du contrôleur et du GameMode du template, pour que les classes C++
du jeu puissent les reproduire.
"""
import os
import unreal

out_lines = []


def log(text):
    out_lines.append(text)


def dump_props(obj, names):
    for name in names:
        try:
            log("    {} = {}".format(name, obj.get_editor_property(name)))
        except Exception as exc:  # propriété absente
            log("    {} = <{}>".format(name, type(exc).__name__))


def dump_blueprint(path, prop_names):
    bp = unreal.load_asset(path)
    log("=== " + path)
    if bp is None:
        log("  introuvable")
        return
    gen = bp.generated_class()
    asset_data = unreal.EditorAssetLibrary.find_asset_data(path)
    log("  parent = {}".format(asset_data.get_tag_value("ParentClass")))
    cdo = unreal.get_default_object(gen)
    dump_props(cdo, prop_names)

    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sub.k2_gather_subobject_data_for_blueprint(bp)
    for handle in handles:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if comp is None:
            continue
        log("  [comp] {} : {}".format(comp.get_name(), comp.get_class().get_name()))
        if isinstance(comp, unreal.SkeletalMeshComponent):
            dump_props(comp, ["skeletal_mesh_asset", "anim_class", "first_person_primitive_type",
                              "only_owner_see", "owner_no_see", "relative_location", "relative_rotation",
                              "cast_shadow"])
        elif isinstance(comp, unreal.CameraComponent):
            dump_props(comp, ["relative_location", "relative_rotation", "field_of_view",
                              "enable_first_person_field_of_view", "first_person_field_of_view",
                              "enable_first_person_scale", "first_person_scale",
                              "use_pawn_control_rotation"])
        elif isinstance(comp, unreal.CapsuleComponent):
            dump_props(comp, ["capsule_half_height", "capsule_radius"])
        elif isinstance(comp, unreal.CharacterMovementComponent):
            dump_props(comp, ["max_walk_speed", "jump_z_velocity", "air_control", "braking_deceleration_walking"])


dump_blueprint("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter",
               ["jump_action", "move_action", "look_action", "mouse_look_action"])
dump_blueprint("/Game/FirstPerson/Blueprints/BP_FirstPersonPlayerController",
               ["default_mapping_contexts", "mobile_excluded_mapping_contexts",
                "mobile_controls_widget_class", "player_camera_manager_class"])
dump_blueprint("/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode",
               ["default_pawn_class", "player_controller_class", "hud_class"])
dump_blueprint("/Game/FirstPerson/Blueprints/BP_FirstPersonCameraManager",
               ["view_pitch_min", "view_pitch_max"])

log("=== assets")
for folder in ["/Game/FirstPerson", "/Game/Input", "/Game/Characters/Mannequins/Meshes",
               "/Game/Characters/Mannequins/Anims", "/Game/LevelPrototyping/Interactable",
               "/Game/LevelPrototyping/Meshes", "/Game/LevelPrototyping/Materials"]:
    for asset in unreal.EditorAssetLibrary.list_assets(folder, recursive=True, include_folder=False):
        log("  " + asset)

imc = unreal.load_asset("/Game/Input/IMC_Default")
if imc:
    log("=== IMC_Default mappings")
    try:
        mappings = imc.get_editor_property("default_key_mappings").get_editor_property("mappings")
    except Exception:
        mappings = imc.get_editor_property("mappings")
    for m in mappings:
        log("  {} -> {}".format(m.get_editor_property("key").get_editor_property("key_name"),
                                m.get_editor_property("action").get_name()))

with open(os.path.join(unreal.Paths.project_saved_dir(), "nf_dump_template.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(out_lines) + "\n")
