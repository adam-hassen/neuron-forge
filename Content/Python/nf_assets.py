"""Neuron Forge - création des assets du jeu : materials, sons et Blueprints.

Toutes les fonctions sont idempotentes : un asset existant est réutilisé
et mis à jour, ce qui permet de relancer le script sans tout casser.
"""
import os

import unreal

import nf_audio

ROOT = "/Game/NeuronForge"
MATERIALS = ROOT + "/Materials"
AUDIO = ROOT + "/Audio"
BLUEPRINTS = ROOT + "/Blueprints"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


# --- Utilitaires -----------------------------------------------------------

def _load_or_create(name, folder, asset_class, factory):
    path = "{}/{}".format(folder, name)
    if eal.does_asset_exist(path):
        return eal.load_asset(path), False
    return asset_tools.create_asset(name, folder, asset_class, factory), True


def _expr(material, expression_class, x, y, **props):
    node = mel.create_material_expression(material, expression_class, x, y)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def _scalar_param(material, name, default, x, y):
    return _expr(material, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=default)


def _vector_param(material, name, color, x, y):
    return _expr(material, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                 default_value=unreal.LinearColor(*color))


def _binary(material, expression_class, a, b, x, y, a_out="", b_out=""):
    node = mel.create_material_expression(material, expression_class, x, y)
    mel.connect_material_expressions(a, a_out, node, "A")
    mel.connect_material_expressions(b, b_out, node, "B")
    return node


def _reset_material(material):
    mel.delete_all_material_expressions(material)


# --- Materials -------------------------------------------------------------

def create_instanced_material():
    """M_NF_Instanced : couleur et intensité lues dans les données par instance
    (PerInstanceCustomData 0..3 = R, G, B, intensité), écrites par ANFNetworkBuilder."""
    material, _ = _load_or_create("M_NF_Instanced", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    _reset_material(material)
    material.set_editor_property("used_with_instanced_static_meshes", True)

    r = _expr(material, unreal.MaterialExpressionPerInstanceCustomData, -1000, -200, data_index=0, const_default_value=0.2)
    g = _expr(material, unreal.MaterialExpressionPerInstanceCustomData, -1000, -50, data_index=1, const_default_value=0.6)
    b = _expr(material, unreal.MaterialExpressionPerInstanceCustomData, -1000, 100, data_index=2, const_default_value=1.0)
    intensity = _expr(material, unreal.MaterialExpressionPerInstanceCustomData, -1000, 250, data_index=3, const_default_value=1.0)

    rg = _binary(material, unreal.MaterialExpressionAppendVector, r, g, -750, -120)
    rgb = _binary(material, unreal.MaterialExpressionAppendVector, rg, b, -550, -50)

    boost = _scalar_param(material, "EmissiveBoost", 4.0, -550, 300)
    lit = _binary(material, unreal.MaterialExpressionMultiply, rgb, intensity, -350, 100)
    emissive = _binary(material, unreal.MaterialExpressionMultiply, lit, boost, -150, 150)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    base_factor = _expr(material, unreal.MaterialExpressionConstant, -350, -200, r=0.15)
    base = _binary(material, unreal.MaterialExpressionMultiply, rgb, base_factor, -150, -150)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = _expr(material, unreal.MaterialExpressionConstant, -150, 0, r=0.25)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(material)
    return material


def create_highlight_material():
    """M_NF_Highlight : objets interactifs. Émissif = EmissiveColor * (BaseGlow + Highlight * HighlightBoost).
    Le paramètre "Highlight" est piloté par le C++ (objet visé, bouton au sol enfoncé)."""
    material, _ = _load_or_create("M_NF_Highlight", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    _reset_material(material)

    base_color = _vector_param(material, "BaseColor", (0.05, 0.08, 0.1, 1), -700, -300)
    mel.connect_material_property(base_color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    emissive_color = _vector_param(material, "EmissiveColor", (0.1, 0.8, 1.0, 1), -900, 0)
    base_glow = _scalar_param(material, "BaseGlow", 0.4, -900, 200)
    highlight = _scalar_param(material, "Highlight", 0.0, -900, 300)
    boost = _scalar_param(material, "HighlightBoost", 6.0, -900, 400)

    boosted = _binary(material, unreal.MaterialExpressionMultiply, highlight, boost, -650, 350)
    glow = _binary(material, unreal.MaterialExpressionAdd, base_glow, boosted, -450, 250)
    emissive = _binary(material, unreal.MaterialExpressionMultiply, emissive_color, glow, -250, 100)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    metallic = _scalar_param(material, "Metallic", 0.6, -450, -150)
    roughness = _scalar_param(material, "Roughness", 0.35, -450, -50)
    mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(material)
    return material


def create_surface_material():
    """M_NF_Surface : surfaces du décor (métal, écran, cadre de porte...)."""
    material, _ = _load_or_create("M_NF_Surface", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    _reset_material(material)

    base_color = _vector_param(material, "BaseColor", (0.3, 0.32, 0.35, 1), -600, -300)
    metallic = _scalar_param(material, "Metallic", 0.8, -600, -150)
    roughness = _scalar_param(material, "Roughness", 0.4, -600, -50)
    emissive_color = _vector_param(material, "EmissiveColor", (0, 0, 0, 1), -800, 100)
    emissive_strength = _scalar_param(material, "EmissiveStrength", 0.0, -800, 250)
    emissive = _binary(material, unreal.MaterialExpressionMultiply, emissive_color, emissive_strength, -400, 150)

    mel.connect_material_property(base_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(material)
    return material


def create_material_instance(name, parent, scalars=None, vectors=None):
    instance, _ = _load_or_create(name, MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, parent)
    for key, value in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(instance, key, value)
    for key, value in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(instance, key, unreal.LinearColor(*value))
    mel.update_material_instance(instance)
    return instance


def create_materials():
    instanced = create_instanced_material()
    highlight = create_highlight_material()
    surface = create_surface_material()

    return {
        "Neuron": create_material_instance("MI_NF_Neuron", instanced, scalars={"EmissiveBoost": 5.0}),
        "Connection": create_material_instance("MI_NF_Connection", instanced, scalars={"EmissiveBoost": 2.5}),
        "Button": create_material_instance("MI_NF_Button", highlight,
                                           vectors={"BaseColor": (0.04, 0.06, 0.08, 1), "EmissiveColor": (0.1, 0.75, 1.0, 1)},
                                           scalars={"BaseGlow": 0.35, "HighlightBoost": 5.0}),
        "FloorPlate": create_material_instance("MI_NF_FloorPlate", highlight,
                                               vectors={"BaseColor": (0.03, 0.08, 0.05, 1), "EmissiveColor": (0.15, 1.0, 0.45, 1)},
                                               scalars={"BaseGlow": 0.5, "HighlightBoost": 8.0}),
        "FloorBase": create_material_instance("MI_NF_FloorBase", surface,
                                              vectors={"BaseColor": (0.08, 0.09, 0.1, 1)}, scalars={"Metallic": 0.9, "Roughness": 0.3}),
        "Table": create_material_instance("MI_NF_Table", surface,
                                          vectors={"BaseColor": (0.06, 0.07, 0.09, 1), "EmissiveColor": (0.1, 0.5, 1.0, 1)},
                                          scalars={"Metallic": 0.9, "Roughness": 0.25, "EmissiveStrength": 0.15}),
        "Screen": create_material_instance("MI_NF_Screen", surface,
                                           vectors={"BaseColor": (0.01, 0.015, 0.02, 1), "EmissiveColor": (0.02, 0.06, 0.1, 1)},
                                           scalars={"Metallic": 0.2, "Roughness": 0.15, "EmissiveStrength": 1.0}),
        "DoorFrame": create_material_instance("MI_NF_DoorFrame", surface,
                                              vectors={"BaseColor": (0.12, 0.13, 0.15, 1)}, scalars={"Metallic": 0.9, "Roughness": 0.35}),
        "DoorPanel": create_material_instance("MI_NF_DoorPanel", surface,
                                              vectors={"BaseColor": (0.35, 0.4, 0.48, 1), "EmissiveColor": (0.1, 0.4, 1.0, 1)},
                                              scalars={"Metallic": 0.85, "Roughness": 0.3, "EmissiveStrength": 0.05}),
        "Console": create_material_instance("MI_NF_Console", surface,
                                            vectors={"BaseColor": (0.1, 0.11, 0.13, 1)}, scalars={"Metallic": 0.7, "Roughness": 0.45}),
        "Light": create_material_instance("MI_NF_LightStrip", surface,
                                          vectors={"BaseColor": (1, 1, 1, 1), "EmissiveColor": (0.85, 0.92, 1.0, 1)},
                                          scalars={"Metallic": 0.0, "Roughness": 0.5, "EmissiveStrength": 6.0}),
    }


# --- Sons ------------------------------------------------------------------

def import_sounds():
    """Génère les WAV dans RawAssets/Audio puis les importe en SoundWave."""
    raw_dir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "RawAssets", "Audio")
    tasks = []
    for path in nf_audio.generate_all(raw_dir):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", path)
        task.set_editor_property("destination_path", AUDIO)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    asset_tools.import_asset_tasks(tasks)

    sounds = {}
    for name in nf_audio.SOUNDS:
        sound = eal.load_asset("{}/{}".format(AUDIO, name))
        if sound is None:
            raise RuntimeError("Import du son impossible : " + name)
        sounds[name] = sound
    return sounds


# --- Blueprints ------------------------------------------------------------

def _blueprint(name, parent_class):
    path = "{}/{}".format(BLUEPRINTS, name)
    if eal.does_asset_exist(path):
        blueprint = eal.load_asset(path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = asset_tools.create_asset(name, BLUEPRINTS, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    return blueprint


def _cdo(blueprint):
    return unreal.get_default_object(blueprint.generated_class())


def _set_materials(cdo, component_property, material):
    component = cdo.get_editor_property(component_property)
    component.set_material(0, material)


def create_blueprints(materials, sounds):
    """Crée les Blueprints enfants des classes C++ et leur assigne materials et sons."""
    blueprints = {}

    bp = _blueprint("BP_NF_Character", unreal.NFCharacter)
    blueprints["Character"] = bp

    bp = _blueprint("BP_NF_GameMode", unreal.NFGameMode)
    _cdo(bp).set_editor_property("default_pawn_class", blueprints["Character"].generated_class())
    blueprints["GameMode"] = bp

    bp = _blueprint("BP_NF_NetworkBuilder", unreal.NFNetworkBuilder)
    cdo = _cdo(bp)
    _set_materials(cdo, "table_mesh", materials["Table"])
    _set_materials(cdo, "neuron_instances", materials["Neuron"])
    _set_materials(cdo, "connection_instances", materials["Connection"])
    cdo.set_editor_property("training_start_sound", sounds["S_NF_TrainingStart"])
    cdo.set_editor_property("training_failed_sound", sounds["S_NF_Failure"])
    blueprints["NetworkBuilder"] = bp

    bp = _blueprint("BP_NF_ConfigButton", unreal.NFConfigButton)
    cdo = _cdo(bp)
    _set_materials(cdo, "mesh", materials["Button"])
    cdo.set_editor_property("click_sound", sounds["S_NF_Click"])
    blueprints["ConfigButton"] = bp

    bp = _blueprint("BP_NF_FloorButton", unreal.NFFloorButton)
    cdo = _cdo(bp)
    _set_materials(cdo, "base_mesh", materials["FloorBase"])
    _set_materials(cdo, "plate_mesh", materials["FloorPlate"])
    cdo.set_editor_property("press_sound", sounds["S_NF_FloorPress"])
    blueprints["FloorButton"] = bp

    bp = _blueprint("BP_NF_StatusDisplay", unreal.NFStatusDisplay)
    _set_materials(_cdo(bp), "screen_mesh", materials["Screen"])
    blueprints["StatusDisplay"] = bp

    bp = _blueprint("BP_NF_LabDoor", unreal.NFLabDoor)
    cdo = _cdo(bp)
    for part in ("frame_left", "frame_right", "frame_top"):
        _set_materials(cdo, part, materials["DoorFrame"])
    for part in ("panel_left", "panel_right"):
        _set_materials(cdo, part, materials["DoorPanel"])
    cdo.set_editor_property("validation_sound", sounds["S_NF_Validation"])
    cdo.set_editor_property("door_move_sound", sounds["S_NF_DoorMove"])
    blueprints["LabDoor"] = bp

    bp = _blueprint("BP_NF_ExitTrigger", unreal.NFExitTrigger)
    _cdo(bp).set_editor_property("completion_sound", sounds["S_NF_LevelComplete"])
    blueprints["ExitTrigger"] = bp

    for blueprint in blueprints.values():
        eal.save_loaded_asset(blueprint, only_if_is_dirty=False)
    return blueprints


def save_all():
    eal.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
