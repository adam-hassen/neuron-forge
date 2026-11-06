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


def _point_light(label, location, intensity, radius, color=(1.0, 0.97, 0.92), shadows=True):
    actor = _spawn(unreal.PointLight, location, label=label, folder="Lumières")
    light = actor.point_light_component
    light.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    light.set_intensity(intensity)
    light.set_attenuation_radius(radius)
    light.set_light_color(unreal.LinearColor(*color, 1.0))
    light.set_cast_shadows(shadows)
    return actor


# --- Construction ----------------------------------------------------------

def build_room(mats):
    wall_mat = eal.load_asset(WALL_MATERIAL)
    floor_mat = eal.load_asset(FLOOR_MATERIAL)
    length = ROOM_MAX_X - ROOM_MIN_X
    center_x = (ROOM_MIN_X + ROOM_MAX_X) / 2.0
    width = 2 * ROOM_HALF_Y

    _box("Sol", (center_x, 0, -WALL / 2), (length + 2 * WALL, width + 2 * WALL, WALL), floor_mat)
    _box("Plafond", (center_x, 0, ROOM_HEIGHT + WALL / 2), (length + 2 * WALL, width + 2 * WALL, WALL), wall_mat)
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
    _box("Couloir_Plafond", (corridor_x, 0, ROOM_HEIGHT + WALL / 2), (corridor_len, corridor_w + 2 * WALL, WALL), wall_mat, "Couloir")
    _box("Couloir_Mur_Nord", (corridor_x, CORRIDOR_HALF_Y + WALL / 2, ROOM_HEIGHT / 2), (corridor_len, WALL, ROOM_HEIGHT), wall_mat, "Couloir")
    _box("Couloir_Mur_Sud", (corridor_x, -CORRIDOR_HALF_Y - WALL / 2, ROOM_HEIGHT / 2), (corridor_len, WALL, ROOM_HEIGHT), wall_mat, "Couloir")
    _box("Couloir_Fond", (CORRIDOR_END_X + WALL / 2, 0, ROOM_HEIGHT / 2), (WALL, corridor_w + 2 * WALL, ROOM_HEIGHT), wall_mat, "Couloir")

    # Bandeaux lumineux au plafond
    for i, x in enumerate((-750, -350, 50, 400)):
        for j, y in enumerate((-350, 350)):
            _box("Bandeau_{}_{}".format(i, j), (x, y, ROOM_HEIGHT - 3), (260, 24, 6), mats["Light"], "Lumières")


def build_lighting():
    for i, x in enumerate((-750, -350, 50, 400)):
        for j, y in enumerate((-350, 350)):
            _point_light("Lumiere_{}_{}".format(i, j), (x, y, ROOM_HEIGHT - 40), 3000.0, 900.0, shadows=(i % 2 == 0))
    _point_light("Lumiere_Couloir", (950, 0, ROOM_HEIGHT - 40), 2500.0, 700.0, color=(0.75, 1.0, 0.8))

    sky = _spawn(unreal.SkyLight, (0, 0, 300), label="SkyLight", folder="Lumières")
    sky.light_component.set_intensity(0.4)

    ppv = _spawn(unreal.PostProcessVolume, (0, 0, 200), label="PostProcess", folder="Lumières")
    ppv.set_editor_property("unbound", True)
    settings = ppv.get_editor_property("settings")
    settings.set_editor_property("override_auto_exposure_min_brightness", True)
    settings.set_editor_property("auto_exposure_min_brightness", 0.0)
    settings.set_editor_property("override_auto_exposure_max_brightness", True)
    settings.set_editor_property("auto_exposure_max_brightness", 2.0)
    settings.set_editor_property("override_bloom_intensity", True)
    settings.set_editor_property("bloom_intensity", 0.9)
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

    _text("Titre", (ROOM_MAX_X - 2, 0, 375), "NEURON FORGE  -  Laboratoire de deep learning", 20, (140, 220, 255))
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


def build_level(blueprints, mats):
    """Recrée L_Lab, construit tout le niveau et l'enregistre."""
    if eal.does_asset_exist(LEVEL_PATH):
        eal.delete_asset(LEVEL_PATH)
    if not les.new_level(LEVEL_PATH):
        raise RuntimeError("Création du niveau impossible : " + LEVEL_PATH)

    build_room(mats)
    build_lighting()
    builder = build_lab(blueprints, mats)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", blueprints["GameMode"].generated_class())

    if not les.save_current_level():
        raise RuntimeError("Enregistrement du niveau impossible")
    return builder
