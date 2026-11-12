"""Neuron Forge - construction du niveau L_Lab : le grand hall de la Forge.

Plan (vue de dessus, cm). Le joueur entre côté -X et regarde vers +X (sa gauche = -Y).

  Y=+1500 ┌────────────── passerelle + baies de serveurs ──────────────┐
          │  [Mode d'emploi]                  [Levier]                 │
          │                   [Bouton sol] [FORGE]   [HOLOGRAMME]      ├──────┐
  spawn ● │                                                     Porte  Sortie │
          │       ATELIER : [modules] [orbes] [cristaux]               ├──────┘
          │                                          [Écran d'état]    │
  Y=-1500 └────────────── passerelle + baies de serveurs ──────────────┘
        X=-2250                                                    X=2250   X=2850

Le niveau est entièrement recréé à chaque exécution.
"""
import math

import unreal

LEVEL_PATH = "/Game/NeuronForge/Maps/L_Lab"
CUBE = "/Engine/BasicShapes/Cube.Cube"
SPHERE = "/Engine/BasicShapes/Sphere.Sphere"
CONE = "/Engine/BasicShapes/Cone.Cone"
PLANE = "/Engine/BasicShapes/Plane.Plane"

HALL_MIN_X, HALL_MAX_X = -2250.0, 2250.0
HALL_HALF_Y = 1500.0
HALL_HEIGHT = 1000.0
WALL = 30.0

DOOR_WIDTH, DOOR_HEIGHT, DOOR_FRAME = 240.0, 300.0, 25.0
CORRIDOR_END_X, CORRIDOR_HALF_Y, CORRIDOR_HEIGHT = 2850.0, 220.0, 420.0

FORGE_X = 0.0
HOLO_X = 950.0
HOLO_SCALE = 3.5
CATWALK_Z = 520.0
CATWALK_DEPTH = 260.0

# Les acteurs du labo sont conçus pour être regardés depuis leur axe +X :
# yaw 180 les tourne vers l'entrée du hall (-X).
FACE_PLAYER = 180.0

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eal = unreal.EditorAssetLibrary


# --- Utilitaires -------------------------------------------------------------

def _vec(x, y, z):
    return unreal.Vector(x, y, z)


def _rot(yaw=0.0, pitch=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def _spawn(actor_class, location, yaw=0.0, label=None, folder=None, pitch=0.0):
    actor = eas.spawn_actor_from_class(actor_class, _vec(*location), _rot(yaw, pitch))
    if actor is None:
        raise RuntimeError("Impossible de créer l'acteur {}".format(label or actor_class))
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    return actor


def _box(label, center, size, material, folder="Hall"):
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


def _point_light(label, location, intensity, radius, color=(0.8, 0.9, 1.0), shadows=True, mood=True):
    actor = _spawn(unreal.PointLight, location, label=label, folder="Lumières")
    light = actor.point_light_component
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    light.set_intensity(intensity)
    light.set_attenuation_radius(radius)
    light.set_light_color(unreal.LinearColor(*color, 1.0))
    light.set_cast_shadows(shadows)
    if mood:
        _mood_tag(actor)
    return actor


def _spot_light(label, location, pitch, intensity, radius, inner, outer, color=(1.0, 1.0, 1.0), mood=False, yaw=0.0):
    actor = _spawn(unreal.SpotLight, location, yaw=yaw, pitch=pitch, label=label, folder="Lumières")
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


def _trim(label, center, size, mats, soft=False, folder="Décor/Liserés"):
    return _box(label, center, size, mats["TrimSoft" if soft else "Trim"], folder)


def _frame(label, center_x, center_y, center_z, width, height, mats, thickness=3.0):
    """Cadre lumineux autour d'un panneau vertical orienté vers -X."""
    x = center_x - 8
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


# --- Structure du hall -------------------------------------------------------

def build_hall(mats):
    wall, floor, ceiling = mats["Wall"], mats["FloorGrid"], mats["Ceiling"]
    length = HALL_MAX_X - HALL_MIN_X
    cx = (HALL_MIN_X + HALL_MAX_X) / 2.0
    width = 2 * HALL_HALF_Y

    _box("Sol", (cx, 0, -WALL / 2), (length + 2 * WALL, width + 2 * WALL, WALL), floor)
    _box("Plafond", (cx, 0, HALL_HEIGHT + WALL / 2), (length + 2 * WALL, width + 2 * WALL, WALL), ceiling)
    _box("Mur_Entree", (HALL_MIN_X - WALL / 2, 0, HALL_HEIGHT / 2), (WALL, width + 2 * WALL, HALL_HEIGHT), wall)
    _box("Mur_Nord", (cx, HALL_HALF_Y + WALL / 2, HALL_HEIGHT / 2), (length, WALL, HALL_HEIGHT), wall)
    _box("Mur_Sud", (cx, -HALL_HALF_Y - WALL / 2, HALL_HEIGHT / 2), (length, WALL, HALL_HEIGHT), wall)

    # Mur du fond percé pour la porte
    opening = DOOR_WIDTH / 2 + DOOR_FRAME
    segment = HALL_HALF_Y + WALL - opening
    wall_x = HALL_MAX_X + WALL / 2
    _box("Mur_Porte_Gauche", (wall_x, opening + segment / 2, HALL_HEIGHT / 2), (WALL, segment, HALL_HEIGHT), wall)
    _box("Mur_Porte_Droite", (wall_x, -opening - segment / 2, HALL_HEIGHT / 2), (WALL, segment, HALL_HEIGHT), wall)
    lintel = HALL_HEIGHT - (DOOR_HEIGHT + DOOR_FRAME)
    _box("Mur_Porte_Linteau", (wall_x, 0, HALL_HEIGHT - lintel / 2), (WALL, 2 * opening, lintel), wall)

    # Couloir de sortie
    start = HALL_MAX_X + WALL
    corridor_len = CORRIDOR_END_X - start
    corridor_x = start + corridor_len / 2
    cw = 2 * CORRIDOR_HALF_Y
    _box("Couloir_Sol", (corridor_x, 0, -WALL / 2), (corridor_len, cw + 2 * WALL, WALL), floor, "Couloir")
    _box("Couloir_Plafond", (corridor_x, 0, CORRIDOR_HEIGHT + WALL / 2), (corridor_len, cw + 2 * WALL, WALL), ceiling, "Couloir")
    _box("Couloir_Mur_Nord", (corridor_x, CORRIDOR_HALF_Y + WALL / 2, CORRIDOR_HEIGHT / 2), (corridor_len, WALL, CORRIDOR_HEIGHT), wall, "Couloir")
    _box("Couloir_Mur_Sud", (corridor_x, -CORRIDOR_HALF_Y - WALL / 2, CORRIDOR_HEIGHT / 2), (corridor_len, WALL, CORRIDOR_HEIGHT), wall, "Couloir")
    _box("Couloir_Fond", (CORRIDOR_END_X + WALL / 2, 0, CORRIDOR_HEIGHT / 2), (WALL, cw + 2 * WALL, CORRIDOR_HEIGHT), wall, "Couloir")

    # Bandeaux lumineux au plafond
    for i, x in enumerate((-1700, -850, 0, 850, 1700)):
        for j, y in enumerate((-900, 0, 900)):
            _box("Bandeau_{}_{}".format(i, j), (x, y, HALL_HEIGHT - 4), (500, 30, 8), mats["Light"], "Lumières")


def build_architecture(mats):
    """Piliers, passerelles, liserés et baies de serveurs."""
    length = HALL_MAX_X - HALL_MIN_X
    cx = (HALL_MIN_X + HALL_MAX_X) / 2.0

    for tag, z, soft in (("Plinthe", 8.0, False), ("Corniche", HALL_HEIGHT - 12.0, True)):
        _trim(tag + "_Nord", (cx, HALL_HALF_Y - 3, z), (length, 5, 5), mats, soft)
        _trim(tag + "_Sud", (cx, -HALL_HALF_Y + 3, z), (length, 5, 5), mats, soft)
        _trim(tag + "_Entree", (HALL_MIN_X + 3, 0, z), (5, 2 * HALL_HALF_Y, 5), mats, soft)

    for side, sign in (("Nord", 1), ("Sud", -1)):
        # Passerelle en hauteur avec garde-corps lumineux
        y_catwalk = sign * (HALL_HALF_Y - CATWALK_DEPTH / 2)
        _box("Passerelle_" + side, (cx, y_catwalk, CATWALK_Z), (length, CATWALK_DEPTH, 20), mats["Pillar"], "Décor/Passerelles")
        edge = sign * (HALL_HALF_Y - CATWALK_DEPTH)
        _trim("Passerelle_{}_Bord".format(side), (cx, edge, CATWALK_Z - 12), (length, 4, 4), mats, folder="Décor/Passerelles")
        _trim("Passerelle_{}_Rambarde".format(side), (cx, edge, CATWALK_Z + 105), (length, 5, 5), mats, True, "Décor/Passerelles")
        for k in range(18):
            x = HALL_MIN_X + 125 + k * 250
            _box("Passerelle_{}_Poteau{}".format(side, k), (x, edge, CATWALK_Z + 55), (6, 6, 100), mats["Pillar"], "Décor/Passerelles")

        # Piliers porteurs avec liseré vertical
        for i, x in enumerate((-1875, -1125, -375, 375, 1125, 1875)):
            y = sign * (HALL_HALF_Y - 35)
            _box("Pilier_{}_{}".format(side, i), (x, y, HALL_HEIGHT / 2), (70, 70, HALL_HEIGHT), mats["Pillar"], "Décor/Piliers")
            _trim("Pilier_{}_{}_Lumiere".format(side, i), (x, y - sign * 36, HALL_HEIGHT / 2), (8, 2, HALL_HEIGHT - 80), mats)

        # Baies de serveurs sous les passerelles
        for g, start in enumerate((-1750, -950, 600, 1350)):
            for i in range(5):
                x = start + i * 85
                y = sign * (HALL_HALF_Y - 60)
                _box("Baie_{}_{}_{}".format(side, g, i), (x, y, 120), (76, 90, 240), mats["Rack"], "Décor/Serveurs")
                _box("Baie_{}_{}_{}_LED".format(side, g, i), (x, y - sign * 46, 125), (66, 2, 210), mats["ServerLEDs"], "Décor/Serveurs")
                _trim("Baie_{}_{}_{}_Haut".format(side, g, i), (x, y - sign * 46, 236), (76, 3, 3), mats, True, "Décor/Serveurs")

    # Encadrement lumineux de la porte
    door_x = HALL_MAX_X - 1
    half = DOOR_WIDTH / 2 + DOOR_FRAME + 10
    height = DOOR_HEIGHT + DOOR_FRAME
    _trim("Porte_Lisere_G", (door_x, half, height / 2), (2, 6, height), mats, folder="Décor/Porte")
    _trim("Porte_Lisere_D", (door_x, -half, height / 2), (2, 6, height), mats, folder="Décor/Porte")
    _trim("Porte_Lisere_Haut", (door_x, 0, height + 8), (2, 2 * half, 6), mats, folder="Décor/Porte")


def build_lighting():
    for i, x in enumerate((-1700, -850, 0, 850, 1700)):
        for j, y in enumerate((-900, 900)):
            _point_light("Lumiere_{}_{}".format(i, j), (x, y, HALL_HEIGHT - 60), 2600.0, 1700.0, shadows=(i % 2 == 0))
    _point_light("Lumiere_Couloir", (2550, 0, CORRIDOR_HEIGHT - 40), 1800.0, 800.0, color=(0.75, 1.0, 0.8))

    _spot_light("Spot_Forge", (FORGE_X - 150, 0, HALL_HEIGHT - 10), -80.0, 4000.0, 1500.0, 30.0, 48.0, (0.85, 0.95, 1.0))
    _spot_light("Spot_Hologramme", (HOLO_X, 0, HALL_HEIGHT - 10), -90.0, 5000.0, 1500.0, 25.0, 40.0, (0.6, 0.85, 1.0), mood=True)
    _spot_light("Spot_Atelier", (-850, -1000, HALL_HEIGHT - 10), -90.0, 3500.0, 1400.0, 32.0, 50.0, (1.0, 0.95, 0.85))
    _spot_light("Spot_Levier", (-150, 900, HALL_HEIGHT - 10), -80.0, 1500.0, 1300.0, 15.0, 28.0, (1.0, 0.85, 0.6))
    _spot_light("Spot_Porte", (1800, 0, HALL_HEIGHT - 300), -25.0, 2000.0, 1200.0, 18.0, 35.0, (0.8, 0.9, 1.0), mood=True)

    sky = _spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight", folder="Lumières")
    sky.light_component.set_intensity(0.15)

    fog = _spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Brume", folder="Lumières")
    fog_component = fog.get_editor_property("component")
    fog_component.set_editor_property("fog_density", 0.01)
    fog_component.set_editor_property("fog_height_falloff", 0.3)
    fog_component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.005, 0.01, 0.02, 1.0))
    fog_component.set_editor_property("enable_volumetric_fog", True)
    fog_component.set_editor_property("volumetric_fog_extinction_scale", 0.6)

    ppv = _spawn(unreal.PostProcessVolume, (0, 0, 300), label="PostProcess", folder="Lumières")
    ppv.set_editor_property("unbound", True)
    settings = ppv.get_editor_property("settings")
    settings.set_editor_property("override_auto_exposure_min_brightness", True)
    settings.set_editor_property("auto_exposure_min_brightness", 1.1)
    settings.set_editor_property("override_auto_exposure_max_brightness", True)
    settings.set_editor_property("auto_exposure_max_brightness", 1.5)
    settings.set_editor_property("override_bloom_intensity", True)
    settings.set_editor_property("bloom_intensity", 1.3)
    settings.set_editor_property("override_vignette_intensity", True)
    settings.set_editor_property("vignette_intensity", 0.5)
    ppv.set_editor_property("settings", settings)


# --- Gameplay ----------------------------------------------------------------

def build_gameplay(blueprints, mats, sounds):
    """Hologramme, Forge, levier, bouton au sol, écran, porte, sortie. Renvoie l'holotable."""
    builder = _spawn(blueprints["NetworkBuilder"].generated_class(), (HOLO_X, 0, 0), FACE_PLAYER, "Hologramme", "Gameplay")
    builder.set_actor_scale3d(_vec(HOLO_SCALE, HOLO_SCALE, HOLO_SCALE))
    # Le niveau jouable utilise la serre (XOR reste pour les tests)
    builder.set_editor_property("problem", unreal.NFProblem.GREENHOUSE)
    builder.set_editor_property("learning_rate_index", 2)

    forge = _spawn(unreal.NFForge, (FORGE_X, 0, 0), FACE_PLAYER, "Forge", "Gameplay")
    forge.set_editor_property("builder", builder)
    forge.set_editor_property("platform_material", mats["Platform"])
    forge.set_editor_property("rail_material", mats["TrimSoft"])
    forge.set_editor_property("pylon_material", mats["Pillar"])
    _text("Forge_Titre", (FORGE_X - 10, 0, 340), "LA FORGE", 34, (140, 220, 255), folder="Gameplay")
    _text("Forge_SousTitre", (FORGE_X - 10, 0, 300), "Construisez le modèle qui prédit si une plante survit", 12, (170, 195, 220), folder="Gameplay")

    lever = _spawn(unreal.NFLever, (FORGE_X, 900, 50), FACE_PLAYER, "Levier", "Gameplay")
    lever.set_editor_property("builder", builder)
    lever.set_editor_property("body_material", mats["Pillar"])
    lever.set_editor_property("handle_material", mats["Module"])
    if "S_NF_Click" in sounds:
        lever.set_editor_property("pull_sound", sounds["S_NF_Click"])

    floor_button = _spawn(blueprints["FloorButton"].generated_class(), (FORGE_X - 450, 0, 0), FACE_PLAYER, "BoutonSol", "Gameplay")
    floor_button.set_editor_property("builder", builder)
    for k in range(24):
        angle = 2 * math.pi * k / 24
        _box("Anneau_Sol_{}".format(k), (FORGE_X - 450 + 95 * math.cos(angle), 95 * math.sin(angle), 1.5), (24, 3, 1),
             mats["TrimSoft"], "Décor/Bouton")

    display = _spawn(blueprints["StatusDisplay"].generated_class(), (HOLO_X - 200, -1050, 330), FACE_PLAYER, "EcranEtat", "Gameplay")
    display.set_actor_scale3d(_vec(1.8, 1.8, 1.8))
    display.set_editor_property("builder", builder)
    display.set_editor_property("mission_title", "MISSION : LA SERRE DU LABO")

    # Carnet du labo : en face de l'écran d'état, de l'autre côté de l'hologramme
    notebook = _spawn(unreal.NFLabNotebook, (HOLO_X - 200, 1080, 380), FACE_PLAYER, "CarnetDuLabo", "Gameplay")
    notebook.set_editor_property("builder", builder)
    notebook.set_editor_property("chart_material", mats["Chart"])
    notebook.set_editor_property("board_material", mats["Console"])
    _frame("Cadre_Carnet", HOLO_X - 200 + 4, 1080, 380, 860, 520, mats)

    door = _spawn(blueprints["LabDoor"].generated_class(), (HALL_MAX_X + WALL / 2, 0, 0), FACE_PLAYER, "PorteLabo", "Gameplay")
    door.set_editor_property("builder", builder)

    _spawn(blueprints["ExitTrigger"].generated_class(), (2550, 0, 150), FACE_PLAYER, "Sortie", "Gameplay")
    _spawn(unreal.PlayerStart, (HALL_MIN_X + 250, 0, 100), 0.0, "PlayerStart", "Gameplay")

    _text("Titre", (HALL_MAX_X - 3, 0, 495), "NEURON FORGE", 60, (140, 220, 255))
    _text("Titre_Sous", (HALL_MAX_X - 3, 0, 435), "Laboratoire de deep learning", 20, (170, 200, 230))
    return builder


def _bench(label, x, y, length, mats, title, subtitle):
    """Établi de l'atelier avec rebords (les pièces ne roulent pas par terre) et enseigne."""
    _box(label, (x, y, 45), (120, length, 90), mats["Bench"], "Atelier")
    _trim(label + "_Lisere", (x - 61, y, 88), (2, length, 3), mats, folder="Atelier")
    rims = {"Avant": (-58, 0, 4, length), "Arriere": (58, 0, 4, length),
            "G": (0, -length / 2 + 2, 120, 4), "D": (0, length / 2 - 2, 120, 4)}
    for name, (dx, dy, sx, sy) in rims.items():
        _box("{}_Rebord{}".format(label, name), (x + dx, y + dy, 98), (sx, sy, 16), mats["Bench"], "Atelier")
    _text(label + "_Titre", (x - 40, y, 230), title, 20, (140, 220, 255), folder="Atelier")
    _text(label + "_Detail", (x - 40, y, 200), subtitle, 10, (180, 200, 220), folder="Atelier")


def _piece(actor_class, label, location, mats, material_key, piece_type=None, activation=None, mesh=None, scale=None):
    piece = _spawn(actor_class, location, 0.0, label, "Atelier/Pièces")
    if piece_type is not None:
        piece.set_editor_property("piece_type", piece_type)
    if activation is not None:
        piece.set_editor_property("activation", activation)
    piece.set_editor_property("body_material", mats[material_key])
    component = piece.get_editor_property("mesh")
    if mesh:
        component.set_static_mesh(eal.load_asset(mesh))
    component.set_material(0, mats[material_key])
    if scale:
        piece.set_actor_scale3d(_vec(*scale))
    return piece


def build_workshop(mats):
    """Atelier : établis garnis de modules de couche, d'orbes-neurones et de cristaux d'activation."""
    y = -1050.0
    _bench("Etabli_Modules", -1400, y, 420, mats, "MODULES DE COUCHE", "1 module = 1 couche cachée")
    _bench("Etabli_Orbes", -850, y, 520, mats, "ORBES-NEURONES", "1 orbe posée autour d'un module = 1 neurone")
    _bench("Etabli_Cristaux", -300, y, 420, mats, "CRISTAUX D'ACTIVATION", "Plantez-en un sur un module")

    for i in range(4):
        _piece(unreal.NFLayerModule, "Module_{}".format(i + 1), (-1400, y - 150 + i * 100, 125), mats, "Module")

    orb = unreal.NFPieceType.NEURON_ORB
    for i in range(20):
        row, col = divmod(i, 10)
        _piece(unreal.NFPiece, "Orbe_{:02d}".format(i + 1), (-850 - 20 + row * 40, y - 225 + col * 50, 110), mats, "Orb",
               piece_type=orb, mesh=SPHERE, scale=(0.16, 0.16, 0.16))

    crystal = unreal.NFPieceType.ACTIVATION_CRYSTAL
    activations = [("Linear", unreal.NFActivation.LINEAR, (51, 153, 255)), ("ReLU", unreal.NFActivation.RE_LU, (255, 115, 13)),
                   ("Sigmoid", unreal.NFActivation.SIGMOID, (26, 255, 77)), ("Tanh", unreal.NFActivation.TANH, (191, 51, 255))]
    for i, (name, activation, color) in enumerate(activations):
        for k in range(2):
            _piece(unreal.NFPiece, "Cristal_{}_{}".format(name, k + 1), (-300 - 20 + k * 40, y - 150 + i * 100, 120), mats,
                   "Crystal", piece_type=crystal, activation=activation, mesh=CONE, scale=(0.14, 0.14, 0.28))
        _text("Cristal_{}_Etiquette".format(name), (-362, y - 150 + i * 100, 140), name, 9, color, folder="Atelier")


def build_help_panel(mats):
    """Panneau « mode d'emploi » près de l'entrée."""
    x, y, z = -1650.0, 700.0, 200.0
    _box("Panneau_Aide", (x, y, z), (12, 520, 300), mats["Console"], "Gameplay/Aide")
    _frame("Cadre_Aide", x, y, z, 520, 300, mats)
    text_x = x - 8
    left = y - 240
    _text("Aide_Titre", (text_x, y, z + 125), "MODE D'EMPLOI", 16, folder="Gameplay/Aide")
    lines = [
        ("1. À l'atelier, visez une pièce et appuyez sur E pour la prendre", (220, 235, 255)),
        ("2. Insérez un module de couche dans une fente de la Forge", (220, 235, 255)),
        ("3. Posez des orbes autour du module : 1 orbe = 1 neurone", (220, 235, 255)),
        ("4. Plantez un cristal sur le module pour choisir l'activation", (220, 235, 255)),
        ("5. Tirez le levier pour régler le taux d'apprentissage", (220, 235, 255)),
        ("6. Marchez sur le bouton au sol : l'entraînement démarre", (220, 235, 255)),
        ("", (0, 0, 0)),
        ("Sans cristal, un module est Linear.", (170, 190, 210)),
        ("Pour retirer une pièce, visez-la et appuyez sur E.", (170, 190, 210)),
        ("H : revoir le briefing", (170, 190, 210)),
    ]
    for i, (line, color) in enumerate(lines):
        if line:
            _text("Aide_Ligne{}".format(i), (text_x, left, z + 85 - i * 24), line, 9, color, folder="Gameplay/Aide",
                  align=unreal.HorizTextAligment.EHTA_LEFT)


def build_holograms(mats, builder):
    s = HOLO_SCALE
    _holo_prop("Holo_Socle", (HOLO_X, 0, 81 * s), PLANE, mats["Holo"], (1.15 * s, 3.1 * s, 1.0))
    _holo_prop("Holo_Anneau", (HOLO_X, 0, 935), PLANE, mats["HoloRing"], (9.5, 9.5, 1.0), yaw_rate=10.0, bob=12.0, builder=builder)
    _holo_prop("Holo_Anneau_Forge", (FORGE_X, 0, 900), PLANE, mats["HoloRing"], (11.0, 11.0, 1.0), yaw_rate=-6.0, bob=8.0, builder=builder)

    orbit = _spawn(unreal.NFAnimatedProp, (HOLO_X, 0, 180 * s), 0.0, "Holo_Donnees", "Décor/Hologrammes")
    orbit.set_editor_property("rotation_rate", unreal.Rotator(roll=0, pitch=0, yaw=0))
    orbit.set_editor_property("orbit_count", 30)
    orbit.set_editor_property("orbit_radius", 680.0)
    orbit.set_editor_property("orbit_speed", 10.0)
    orbit.set_editor_property("orbit_wave", 60.0)
    orbit.set_editor_property("orbit_item_size", 13.0)
    orbit.set_editor_property("builder", builder)
    orbit_mesh = orbit.get_editor_property("orbit")
    orbit_mesh.set_static_mesh(eal.load_asset(CUBE))
    orbit_mesh.set_material(0, mats["Trim"])

    ambience = _spawn(unreal.NFLabAmbience, (0, 0, 600), 0.0, "Ambiance", "Gameplay")
    ambience.set_editor_property("builder", builder)
    ambience.set_editor_property("mood_collection", mats["MoodCollection"])


def build_level(blueprints, mats, sounds=None):
    """Recrée L_Lab, construit tout le hall et l'enregistre."""
    if eal.does_asset_exist(LEVEL_PATH):
        eal.delete_asset(LEVEL_PATH)
    if not les.new_level(LEVEL_PATH):
        raise RuntimeError("Création du niveau impossible : " + LEVEL_PATH)

    build_hall(mats)
    build_architecture(mats)
    build_lighting()
    builder = build_gameplay(blueprints, mats, sounds or {})
    build_workshop(mats)
    build_help_panel(mats)
    build_holograms(mats, builder)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", blueprints["GameMode"].generated_class())

    if not les.save_current_level():
        raise RuntimeError("Enregistrement du niveau impossible")
    return builder
