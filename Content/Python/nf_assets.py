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
                                          scalars={"Metallic": 0.0, "Roughness": 0.5, "EmissiveStrength": 2.0}),
    }



# --- Ambiance et décor animé du labo -------------------------------------------

_mood_collection = None


def create_mood_collection():
    """MPC_NF_Lab : couleur et intensité d'ambiance, pilotées en jeu par ANFLabAmbience."""
    global _mood_collection
    mpc, _ = _load_or_create("MPC_NF_Lab", MATERIALS, unreal.MaterialParameterCollection,
                             unreal.MaterialParameterCollectionFactoryNew())
    color = unreal.CollectionVectorParameter()
    color.set_editor_property("parameter_name", "MoodColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.10, 0.65, 1.0, 1.0))
    intensity = unreal.CollectionScalarParameter()
    intensity.set_editor_property("parameter_name", "MoodIntensity")
    intensity.set_editor_property("default_value", 1.0)
    mpc.set_editor_property("vector_parameters", [color])
    mpc.set_editor_property("scalar_parameters", [intensity])
    eal.save_loaded_asset(mpc, only_if_is_dirty=False)
    _mood_collection = mpc
    return mpc


def _mood(material, x, y):
    """Couleur d'ambiance x intensité, lues dans MPC_NF_Lab."""
    color = _expr(material, unreal.MaterialExpressionCollectionParameter, x, y,
                  collection=_mood_collection, parameter_name="MoodColor")
    intensity = _expr(material, unreal.MaterialExpressionCollectionParameter, x, y + 120,
                      collection=_mood_collection, parameter_name="MoodIntensity")
    return _binary(material, unreal.MaterialExpressionMultiply, color, intensity, x + 220, y + 40)


def _op(material, expression_class, source, x, y, **consts):
    """Opération à une entrée connectée ; l'autre opérande est une constante (const_a ou const_b)."""
    node = mel.create_material_expression(material, expression_class, x, y)
    for key, value in consts.items():
        node.set_editor_property(key, value)
    pin = "B" if "const_a" in consts else ("A" if "const_b" in consts else "")
    mel.connect_material_expressions(source, "", node, pin)
    return node


def _mask(material, source, x, y, r=False, g=False, b=False):
    return _op(material, unreal.MaterialExpressionComponentMask, source, x, y, r=r, g=g, b=b, a=False)


def create_trim_material():
    """M_NF_Trim : liserés lumineux dont la couleur suit l'ambiance du labo."""
    material, _ = _load_or_create("M_NF_Trim", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    _reset_material(material)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    mood = _mood(material, -900, 0)
    brightness = _scalar_param(material, "Brightness", 6.0, -650, 250)
    emissive = _binary(material, unreal.MaterialExpressionMultiply, mood, brightness, -350, 100)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    black = _expr(material, unreal.MaterialExpressionConstant, -350, -100, r=0.01)
    mel.connect_material_property(black, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.recompile_material(material)
    return material


def create_floor_grid_material():
    """M_NF_FloorGrid : sol sombre et brillant, quadrillé de lignes lumineuses aux couleurs de l'ambiance."""
    material, _ = _load_or_create("M_NF_FloorGrid", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    _reset_material(material)

    world = _expr(material, unreal.MaterialExpressionWorldPosition, -1600, 0)
    xy = _mask(material, world, -1400, 0, r=True, g=True)
    cell = _scalar_param(material, "CellSize", 100.0, -1400, 150)
    scaled = _binary(material, unreal.MaterialExpressionDivide, xy, cell, -1200, 50)
    frac = _op(material, unreal.MaterialExpressionFrac, scaled, -1000, 50)
    centered = _op(material, unreal.MaterialExpressionSubtract, frac, -850, 50, const_b=0.5)
    dist = _op(material, unreal.MaterialExpressionAbs, centered, -700, 50)
    dx = _mask(material, dist, -550, 0, r=True)
    dy = _mask(material, dist, -550, 120, g=True)
    edge = _binary(material, unreal.MaterialExpressionMax, dx, dy, -400, 50)
    near = _op(material, unreal.MaterialExpressionSubtract, edge, -250, 50, const_b=0.488)
    sharp = _op(material, unreal.MaterialExpressionMultiply, near, -100, 50, const_b=80.0)
    line = _op(material, unreal.MaterialExpressionSaturate, sharp, 50, 50)

    mood = _mood(material, -400, 350)
    brightness = _scalar_param(material, "LineBrightness", 1.6, -100, 500)
    glow = _binary(material, unreal.MaterialExpressionMultiply, mood, brightness, 100, 400)
    emissive = _binary(material, unreal.MaterialExpressionMultiply, line, glow, 300, 200)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    base = _vector_param(material, "BaseColor", (0.014, 0.016, 0.021, 1), 100, -300)
    metallic = _scalar_param(material, "Metallic", 0.7, 100, -180)
    roughness = _scalar_param(material, "Roughness", 0.22, 100, -80)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(material)
    return material


def create_server_led_material():
    """M_NF_ServerLEDs : façades de baies de serveurs couvertes de LED qui clignotent au hasard.
    Chaque cellule de 7 cm (plan XZ du monde) reçoit une valeur pseudo-aléatoire qui décale son clignotement."""
    material, _ = _load_or_create("M_NF_ServerLEDs", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    _reset_material(material)

    world = _expr(material, unreal.MaterialExpressionWorldPosition, -1800, 0)
    xz = _mask(material, world, -1600, 0, r=True, b=True)
    cells = _op(material, unreal.MaterialExpressionDivide, xz, -1450, 0, const_b=7.0)
    cell_id = _op(material, unreal.MaterialExpressionFloor, cells, -1300, -100)
    seed = _expr(material, unreal.MaterialExpressionConstant2Vector, -1300, 0, r=12.9898, g=78.233)
    dot = _binary(material, unreal.MaterialExpressionDotProduct, cell_id, seed, -1150, -60)
    sine = _op(material, unreal.MaterialExpressionSine, dot, -1000, -60)
    big = _op(material, unreal.MaterialExpressionMultiply, sine, -850, -60, const_b=43758.5453)
    rand = _op(material, unreal.MaterialExpressionFrac, big, -700, -60)

    time = _expr(material, unreal.MaterialExpressionTime, -1000, 150)
    slow = _op(material, unreal.MaterialExpressionMultiply, time, -850, 150, const_b=0.9)
    offset = _op(material, unreal.MaterialExpressionMultiply, rand, -700, 60, const_b=7.0)
    phase = _binary(material, unreal.MaterialExpressionAdd, slow, offset, -550, 120)
    cycle = _op(material, unreal.MaterialExpressionFrac, phase, -400, 120)
    on = _op(material, unreal.MaterialExpressionSubtract, cycle, -250, 120, const_b=0.55)
    on_sharp = _op(material, unreal.MaterialExpressionMultiply, on, -100, 120, const_b=6.0)
    blink = _op(material, unreal.MaterialExpressionSaturate, on_sharp, 50, 120)

    local = _op(material, unreal.MaterialExpressionFrac, cells, -1300, 300)
    centered = _op(material, unreal.MaterialExpressionSubtract, local, -1150, 300, const_b=0.5)
    d2 = _binary(material, unreal.MaterialExpressionDotProduct, centered, centered, -1000, 300)
    inside = _op(material, unreal.MaterialExpressionSubtract, d2, -850, 300, const_b=0.06)
    inside_sharp = _op(material, unreal.MaterialExpressionMultiply, inside, -700, 300, const_b=-80.0)
    dot_mask = _op(material, unreal.MaterialExpressionSaturate, inside_sharp, -550, 300)

    green = _expr(material, unreal.MaterialExpressionConstant3Vector, -400, -250, constant=unreal.LinearColor(0.1, 1.0, 0.35, 1))
    cyan = _expr(material, unreal.MaterialExpressionConstant3Vector, -400, -150, constant=unreal.LinearColor(0.1, 0.6, 1.0, 1))
    lerp = mel.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, -200, -200)
    mel.connect_material_expressions(green, "", lerp, "A")
    mel.connect_material_expressions(cyan, "", lerp, "B")
    mel.connect_material_expressions(rand, "", lerp, "Alpha")

    level = _op(material, unreal.MaterialExpressionMultiply, blink, 200, 120, const_b=10.0)
    level_base = _op(material, unreal.MaterialExpressionAdd, level, 350, 120, const_b=0.35)
    masked = _binary(material, unreal.MaterialExpressionMultiply, level_base, dot_mask, 500, 200)
    emissive = _binary(material, unreal.MaterialExpressionMultiply, lerp, masked, 650, 0)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    base = _expr(material, unreal.MaterialExpressionConstant3Vector, 650, -200, constant=unreal.LinearColor(0.01, 0.012, 0.015, 1))
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.recompile_material(material)
    return material


def create_holo_material():
    """M_NF_Holo : disque holographique additif, anneaux concentriques qui s'éloignent du centre."""
    material, _ = _load_or_create("M_NF_Holo", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    _reset_material(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)

    uv = _expr(material, unreal.MaterialExpressionTextureCoordinate, -1500, 0)
    centered = _op(material, unreal.MaterialExpressionSubtract, uv, -1300, 0, const_b=0.5)
    d2 = _binary(material, unreal.MaterialExpressionDotProduct, centered, centered, -1150, 0)
    radius = _op(material, unreal.MaterialExpressionSquareRoot, d2, -1000, 0)

    rings_count = _scalar_param(material, "Rings", 9.0, -1000, 150)
    scaled = _binary(material, unreal.MaterialExpressionMultiply, radius, rings_count, -850, 60)
    time = _expr(material, unreal.MaterialExpressionTime, -1000, 280)
    speed = _scalar_param(material, "Speed", 0.35, -1000, 380)
    moving = _binary(material, unreal.MaterialExpressionMultiply, time, speed, -850, 300)
    wave = _binary(material, unreal.MaterialExpressionSubtract, scaled, moving, -700, 150)
    frac = _op(material, unreal.MaterialExpressionFrac, wave, -550, 150)
    ring_cut = _op(material, unreal.MaterialExpressionSubtract, frac, -400, 150, const_b=0.78)
    ring_sharp = _op(material, unreal.MaterialExpressionMultiply, ring_cut, -250, 150, const_b=5.0)
    rings = _op(material, unreal.MaterialExpressionSaturate, ring_sharp, -100, 150)
    rings_lit = _op(material, unreal.MaterialExpressionMultiply, rings, 50, 150, const_b=3.0)
    glow = _op(material, unreal.MaterialExpressionAdd, rings_lit, 200, 150, const_b=0.12)

    edge_in = _op(material, unreal.MaterialExpressionSubtract, radius, -700, -150, const_a=0.5)
    edge_sharp = _op(material, unreal.MaterialExpressionMultiply, edge_in, -550, -150, const_b=14.0)
    edge = _op(material, unreal.MaterialExpressionSaturate, edge_sharp, -400, -150)
    core = _op(material, unreal.MaterialExpressionMultiply, radius, -700, -280, const_b=5.0)
    core_fade = _op(material, unreal.MaterialExpressionSaturate, core, -550, -280)
    fade = _binary(material, unreal.MaterialExpressionMultiply, edge, core_fade, -250, -200)

    mood = _mood(material, -250, 400)
    brightness = _scalar_param(material, "Brightness", 1.0, 50, 450)
    tint = _binary(material, unreal.MaterialExpressionMultiply, mood, brightness, 250, 420)
    shaped = _binary(material, unreal.MaterialExpressionMultiply, glow, fade, 350, 0)
    emissive = _binary(material, unreal.MaterialExpressionMultiply, shaped, tint, 550, 150)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    return material


def create_chart_material():
    """M_NF_Chart : écran non éclairé qui affiche la texture « Chart » dessinée en C++ (carnet du labo)."""
    material, _ = _load_or_create("M_NF_Chart", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    _reset_material(material)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    texture = _expr(material, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0, parameter_name="Chart",
                    texture=eal.load_asset("/Engine/EngineResources/DefaultTexture"))
    brightness = _scalar_param(material, "Brightness", 1.4, -600, 250)
    emissive = _binary(material, unreal.MaterialExpressionMultiply, texture, brightness, -250, 100)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    return material


def create_lab_materials(materials):
    """Ajoute au dictionnaire les materials du décor sci-fi."""
    materials["MoodCollection"] = create_mood_collection()
    trim = create_trim_material()
    surface = eal.load_asset(MATERIALS + "/M_NF_Surface")
    instanced = eal.load_asset(MATERIALS + "/M_NF_Instanced")

    materials["Trim"] = create_material_instance("MI_NF_Trim", trim, scalars={"Brightness": 6.0})
    materials["TrimSoft"] = create_material_instance("MI_NF_TrimSoft", trim, scalars={"Brightness": 2.0})
    materials["FloorGrid"] = create_floor_grid_material()
    materials["ServerLEDs"] = create_server_led_material()
    holo = create_holo_material()
    materials["Holo"] = create_material_instance("MI_NF_Holo", holo, scalars={"Brightness": 1.0, "Rings": 9.0})
    materials["HoloRing"] = create_material_instance("MI_NF_HoloRing", holo, scalars={"Brightness": 0.7, "Rings": 4.0, "Speed": 0.2})
    materials["Pulse"] = create_material_instance("MI_NF_Pulse", instanced, scalars={"EmissiveBoost": 9.0})
    materials["Chart"] = create_chart_material()
    highlight_for_plant = eal.load_asset(MATERIALS + "/M_NF_Highlight")
    materials["Plant"] = create_material_instance("MI_NF_Plant", highlight_for_plant,
                                                  vectors={"BaseColor": (0.12, 0.75, 0.22, 1), "EmissiveColor": (0.05, 0.3, 0.08, 1)},
                                                  scalars={"BaseGlow": 0.4, "HighlightBoost": 0.0, "Metallic": 0.0, "Roughness": 0.6})
    materials["PotBody"] = create_material_instance("MI_NF_PotBody", surface,
                                                    vectors={"BaseColor": (0.42, 0.18, 0.09, 1)},
                                                    scalars={"Metallic": 0.0, "Roughness": 0.7, "EmissiveStrength": 0.0})

    def surface_mi(name, color, metallic, roughness):
        return create_material_instance(name, surface, vectors={"BaseColor": color},
                                        scalars={"Metallic": metallic, "Roughness": roughness, "EmissiveStrength": 0.0})

    materials["Wall"] = surface_mi("MI_NF_Wall", (0.022, 0.026, 0.034, 1), 0.1, 0.7)
    materials["Ceiling"] = surface_mi("MI_NF_Ceiling", (0.012, 0.014, 0.018, 1), 0.1, 0.8)
    materials["Pillar"] = surface_mi("MI_NF_Pillar", (0.055, 0.060, 0.072, 1), 0.9, 0.28)
    materials["Rack"] = surface_mi("MI_NF_Rack", (0.018, 0.020, 0.024, 1), 0.9, 0.32)
    # Pièces du puzzle de la Forge (M_NF_Highlight : le C++ règle EmissiveColor et Highlight)
    highlight = eal.load_asset(MATERIALS + "/M_NF_Highlight")
    materials["Module"] = create_material_instance("MI_NF_Module", highlight,
                                                   vectors={"BaseColor": (0.03, 0.05, 0.08, 1), "EmissiveColor": (0.15, 0.7, 1.0, 1)},
                                                   scalars={"BaseGlow": 0.5, "HighlightBoost": 4.0, "Metallic": 0.8, "Roughness": 0.3})
    materials["Orb"] = create_material_instance("MI_NF_Orb", highlight,
                                                vectors={"BaseColor": (0.6, 0.85, 1.0, 1), "EmissiveColor": (0.3, 0.85, 1.0, 1)},
                                                scalars={"BaseGlow": 3.0, "HighlightBoost": 6.0, "Metallic": 0.0, "Roughness": 0.2})
    materials["Crystal"] = create_material_instance("MI_NF_Crystal", highlight,
                                                    vectors={"BaseColor": (0.1, 0.1, 0.1, 1), "EmissiveColor": (1.0, 1.0, 1.0, 1)},
                                                    scalars={"BaseGlow": 2.5, "HighlightBoost": 5.0, "Metallic": 0.3, "Roughness": 0.1})
    materials["Platform"] = surface_mi("MI_NF_Platform", (0.04, 0.05, 0.065, 1), 0.9, 0.25)
    materials["Bench"] = surface_mi("MI_NF_Bench", (0.05, 0.055, 0.065, 1), 0.85, 0.35)

    # Les panneaux et la table passent eux aussi dans la palette sombre
    create_material_instance("MI_NF_Console", surface, vectors={"BaseColor": (0.012, 0.016, 0.024, 1)},
                             scalars={"Metallic": 0.15, "Roughness": 0.75})
    create_material_instance("MI_NF_Table", surface, vectors={"BaseColor": (0.03, 0.035, 0.045, 1), "EmissiveColor": (0.1, 0.5, 1.0, 1)},
                             scalars={"Metallic": 0.9, "Roughness": 0.22, "EmissiveStrength": 0.05})
    return materials


# --- Mains de chercheur --------------------------------------------------------

# Poignet et coude gauches du mannequin en pose de référence (mesurés avec nf_inspect_arms.py)
WRIST = (47.8, 15.7, 104.5)
ELBOW = (35.0, -1.9, 120.9)


def create_researcher_arms_material():
    """M_NF_ResearcherArms : peau couleur chair au-delà du poignet, manche de blouse blanche avant.

    Le mesh des bras n'a que deux slots (M_HeadLegs, M_Torso) : la séparation se fait dans le material,
    à partir de la position du point dans la pose de référence (Pre-Skinned Local Position), qui ne bouge
    pas avec l'animation. Les deux bras sont symétriques (|x|). Gants : paramètre GloveAmount (0 par défaut).
    """
    import math
    material, _ = _load_or_create("M_NF_ResearcherArms", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    _reset_material(material)
    material.set_editor_property("used_with_skeletal_mesh", True)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)

    # Position avant skinning, transmise du vertex shader au pixel shader
    pre = _expr(material, unreal.MaterialExpressionPreSkinnedPosition, -2200, 0)
    interp = mel.create_material_expression(material, unreal.MaterialExpressionVertexInterpolator, -2000, 0)
    mel.connect_material_expressions(pre, "", interp, "")

    # Symétrie gauche / droite : P' = (|x|, y, z)
    px = _mask(material, interp, -1800, -100, r=True)
    abs_x = _op(material, unreal.MaterialExpressionAbs, px, -1650, -100)
    yz = _mask(material, interp, -1800, 50, g=True, b=True)
    mirrored = _binary(material, unreal.MaterialExpressionAppendVector, abs_x, yz, -1500, 0)

    # Distance signée au plan du poignet, le long de l'avant-bras (cm) : positive = main
    direction = [WRIST[i] - ELBOW[i] for i in range(3)]
    length = math.sqrt(sum(c * c for c in direction))
    normal = [c / length for c in direction]
    wrist = _expr(material, unreal.MaterialExpressionConstant3Vector, -1500, 150, constant=unreal.LinearColor(*WRIST, 1))
    axis = _expr(material, unreal.MaterialExpressionConstant3Vector, -1500, 250, constant=unreal.LinearColor(*normal, 1))
    relative = _binary(material, unreal.MaterialExpressionSubtract, mirrored, wrist, -1300, 100)
    along = _binary(material, unreal.MaterialExpressionDotProduct, relative, axis, -1150, 150)

    # Main : au-delà de WristOffset (la manche s'arrête un peu avant le poignet)
    offset = _scalar_param(material, "WristOffset", -2.5, -1150, 300)
    beyond = _binary(material, unreal.MaterialExpressionSubtract, along, offset, -950, 200)
    beyond_sharp = _op(material, unreal.MaterialExpressionMultiply, beyond, -800, 200, const_b=3.0)
    hand_raw = _op(material, unreal.MaterialExpressionSaturate, beyond_sharp, -650, 200)
    # Uniquement sur les bras (loin du torse), pas sur les jambes ni la tête
    arm_zone = _op(material, unreal.MaterialExpressionSubtract, abs_x, -1150, -250, const_b=28.0)
    arm_sharp = _op(material, unreal.MaterialExpressionSaturate, arm_zone, -950, -250)
    hand = _binary(material, unreal.MaterialExpressionMultiply, hand_raw, arm_sharp, -500, 100)

    # Poignet de manche : bande de 6 cm juste avant la peau
    cuff_start = _op(material, unreal.MaterialExpressionAdd, beyond, -800, 400, const_b=6.0)
    cuff_sharp = _op(material, unreal.MaterialExpressionMultiply, cuff_start, -650, 400, const_b=2.0)
    cuff_raw = _op(material, unreal.MaterialExpressionSaturate, cuff_sharp, -500, 400)
    not_hand = _op(material, unreal.MaterialExpressionOneMinus, hand, -500, 300)
    cuff = _binary(material, unreal.MaterialExpressionMultiply, cuff_raw, not_hand, -350, 350)

    skin = _vector_param(material, "SkinColor", (0.50, 0.27, 0.17, 1), -500, -400)
    glove = _vector_param(material, "GloveColor", (0.10, 0.30, 0.85, 1), -500, -250)
    glove_amount = _scalar_param(material, "GloveAmount", 0.0, -500, -100)
    coat = _vector_param(material, "CoatColor", (0.86, 0.87, 0.89, 1), -500, 550)
    cuff_color = _vector_param(material, "CuffColor", (0.62, 0.68, 0.76, 1), -500, 700)

    def lerp(a, b, alpha, x, y):
        node = mel.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, x, y)
        mel.connect_material_expressions(a, "", node, "A")
        mel.connect_material_expressions(b, "", node, "B")
        mel.connect_material_expressions(alpha, "", node, "Alpha")
        return node

    hand_color = lerp(skin, glove, glove_amount, -250, -300)
    sleeve_color = lerp(coat, cuff_color, cuff, -250, 550)
    base = lerp(sleeve_color, hand_color, hand, 0, 100)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_hand = _expr(material, unreal.MaterialExpressionConstant, -250, -150, r=0.45)
    rough_coat = _expr(material, unreal.MaterialExpressionConstant, -250, 700, r=0.85)
    roughness = lerp(rough_coat, rough_hand, hand, 0, 300)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Seuls les bras sont visibles (|x| > 15 cm dans la pose de référence) : torse, tête et jambes masqués
    visible_zone = _op(material, unreal.MaterialExpressionSubtract, abs_x, -950, -400, const_b=15.0)
    visible = _op(material, unreal.MaterialExpressionSaturate, visible_zone, -800, -400)
    mel.connect_material_property(visible, "", unreal.MaterialProperty.MP_OPACITY_MASK)

    mel.recompile_material(material)
    return material


def create_arm_materials(materials):
    arms = create_researcher_arms_material()
    materials["ResearcherArms"] = create_material_instance("MI_NF_ResearcherArms", arms, scalars={"GloveAmount": 0.0})
    return materials

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
    if "ResearcherArms" in materials:
        # Bras vus à la première personne : mains de chercheur (le corps qui projette l'ombre garde le template)
        arms = _cdo(bp).get_editor_property("researcher_arms")
        for slot in range(2):
            arms.set_material(slot, materials["ResearcherArms"])
    blueprints["Character"] = bp

    bp = _blueprint("BP_NF_GameMode", unreal.NFGameMode)
    _cdo(bp).set_editor_property("default_pawn_class", blueprints["Character"].generated_class())
    blueprints["GameMode"] = bp

    bp = _blueprint("BP_NF_NetworkBuilder", unreal.NFNetworkBuilder)
    cdo = _cdo(bp)
    _set_materials(cdo, "table_mesh", materials["Table"])
    _set_materials(cdo, "neuron_instances", materials["Neuron"])
    _set_materials(cdo, "connection_instances", materials["Connection"])
    _set_materials(cdo, "pulse_instances", materials["Pulse"])
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
