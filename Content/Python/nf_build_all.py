"""Neuron Forge - génère tout le contenu du jeu (à lancer éditeur fermé).

    UnrealEditor-Cmd.exe ProjetNeuro.uproject -run=pythonscript
        -script="<chemin absolu>/Content/Python/nf_build_all.py" -unattended -nullrhi

Étapes : sons -> materials -> Blueprints -> niveau L_Lab.
Rapport écrit dans Saved/nf_build_report.txt.
"""
import os
import sys
import traceback

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import nf_assets  # noqa: E402
import nf_level  # noqa: E402

report = []


def step(name, function, *args):
    try:
        result = function(*args)
        report.append("[OK] " + name)
        return result
    except Exception:
        report.append("[ERREUR] " + name + "\n" + traceback.format_exc())
        raise


def main():
    try:
        sounds = step("Sons (génération + import)", nf_assets.import_sounds)
        materials = step("Materials", nf_assets.create_materials)
        step("Materials du décor (ambiance, sol, serveurs, hologrammes)", nf_assets.create_lab_materials, materials)
        blueprints = step("Blueprints", nf_assets.create_blueprints, materials, sounds)
        step("Niveau " + nf_level.LEVEL_PATH, nf_level.build_level, blueprints, materials)
        step("Enregistrement", nf_assets.save_all)
        report.append("Sons : " + ", ".join(sorted(sounds)))
        report.append("Materials : " + ", ".join(sorted(m.get_name() for m in materials.values() if m)))
        report.append("Blueprints : " + ", ".join(sorted(b.get_name() for b in blueprints.values())))
    finally:
        path = os.path.join(unreal.Paths.project_saved_dir(), "nf_build_report.txt")
        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(report) + "\n")


main()
