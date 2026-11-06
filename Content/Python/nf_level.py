"""Neuron Forge - construction du niveau du labo (L_Lab).

Plan (vue de dessus, unités en cm, le joueur entre côté -X et regarde vers +X) :

    Y=+700 ┌──────────────────────────────────────────────┐
           │              [Panneau de configuration]      │
           │                                              ├────────────┐
    spawn  │   ●        [Bouton sol] [Holotable] [Écran]  Porte  Sortie │
           │                                              ├────────────┘
           │              [Mode d'emploi]                 │
    Y=-700 └──────────────────────────────────────────────┘
         X=-1000                                        X=600       X=1220

Le niveau est entièrement recréé à chaque exécution.
"""
import math

import unreal

LEVEL_PATH = "/Game/NeuronForge/Maps/L_Lab"

CUBE = "/Engine/BasicShapes/Cube.Cube"
WALL_MATERIAL = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray"
FLOOR_MATERIAL = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"

ROOM_MIN_X, ROOM_MAX_X = -1000.0, 600.0
ROOM_HALF_Y = 700.0
ROOM_HEIGHT = 400.0
WALL = 20.0

DOOR_WIDTH, DOOR_HEIGHT, DOOR_FRAME = 240.0, 300.0, 25.0
CORRIDOR_END_X, CORRIDOR_HALF_Y = 1220.0, 200.0

# Le C++ des acteurs du labo suppose que le joueur les regarde depuis leur axe +X :
# yaw 180 les tourne vers l'entrée du labo (-X).
FACE_PLAYER = 180.0

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eal = unreal.EditorAssetLibrary


def _vec(x, y, z):
    return unreal.Vector(x, y, z)


def _rot(yaw=0.0, pitch=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def _spawn(actor_class, location, yaw=0.0, label=None, folder=None):
    actor = eas.spawn_actor_from_class(actor_class, _vec(*location), _rot(yaw))
    if actor is None:
        raise RuntimeError("Impossible de créer l'acteur {}".format(label or actor_class))
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    return actor


def _box(label, center, size, material, folder="Salle"):
    """Bloc de décor : cube de 100 cm mis à l'échelle (pivot au centre)."""
    actor = _spawn(unreal.StaticMeshActor, center, label=label, folder=folder)
    component = actor.static_mesh_component
    component.set_static_mesh(eal.load_asset(CUBE))
    component.set_material(0, material)
    actor.set_actor_scale3d(_vec(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    return actor


def _text(label, location, text, size, color=(200, 235, 255), yaw=FACE_PLAYER, folder="Textes",
          align=unreal.HorizTextAligment.EHTA_CENTER):
    actor = _spawn(unreal.TextRenderActor, location, yaw=yaw, label=label, folder=folder)
    component = actor.text_render
    component.set_text(text)
    component.set_world_size(size)
    component.set_horizontal_alignment(align)
    component.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    component.set_text_render_color(unreal.Color(r=color[0], g=color[1], b=color[2], a=255))
    return actor


def _mood_tag(actor):
    """Les lumières taguées NFMood suivent l'ambiance (ANFLabAmbience)."""
    actor.set_editor_property("tags", [unreal.Name("NFMood")])


def _spot_light(label, location, pitch, intensity, radius, inner, outer, color=(1.0, 1.0, 1.0), mood=False, yaw=0.0):
    actor = eas.spawn_actor_from_class(unreal.SpotLight, _vec(*location), _rot(yaw=yaw, pitch=pitch))
    actor.set_actor_label(label)
    actor.set_folder_path("Lumières")
    light = actor.spot_light_component
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    light.set_intensity(intensity)
    light.set_attenuation_radius(radius)
    light.set_inner_cone_angle(inner)
    light.set_outer_cone_angle(outer)
    light.set_light_color(unreal.LinearColor(*color, 1.0))
    light.set_cast_shadows(True)
    if mood:
        _mood_tag(actor)
    return actor


def _point_light(label, location, intensity, radius, color=(1.0, 0.97, 0.92), shadows=True):
    actor = _spawn(unreal.PointLight, location, label=label, folder="Lumières")
    light = actor.point_light_component
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    light.set_intensity(intensity)
    light.set_attenuation_radius(radius)
    light.set_light_color(unreal.LinearColor(*color, 1.0))
    light.set_cast_shadows(shadows)
    return actor


# --- Construction ----------------------------------------------------------

def build_room(mats):
    wall_mat = mats["Wall"]
    floor_mat = mats["FloorGrid"]
    ceiling_mat = mats["Ceiling"]
    length = ROOM_MAX_X - ROOM_MIN_X
    center_x = (ROOM_MIN_X + ROOM_MAX_X) / 2.0
    width = 2 * ROOM_HALF_Y

    _box("Sol", (center_x, 0, -WALL / 2), (length + 2 * WALL, width + 2 * WALL, WALL), floor_mat)
    _box("Plafond", (center_x, 0, ROOM_HEIGHT + WALL / 2), (length + 2 * WALL, width + 2 * WALL, WALL), ceiling_mat)
    _box("Mur_Entree", (ROOM_MIN_X - WALL / 2, 0, ROOM_HEIGHT / 2), (WALL, width + 2 * WALL, ROOM_HEIGHT), wall_mat)
    _box("Mur_Nord", (center_x, ROOM_HALF_Y + WALL / 2, ROOM_HEIGHT / 2), (length, WALL, ROOM_HEIGHT), wall_mat)
    _box("Mur_Sud", (center_x, -ROOM_HALF_Y - WALL / 2, ROOM_HEIGHT / 2), (length, WALL, ROOM_HEIGHT), wall_mat)

    # Mur du fond percé pour la porte (largeur porte + cadre)
    opening = DOOR_WIDTH / 2 + DOOR_FRAME
    segment = ROOM_HALF_Y + WALL - opening
    wall_x = ROOM_MAX_X + WALL / 2
    _box("Mur_Porte_Gauche", (wall_x, opening + segment / 2, ROOM_HEIGHT / 2), (WALL, segment, ROOM_HEIGHT), wall_mat)
    _box("Mur_Porte_Droite", (wall_x, -opening - segment / 2, ROOM_HEIGHT / 2), (WALL, segment, ROOM_HEIGHT), wall_mat)
    lintel = ROOM_HEIGHT - (DOOR_HEIGHT + DOOR_FRAME)
    _box("Mur_Porte_Linteau", (wall_x, 0, ROOM_HEIGHT - lintel / 2), (WALL, 2 * opening, lintel), wall_mat)

    # Couloir de sortie
    corridor_len = CORRIDOR_END_X - (ROOM_MAX_X + WALL)
    corridor_x = ROOM_MAX_X + WALL + corridor_len / 2
    corridor_w = 2 * CORRIDOR_HALF_Y
    _box("Couloir_Sol", (corridor_x, 0, -WALL / 2), (corridor_len, corridor_w + 2 * WALL, WALL), floor_mat, "Couloir")
    _box("Couloir_Plafond", (corridor_x, 0, ROOM_HEIGHT + WALL / 2), (corridor_len, corridor_w + 2 * WALL, WALL), ceiling_mat, "Couloir")
    _box("Couloir_Mur_Nord", (corridor_x, CORRIDOR_HALF_Y + WALL / 2, ROOM_HEIGHT / 2), (corridor_len, WALL, ROOM_HEIGHT), wall_mat, "Couloir")
    _box("Couloir_Mur_Sud", (corridor_x, -CORRIDOR_HALF_Y - WALL / 2, ROOM_HEIGHT / 2), (corridor_len, WALL, ROOM_HEIGHT), wall_mat, "Couloir")
    _box("Couloir_Fond", (CORRIDOR_END_X + WALL / 2, 0, ROOM_HEIGHT / 2), (WALL, corridor_w + 2 * WALL, ROOM_HEIGHT), wall_mat, "Couloir")

    # Bandeaux lumineux au plafond
    for i, x in enumerate((-750, -350, 50, 400)):
        for j, y in enumerate((-350, 350)):
            _box("Bandeau_{}_{}".format(i, j), (x, y, ROOM_HEIGHT - 3), (260, 24, 6), mats["Light"], "Lumières")


def build_lighting():
    # Éclairage général doux et froid, teinté par l'ambiance
    for i, x in enumerate((-750, -350, 50, 400)):
        for j, y in enumerate((-350, 350)):
            light = _point_light("Lumiere_{}_{}".format(i, j), (x, y, ROOM_HEIGHT - 40), 600.0, 850.0,
                                 color=(0.80, 0.90, 1.0), shadows=(i % 2 == 0))
            _mood_tag(light)
    corridor = _point_light("Lumiere_Couloir", (950, 0, ROOM_HEIGHT - 40), 1800.0, 700.0, color=(0.75, 1.0, 0.8))
    _mood_tag(corridor)

    # Projecteurs : l'holotable, les deux panneaux, la porte
    _spot_light("Spot_Holotable", (0, 0, ROOM_HEIGHT - 5), -90.0, 1800.0, 700.0, 22.0, 38.0, (0.6, 0.85, 1.0), mood=True)
    _spot_light("Spot_Console", (-300, 430, ROOM_HEIGHT - 10), -55.0, 700.0, 800.0, 25.0, 45.0, (0.9, 0.95, 1.0))
    _spot_light("Spot_Aide", (-300, -430, ROOM_HEIGHT - 10), -55.0, 700.0, 800.0, 25.0, 45.0, (0.9, 0.95, 1.0))
    _spot_light("Spot_Porte", (300, 0, ROOM_HEIGHT - 10), -35.0, 1200.0, 900.0, 20.0, 40.0, (0.8, 0.9, 1.0), mood=True)

    sky = _spawn(unreal.SkyLight, (0, 0, 300), label="SkyLight", folder="Lumières")
    sky.light_component.set_intensity(0.15)

    # Brume volumétrique : rend visibles les faisceaux des projecteurs
    fog = _spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Brume", folder="Lumières")
    fog_component = fog.get_editor_property("component")
    fog_component.set_editor_property("fog_density", 0.012)
    fog_component.set_editor_property("fog_height_falloff", 0.4)
    fog_component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.005, 0.01, 0.02, 1.0))
    fog_component.set_editor_property("enable_volumetric_fog", True)
    fog_component.set_editor_property("volumetric_fog_extinction_scale", 0.6)

    ppv = _spawn(unreal.PostProcessVolume, (0, 0, 200), label="PostProcess", folder="Lumières")
    ppv.set_editor_property("unbound", True)
    settings = ppv.get_editor_property("settings")
    settings.set_editor_property("override_auto_exposure_min_brightness", True)
    # Exposition quasi fixe et sombre : ambiance de labo de nuit, les éléments lumineux ressortent
    settings.set_editor_property("auto_exposure_min_brightness", 1.2)
    settings.set_editor_property("override_auto_exposure_max_brightness", True)
    settings.set_editor_property("auto_exposure_max_brightness", 1.6)
    settings.set_editor_property("override_bloom_intensity", True)
    settings.set_editor_property("bloom_intensity", 1.3)
    settings.set_editor_property("override_vignette_intensity", True)
    settings.set_editor_property("vignette_intensity", 0.5)
    ppv.set_editor_property("settings", settings)


def build_lab(blueprints, mats):
    """Place les acteurs de gameplay et les relie à l'holotable."""
    builder = _spawn(blueprints["NetworkBuilder"].generated_class(), (0, 0, 0), FACE_PLAYER, "Holotable", "Gameplay")

    display = _spawn(blueprints["StatusDisplay"].generated_class(), (ROOM_MAX_X - 8, 330, 215), FACE_PLAYER, "EcranEtat", "Gameplay")
    display.set_editor_property("builder", builder)

    floor_button = _spawn(blueprints["FloorButton"].generated_class(), (-280, 0, 0), FACE_PLAYER, "BoutonSol", "Gameplay")
    floor_button.set_editor_property("builder", builder)

    door = _spawn(blueprints["LabDoor"].generated_class(), (ROOM_MAX_X + WALL / 2, 0, 0), FACE_PLAYER, "PorteLabo", "Gameplay")
    door.set_editor_property("builder", builder)

    _spawn(blueprints["ExitTrigger"].generated_class(), (1000, 0, 150), FACE_PLAYER, "Sortie", "Gameplay")
    _spawn(unreal.PlayerStart, (-800, 0, 100), 0.0, "PlayerStart", "Gameplay")

    build_config_console(blueprints, mats, builder)
    build_help_panel(mats)

    _text("Titre", (ROOM_MAX_X - 3, 0, 372), "NEURON FORGE  -  Laboratoire de deep learning", 18, (140, 220, 255))
    return builder


def build_config_console(blueprints, mats, builder):
    """Panneau de configuration à droite de l'holotable (côté +Y), face au joueur."""
    board_x, board_center_y, board_center_z = -60.0, 430.0, 140.0
    _box("Console_Configuration", (board_x, board_center_y, board_center_z), (10, 400, 250), mats["Console"], "Gameplay/Console")

    button_x = board_x - 5 - 4
    text_x = board_x - 6
    button_class = blueprints["ConfigButton"].generated_class()
    actions = unreal.NFConfigAction

    def button(label, y, z, action, layer=0):
        actor = _spawn(button_class, (button_x, y, z), FACE_PLAYER, label, "Gameplay/Console")
        actor.set_editor_property("builder", builder)
        actor.set_editor_property("action", action)
        actor.set_editor_property("layer_index", layer)
        return actor

    _text("Console_Titre", (text_x, board_center_y, 250), "CONFIGURATION DU RÉSEAU", 11, folder="Gameplay/Console")

    # Colonne de gauche : couches et taux d'apprentissage
    global_y = 280.0
    _text("Console_Reseau", (text_x, global_y, 222), "Couches", 7, folder="Gameplay/Console")
    button("Btn_AjouterCouche", global_y, 190, actions.ADD_LAYER)
    button("Btn_RetirerCouche", global_y, 155, actions.REMOVE_LAYER)
    _text("Console_LR", (text_x, global_y, 125), "Taux d'appr.", 6, folder="Gameplay/Console")
    button("Btn_LR_Plus", global_y, 98, actions.INCREASE_LEARNING_RATE)
    button("Btn_LR_Moins", global_y, 63, actions.DECREASE_LEARNING_RATE)

    # Une colonne par couche cachée : + / - / activation
    for layer in range(3):
        y = 390.0 + layer * 95.0
        _text("Console_Couche{}".format(layer + 1), (text_x, y, 222), "Couche {}".format(layer + 1), 7, folder="Gameplay/Console")
        button("Btn_C{}_PlusNeurone".format(layer + 1), y, 190, actions.ADD_NEURON, layer)
        button("Btn_C{}_MoinsNeurone".format(layer + 1), y, 155, actions.REMOVE_NEURON, layer)
        button("Btn_C{}_Activation".format(layer + 1), y, 115, actions.NEXT_ACTIVATION, layer)
    _text("Console_Legende", (text_x, 485, 62), "+ / -  : neurones      bas : activation", 5.5, (170, 190, 210), folder="Gameplay/Console")


def build_help_panel(mats):
    """Panneau « mode d'emploi » à gauche de l'holotable (côté -Y)."""
    board_x, board_y = -60.0, -430.0
    _box("Panneau_Aide", (board_x, board_y, 140), (10, 400, 250), mats["Console"], "Gameplay/Aide")
    text_x = board_x - 6
    left = board_y - 185
    _text("Aide_Titre", (text_x, board_y, 250), "MODE D'EMPLOI", 11, folder="Gameplay/Aide")
    lines = [
        ("1. Configurez le réseau sur le panneau de droite", (220, 235, 255)),
        ("    (visez un bouton et appuyez sur E)", (170, 190, 210)),
        ("2. Marchez sur le bouton au sol pour l'entraîner", (220, 235, 255)),
        ("3. Atteignez 100 % de précision : la porte s'ouvre", (220, 235, 255)),
        ("", (0, 0, 0)),
        ("Couleur des neurones = activation :", (220, 235, 255)),
        ("", (0, 0, 0)),  # ligne de la légende colorée ci-dessous
        ("Poids :  bleu = positif,  rouge = négatif", (220, 235, 255)),
        ("Épaisseur du lien = force du poids", (170, 190, 210)),
        ("Luminosité du neurone = son activation", (170, 190, 210)),
    ]
    for i, (line, color) in enumerate(lines):
        if line:
            _text("Aide_Ligne{}".format(i), (text_x, left, 215 - i * 19), line, 7, color, folder="Gameplay/Aide",
                  align=unreal.HorizTextAligment.EHTA_LEFT)

    # Légende colorée des activations (mêmes couleurs que NFActivation::GetColor)
    legend = [("Linear", (51, 153, 255)), ("ReLU", (255, 115, 13)), ("Sigmoid", (26, 255, 77)), ("Tanh", (191, 51, 255))]
    for i, (name, color) in enumerate(legend):
        _text("Aide_Couleur_" + name, (text_x - 1, left + 60 + i * 70, 215 - 6 * 19), name, 7.5, color, folder="Gameplay/Aide")



def _trim(label, center, size, mats, soft=False, folder="Décor/Liserés"):
    return _box(label, center, size, mats["TrimSoft" if soft else "Trim"], folder)


def _frame(label, center_x, center_y, center_z, width, height, mats, thickness=3.0):
    """Cadre lumineux autour d'un panneau vertical orienté vers -X."""
    x = center_x - 6
    _trim(label + "_Haut", (x, center_y, center_z + height / 2), (2, width, thickness), mats)
    _trim(label + "_Bas", (x, center_y, center_z - height / 2), (2, width, thickness), mats)
    _trim(label + "_G", (x, center_y - width / 2, center_z), (2, thickness, height), mats)
    _trim(label + "_D", (x, center_y + width / 2, center_z), (2, thickness, height), mats)


def _holo_prop(label, location, mesh_path, material, scale, yaw_rate=0.0, bob=0.0, builder=None):
    prop = _spawn(unreal.NFAnimatedProp, location, 0.0, label, "Décor/Hologrammes")
    prop.set_editor_property("rotation_rate", unreal.Rotator(roll=0, pitch=0, yaw=yaw_rate))
    prop.set_editor_property("bob_amplitude", bob)
    if builder:
        prop.set_editor_property("builder", builder)
    mesh = prop.get_editor_property("mesh")
    mesh.set_static_mesh(eal.load_asset(mesh_path))
    mesh.set_material(0, material)
    mesh.set_relative_scale3d(_vec(*scale))
    return prop


def build_decor(mats, builder):
    """Décor sci-fi : liserés, piliers, baies de serveurs, hologrammes et ambiance réactive."""
    length = ROOM_MAX_X - ROOM_MIN_X
    center_x = (ROOM_MIN_X + ROOM_MAX_X) / 2.0

    # Liserés au pied et en haut des murs
    for tag, z, soft in (("Plinthe", 6.0, False), ("Corniche", ROOM_HEIGHT - 8.0, True)):
        _trim(tag + "_Nord", (center_x, ROOM_HALF_Y - 2, z), (length, 4, 4), mats, soft)
        _trim(tag + "_Sud", (center_x, -ROOM_HALF_Y + 2, z), (length, 4, 4), mats, soft)
        _trim(tag + "_Entree", (ROOM_MIN_X + 2, 0, z), (4, 2 * ROOM_HALF_Y, 4), mats, soft)

    # Piliers métalliques avec liseré vertical
    for side, sign in (("Nord", 1), ("Sud", -1)):
        for i, x in enumerate((-760, -360, 240)):
            y = sign * (ROOM_HALF_Y - 20)
            _box("Pilier_{}_{}".format(side, i), (x, y, ROOM_HEIGHT / 2), (50, 40, ROOM_HEIGHT), mats["Pillar"], "Décor/Piliers")
            _trim("Pilier_{}_{}_Lumiere".format(side, i), (x, y - sign * 21, ROOM_HEIGHT / 2), (6, 2, ROOM_HEIGHT - 60), mats)

    # Baies de serveurs le long des murs latéraux, façades couvertes de LED
    for side, sign in (("Nord", 1), ("Sud", -1)):
        for i, x in enumerate((-660, -580, -500, -260, -180)):
            y = sign * (ROOM_HALF_Y - 45)
            _box("Baie_{}_{}".format(side, i), (x, y, 110), (72, 80, 220), mats["Rack"], "Décor/Serveurs")
            _box("Baie_{}_{}_LED".format(side, i), (x, y - sign * 41, 115), (62, 2, 190), mats["ServerLEDs"], "Décor/Serveurs")
            _trim("Baie_{}_{}_Haut".format(side, i), (x, y - sign * 41, 216), (72, 3, 3), mats, True, "Décor/Serveurs")

    # Encadrement lumineux de la porte (côté labo)
    door_x = ROOM_MAX_X - 1
    half = DOOR_WIDTH / 2 + DOOR_FRAME + 8
    height = DOOR_HEIGHT + DOOR_FRAME
    _trim("Porte_Lisere_G", (door_x, half, height / 2), (2, 5, height), mats, folder="Décor/Porte")
    _trim("Porte_Lisere_D", (door_x, -half, height / 2), (2, 5, height), mats, folder="Décor/Porte")
    _trim("Porte_Lisere_Haut", (door_x, 0, height + 6), (2, 2 * half, 5), mats, folder="Décor/Porte")

    # Cadres lumineux des deux panneaux (console et mode d'emploi)
    _frame("Cadre_Console", -60, 430, 140, 400, 250, mats)
    _frame("Cadre_Aide", -60, -430, 140, 400, 250, mats)

    # Anneau lumineux autour du bouton au sol
    for k in range(24):
        angle = 2 * math.pi * k / 24
        _box("Anneau_Sol_{}".format(k), (-280 + 95 * math.cos(angle), 95 * math.sin(angle), 1.5), (24, 3, 1),
             mats["TrimSoft"], "Décor/Bouton")

    # Hologrammes de l'holotable : socle à ondes, anneau flottant, cubes de données en orbite
    _holo_prop("Holo_Socle", (0, 0, 81), "/Engine/BasicShapes/Plane.Plane", mats["Holo"], (1.15, 3.1, 1.0))
    _holo_prop("Holo_Anneau", (0, 0, 300), "/Engine/BasicShapes/Plane.Plane", mats["HoloRing"], (2.6, 2.6, 1.0),
               yaw_rate=12.0, bob=6.0, builder=builder)

    orbit = _spawn(unreal.NFAnimatedProp, (0, 0, 185), 0.0, "Holo_Donnees", "Décor/Hologrammes")
    orbit.set_editor_property("rotation_rate", unreal.Rotator(roll=0, pitch=0, yaw=0))
    orbit.set_editor_property("orbit_count", 18)
    orbit.set_editor_property("orbit_radius", 175.0)
    orbit.set_editor_property("orbit_speed", 14.0)
    orbit.set_editor_property("orbit_wave", 18.0)
    orbit.set_editor_property("orbit_item_size", 5.0)
    orbit.set_editor_property("builder", builder)
    orbit_mesh = orbit.get_editor_property("orbit")
    orbit_mesh.set_static_mesh(eal.load_asset(CUBE))
    orbit_mesh.set_material(0, mats["Trim"])

    # Chef d'orchestre de l'ambiance (couleurs, lumières, hologrammes)
    ambience = _spawn(unreal.NFLabAmbience, (0, 0, 350), 0.0, "Ambiance", "Gameplay")
    ambience.set_editor_property("builder", builder)
    ambience.set_editor_property("mood_collection", mats["MoodCollection"])


def build_level(blueprints, mats):
    """Recrée L_Lab, construit tout le niveau et l'enregistre."""
    if eal.does_asset_exist(LEVEL_PATH):
        eal.delete_asset(LEVEL_PATH)
    if not les.new_level(LEVEL_PATH):
        raise RuntimeError("Création du niveau impossible : " + LEVEL_PATH)

    build_room(mats)
    build_lighting()
    builder = build_lab(blueprints, mats)
    build_decor(mats, builder)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", blueprints["GameMode"].generated_class())

    if not les.save_current_level():
        raise RuntimeError("Enregistrement du niveau impossible")
    return builder
