"""Neuron Forge - vérifie l'environnement : Python headless, assets du template
et classes C++ du jeu.

Écrit le résultat dans Saved/nf_check_env.txt (unreal.log n'est pas toujours
redirigé vers stdout en mode commandlet).
"""
import os
import unreal

lines = [
    "python=OK",
    "engine=" + unreal.SystemLibrary.get_engine_version(),
    "firstperson_bp=" + str(unreal.EditorAssetLibrary.does_asset_exist(
        "/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter")),
]

# Classes C++ exposées par le module ProjetNeuro
for class_name in ["NFCharacter", "NFPlayerController", "NFHUD", "NFGameMode",
                   "NFInteractableActor", "NFInteractorComponent"]:
    lines.append("class_{}={}".format(class_name, hasattr(unreal, class_name)))

# Le personnage C++ doit avoir récupéré les assets du template
if hasattr(unreal, "NFCharacter"):
    cdo = unreal.get_default_object(unreal.NFCharacter)
    body = cdo.get_editor_property("mesh")
    fp_mesh = cdo.get_editor_property("first_person_mesh")
    lines.append("character_body_mesh=" + str(body.get_editor_property("skeletal_mesh_asset").get_name()))
    lines.append("character_body_anim=" + str(body.get_editor_property("anim_class").get_name()))
    lines.append("character_fp_anim=" + str(fp_mesh.get_editor_property("anim_class").get_name()))
    for prop in ["jump_action", "move_action", "look_action", "mouse_look_action"]:
        action = cdo.get_editor_property(prop)
        lines.append("character_{}={}".format(prop, action.get_name() if action else None))
    contexts = cdo.get_editor_property("default_mapping_contexts")
    lines.append("character_mapping_contexts=" + ",".join(c.get_name() for c in contexts))

out = os.path.join(unreal.Paths.project_saved_dir(), "nf_check_env.txt")
with open(out, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
for line in lines:
    unreal.log("NF_CHECK " + line)
