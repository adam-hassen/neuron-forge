"""Neuron Forge - vérifie que Python headless fonctionne et voit les assets du projet.

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
out = os.path.join(unreal.Paths.project_saved_dir(), "nf_check_env.txt")
with open(out, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
for line in lines:
    unreal.log("NF_CHECK " + line)
