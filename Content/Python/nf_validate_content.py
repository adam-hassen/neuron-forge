"""Neuron Forge - vérifie le contenu généré par nf_build_all.py (à lancer dans un nouveau processus).

Contrôle que les réglages ont bien été enregistrés : materials et sons des Blueprints,
acteurs du niveau L_Lab et leurs références à l'holotable, GameMode du niveau.
Rapport : Saved/nf_validate_report.txt (dernière ligne : RÉSULTAT OK / ÉCHEC).
"""
import os

import unreal

eal = unreal.EditorAssetLibrary
BP = "/Game/NeuronForge/Blueprints/"
lines = []
failures = []


def check(label, condition, detail=""):
    lines.append("[{}] {}{}".format("OK" if condition else "ÉCHEC", label, (" : " + str(detail)) if detail else ""))
    if not condition:
        failures.append(label)


def cdo_of(name):
    bp = eal.load_asset(BP + name)
    check("Blueprint " + name + " existe", bp is not None)
    return unreal.get_default_object(bp.generated_class()) if bp else None


def material_name(cdo, component_property):
    component = cdo.get_editor_property(component_property)
    material = component.get_material(0)
    return material.get_name() if material else None


def asset_name(obj):
    return obj.get_name() if obj else None


# --- Blueprints ------------------------------------------------------------

expected_materials = {
    "BP_NF_NetworkBuilder": {"table_mesh": "MI_NF_Table", "neuron_instances": "MI_NF_Neuron", "connection_instances": "MI_NF_Connection"},
    "BP_NF_ConfigButton": {"mesh": "MI_NF_Button"},
    "BP_NF_FloorButton": {"base_mesh": "MI_NF_FloorBase", "plate_mesh": "MI_NF_FloorPlate"},
    "BP_NF_StatusDisplay": {"screen_mesh": "MI_NF_Screen"},
    "BP_NF_LabDoor": {"frame_left": "MI_NF_DoorFrame", "frame_top": "MI_NF_DoorFrame", "panel_left": "MI_NF_DoorPanel", "panel_right": "MI_NF_DoorPanel"},
}
expected_sounds = {
    "BP_NF_NetworkBuilder": {"training_start_sound": "S_NF_TrainingStart", "training_failed_sound": "S_NF_Failure"},
    "BP_NF_ConfigButton": {"click_sound": "S_NF_Click"},
    "BP_NF_FloorButton": {"press_sound": "S_NF_FloorPress"},
    "BP_NF_LabDoor": {"validation_sound": "S_NF_Validation", "door_move_sound": "S_NF_DoorMove"},
    "BP_NF_ExitTrigger": {"completion_sound": "S_NF_LevelComplete"},
}

for bp_name, components in expected_materials.items():
    cdo = cdo_of(bp_name)
    if cdo:
        for component, expected in components.items():
            actual = material_name(cdo, component)
            check("{}.{} -> {}".format(bp_name, component, expected), actual == expected, actual)

for bp_name, props in expected_sounds.items():
    cdo = cdo_of(bp_name)
    if cdo:
        for prop, expected in props.items():
            actual = asset_name(cdo.get_editor_property(prop))
            check("{}.{} -> {}".format(bp_name, prop, expected), actual == expected, actual)

gm = cdo_of("BP_NF_GameMode")
if gm:
    pawn = gm.get_editor_property("default_pawn_class")
    check("BP_NF_GameMode.default_pawn_class -> BP_NF_Character", pawn is not None and pawn.get_name() == "BP_NF_Character_C", pawn)

for sound in ["S_NF_Validation", "S_NF_Click", "S_NF_DoorMove"]:
    wave = eal.load_asset("/Game/NeuronForge/Audio/" + sound)
    check("Son {} importé".format(sound), wave is not None and wave.get_editor_property("duration") > 0,
          wave.get_editor_property("duration") if wave else None)

# --- Niveau ----------------------------------------------------------------

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
check("Chargement de L_Lab", les.load_level("/Game/NeuronForge/Maps/L_Lab"))

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def of_class(class_name):
    return [a for a in actors if a.get_class().get_name() == class_name]


builders = of_class("BP_NF_NetworkBuilder_C")
check("1 holotable", len(builders) == 1, len(builders))
builder = builders[0] if builders else None

buttons = of_class("BP_NF_ConfigButton_C")
check("13 boutons de configuration", len(buttons) == 13, len(buttons))
check("Tous les boutons pointent vers l'holotable", all(b.get_editor_property("builder") == builder for b in buttons))

for class_name in ["BP_NF_FloorButton_C", "BP_NF_StatusDisplay_C", "BP_NF_LabDoor_C"]:
    found = of_class(class_name)
    check("1 " + class_name, len(found) == 1, len(found))
    if found:
        check(class_name + " pointe vers l'holotable", found[0].get_editor_property("builder") == builder)

check("1 zone de sortie", len(of_class("BP_NF_ExitTrigger_C")) == 1)
check("1 PlayerStart", len(of_class("PlayerStart")) == 1)
check("Lumières présentes", len(of_class("PointLight")) >= 8, len(of_class("PointLight")))

game_mode = world.get_world_settings().get_editor_property("default_game_mode")
check("GameMode du niveau -> BP_NF_GameMode", game_mode is not None and game_mode.get_name() == "BP_NF_GameMode_C", game_mode)

actions = sorted(str(b.get_editor_property("action")) for b in buttons)
lines.append("Actions des boutons : " + ", ".join(actions))
lines.append("Total acteurs : {}".format(len(actors)))
lines.append("RÉSULTAT " + ("OK" if not failures else "ÉCHEC ({} problème(s))".format(len(failures))))

with open(os.path.join(unreal.Paths.project_saved_dir(), "nf_validate_report.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
